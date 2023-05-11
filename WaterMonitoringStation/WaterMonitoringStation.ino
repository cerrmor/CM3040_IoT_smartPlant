/*This is the Water Monitoring node. This node recieves requests from 
 * the central node and respondes with sensor data this node also 
 * controls when the irrigation resiviour is refilled*/
//Libraries
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Constants
#define LED D4
#define UPDATE_TIME 500
//Water level sensor
const int levelPin = A0;
//water pump
const int motorPin = D4;
//used for calibration of moisture sensor
const int drySensor = 22;
const int wetSensor = 400;
//screen constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

//Parameters
String nom = "WaterPump";
const char* ssid = "TELUS0669-2.4G";
const char* password = "pcjyn4n8rn";

//Variables
String command;
unsigned long previousRequest = 0;
String activeRequest;
int levelPercentage = 0;
int levelValue = 0; 
String controlMode = "0";

//Objects
WiFiClient master;
IPAddress server(192, 168, 1, 44);
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//*********************************************************************
void setup() 
{
  //Init Serial USB
  Serial.begin(115200);
  Serial.println(F("Initialize System"));

  //Init PINMODE
  pinMode(levelPin, INPUT);
  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);
  
  //Init ESP8266 Wifi
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(F("."));
  }
  
  Serial.print(nom);
  Serial.print(F(" connected to Wifi! IP address : ")); Serial.println(WiFi.localIP()); // Print the IP address
  pinMode(LED, OUTPUT);

  //Init SSD1306 display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
}

//*********************************************************************
void loop() 
{
  //handles the request
  requestMaster();
  //controls the node
  waterLevel();
  //displays the static display text
  displayOutput();
}

//*********************************************************************
void waterLevel()
{
  //logic controlling the automatic opperation
  levelValue = analogRead(levelPin);
  Serial.println(levelValue);
  levelPercentage = map(levelValue, drySensor, wetSensor, 0 ,100);
  levelPercentage = constrain(levelPercentage, 0, 100);

  if(controlMode == "0")
  {
    if(levelPercentage < 20)
    {
      Serial.println("Reservoir is almost empty, turning on pump!");
      digitalWrite(motorPin, HIGH);
    }
    else if(levelPercentage > 75)
    {
      Serial.println("Reservoir has been filled, turning off pump!");
      digitalWrite(motorPin, LOW);
    }
  }
}

//*********************************************************************
void displayOutput()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
 
  // Display static text
  display.print("Water Level:");
  display.print(levelPercentage);
  display.print(" %FULL");
  
  if(String(digitalRead(LED)) == "1")
  {
    display.setTextSize(3);
    display.setCursor(28, 16);
    display.print("PUMP");
    display.setCursor(38, 39);
    display.print("ON");
    display.setTextSize(1);
  }
  else
  {
    display.setTextSize(3);
    display.setCursor(28, 16);
    display.print("PUMP");
    display.setCursor(32, 39);
    display.print("OFF");
    display.setTextSize(1);
  }
  
  display.display();
  delay(200);
  display.clearDisplay();
}

//*********************************************************************

/* Handles the request from the server*/
void requestMaster( ) 
{
  //Request to master
  // client connect to server every 500ms
  if ((millis() - previousRequest) > UPDATE_TIME) 
  { 
    previousRequest = millis();
    
    // Connection to the server
    if (master.connect(server, 80)) 
    { 
      //message to the server
      master.println(nom + ": Hello! the Reservoir is z" + String(levelPercentage) + "% full and my current state is x" + String(digitalRead(LED)) + "\r");
      
      //answer
      String answer = master.readStringUntil('\r');   // receives the answer from the sever
      master.flush();
      Serial.println("from " + answer);
      
      if (answer.indexOf("x") >= 0) 
      {
        //lets the node know if the request is for it
        activeRequest = answer.substring(answer.indexOf("(") + 1, answer.indexOf(")"));
        //lets the node know if it is being manually controlled
        controlMode = answer.substring(answer.indexOf("z") + 2, answer.indexOf("z") + 3);
        //lets the node know what it is supposed to do 
        command = answer.substring(answer.indexOf("x") + 1, answer.length());
        Serial.print("command received: "); 
        Serial.print(command);
        Serial.println(" for " + activeRequest);

        //controls the pump
        if (command == "1" && activeRequest == nom) 
        {
          Serial.println("PUMP ON");
          digitalWrite(motorPin, HIGH);
        } 
        
        else if(activeRequest == nom && command != "1")
        {
          Serial.println("PUMP OFF");
          digitalWrite(motorPin, LOW);
        }
      }
    }
  }
}
