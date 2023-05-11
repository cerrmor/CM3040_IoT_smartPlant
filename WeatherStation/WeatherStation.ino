/*This is the central node. this is also the weather station.
 *all node request are sent from here and the dashboard website is 
 *built from this node*/
//Libraries
#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


//Constants
#define NUM_SLAVES 2
#define LED D4
//calibrates the atmospheric reading
#define SEALEVELPRESSURE_HPA (1013.25)
//screen constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// The photoresistor night threshold
const int nightThreshold = 550;
// The photoresistor full sun threshold
const int fullSunThreshold = 1024;
// Initialise the photoresistor pin
const int photo_sensor = A0;

//Parameters
String nom = "Master";
//Initialise the variable names
float temperature, humidity, pressure, altitude;

//Wi-Fi Parameters
const char* ssid = "TELUS0669-2.4G";
const char* password = "pcjyn4n8rn";

//Variables
bool sendCmd = false;
String slaveCmd = "0";
String slaveState = "0";
String slaveid;
String activeSlave;
String slave1Status = "0";
String slave2Status = "0";
String manualMode1 = "0";
String manualMode2 = "0";
// The photoresistor value coming from the photo sensor
int photoValue = 0;
//The sensor values  
String soilMoisture = "Not Detected";
String waterLevel = "Not Detected";

//Objects
// Declaration for an BME-280 I2C environmental sensor
Adafruit_BME280 bme;
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiServer server(80);
WiFiClient browser;
IPAddress ip(192, 168, 1, 44);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);

//*********************************************************************
void setup() 
{
  //Init Serial USB
  Serial.begin(115200);
  Serial.println(F("Initialize System"));

  //Init the BME-280 sensor
  bme.begin(0x77);

  //Init PINMODE
  pinMode(photo_sensor, INPUT);
  
  //Init ESP8266 Wifi
  WiFi.config(ip, gateway, subnet);       // forces to use the fix IP
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(F("."));
  }
  
  server.begin();
  Serial.print(nom);
  Serial.print(F(" connected to Wifi! IP address : http://")); Serial.println(WiFi.localIP()); // Print the IP address
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
  //Init the Sensor Values
  sensor();
  //handles the request
  clientRequest();
  //displays the static display text
  displayOutput();
}

//*********************************************************************
//Sets the output for light intensity based off measured value
String sunlight()
{
  if(photoValue <= nightThreshold)
  {
    return ("Low Sun");
  }
  else if(photoValue > nightThreshold && photoValue < fullSunThreshold)
  {
    return ("Mild Sun");
  }
  return("Full Sun");
}

//*********************************************************************
void sensor()
{
  //BME-280 sensor readings
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  //Light intensity readings
  photoValue = analogRead(photo_sensor); // Read the value of the photo resistor (0--1023)
}

//*********************************************************************
void displayOutput()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  
  // Display static text
  display.print("Temp: ");
  display.print(temperature);
  display.print(" C");
  display.setCursor(0, 16);
  display.print("Hum: ");
  display.print(humidity);
  display.print(" %");
  display.setCursor(0, 25);
  display.print("Press: ");
  display.print(pressure);
  display.print(" hPa");
  display.setCursor(0, 33);
  display.print("Alti: ");
  display.print(altitude);
  display.print(" m");
  display.println("");
  display.println("Dashboard Address:");
  display.println(WiFi.localIP()); 
  
  display.display();
  delay(200);
  display.clearDisplay(); 
}

//*********************************************************************
/* function clientRequest */
void clientRequest( ) 
{
  ////Check if client connected
  WiFiClient client = server.available();
  client.setTimeout(50);
  
  if (client) 
  {
    
    if (client.connected()) 
    {    
      //Print client IP address
      Serial.print(" ->");Serial.println(client.remoteIP());
     
      //receives the message from the client
      String request = client.readStringUntil('\r');
      
      if (request.indexOf("Irrigation") == 0 ||request.indexOf("WaterPump") == 0) 
      {
        //Handle slave request
        Serial.print("From "); Serial.println(request);
        int index = request.indexOf(":");
        slaveid = request.substring(0, index);
        Serial.print("The slave ID: ");
        Serial.println(slaveid);
        slaveState = request.substring(request.indexOf("x") + 1, request.length());

        Serial.print("state received: "); Serial.println(slaveState);

        //controls which node get updated
        if(slaveid == "Irrigation")
        {
          slave1Status = slaveState;
          Serial.print("slave1Status: "); Serial.println(slave1Status);
          soilMoisture = request.substring(request.indexOf("z") + 1, request.indexOf("%"));
        }
        if(slaveid == "WaterPump")
        {
          slave2Status = slaveState;
          Serial.print("slave2Status: "); Serial.println(slave2Status);
          waterLevel = request.substring(request.indexOf("z") + 1, request.indexOf("%"));
        }
        client.print(nom);

        //sends the response to the node
        if (sendCmd) 
        {
          sendCmd = false;
          client.println(": Ok " + slaveid + "! The active slave is " + "(" + activeSlave + ")" + "Set mode to z" + manualMode1 + manualMode2 + " Set state to x" + String(slaveCmd) + "\r");
        } 
        
        else 
        {
          // sends the answer to the client
          client.println(": Hi " + slaveid + "!\r"); 
        }
        
        // terminates the connection with the client
        client.stop();
      } 
      //handles the webpage request
      else 
      {
        Serial.print("From Browser : "); Serial.println(request);
        client.flush();
        handleRequest(request);
        webpage(client);
      }
    }
  }
}

//*********************************************************************
/* function handleRequest */
//updates transmitted settings
void handleRequest(String request) 
{
  ////Check if client connected
  if (request.indexOf("/pump1on") > 0)
  {
    activeSlave = "Irrigation";
    sendCmd = true;
    slaveCmd = "1";
  }
  if (request.indexOf("/pump1off") > 0)
  {
    activeSlave = "Irrigation";
    sendCmd = true;
    slaveCmd = "0";
  }
  if (request.indexOf("/pump2on") > 0)
  {
    activeSlave = "WaterPump";
    sendCmd = true;
    slaveCmd = "1";
  }
  if (request.indexOf("/pump2off") > 0)
  {
    activeSlave = "WaterPump";
    sendCmd = true;
    slaveCmd = "0";
  }
  if (request.indexOf("/automatic1") > 0)
  {
    manualMode1 = "0";
  }
  if (request.indexOf("/manual1") > 0)
  {
    manualMode1 = "1";
  }
  if (request.indexOf("/automatic2") > 0)
  {
    manualMode2 = "0";
  }
  if (request.indexOf("/manual2") > 0)
  {
    manualMode2 = "1";
  }
}

//*********************************************************************
/* Creates The Webpage */
void webpage(WiFiClient browser) 
{
  //Send webpage to browser
  browser.println("HTTP/1.1 200 OK");
  browser.println("Content-Type: text/html");
  browser.println(""); //  do not forget this one
  browser.println("<!DOCTYPE HTML>"); 
  browser.println("<html>");
  browser.println("<head>");
  browser.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
  browser.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
  
  //AJAX script for refreshing the elements on the page
  browser.println("<script>");
    browser.println("setInterval(loadDoc,1000);");
    browser.println("function loadDoc() {");
      browser.println("var xhttp = new XMLHttpRequest();");
      browser.println("xhttp.onreadystatechange = function() {");
      browser.println("if (this.readyState == 4 && this.status == 200) {");
        browser.println("document.body.innerHTML = this.responseText}");
      browser.println("};");
      browser.println("xhttp.open(\"GET\", \"/\", true);");
      browser.println("xhttp.send();");
    browser.println("}");
  browser.println("</script>");
  browser.println("</head>");
  
  browser.println("<body style = ' background-color:#000000; color:white;'>");
  
    browser.println("<hr/><hr>");
      browser.println("<h1><center> ESP-8266 Smart Irrigation System </center></h1>");
    browser.println("<hr/><hr>");
    
    browser.println("<br><br>");
    browser.println("<br><br>");
    
    browser.println("<h2> Irrigation Control </h2>");

   
    browser.println("<center>");
      browser.println("Irrigation Pump");
    browser.println("</center>");
    
    browser.println("<br>");
    
    browser.println("<center>");
      browser.println("<a href='/pump1on'><button>Turn On </button></a>");
      browser.println("<a href='/pump1off'><button>Turn Off </button></a><br />");
    browser.println("</center>");

    browser.println("<br>");
    
    browser.println("<center>");
      browser.println("<table>");
        browser.println("<tr>");  
          browser.println("<td>Control Mode:</td>"); 
          if (manualMode1 == "1")
          {
            browser.print("<td>Manual</td>");
          }
          else
          {
            browser.print("<td>Automatic</td>");
          }
        browser.println("</tr>");
      browser.println("</table>");   
    browser.println("</center>");
    
    browser.println("<br>");
    
    browser.println("<center>");
      browser.println("<a href='/automatic1'><button style ='background-color:green'>Automatic </button></a>");
      browser.println("<a href='/manual1'><button style ='background-color:red'>Manual </button></a><br />");
    browser.println("</center>");
    
    
    browser.println("<br><br>");
    
    browser.println("<center>");
      browser.println("Water Reservoir Pump");
    browser.println("</center>");
    
    browser.println("<br>");
    
    browser.println("<center>");
      browser.println("<a href='/pump2on'><button>Turn On </button></a>");
      browser.println("<a href='/pump2off'><button>Turn Off </button></a><br />");
    browser.println("</center>");

    browser.println("<br>");
    
    browser.println("<center>");
      browser.println("<table>");     
        browser.println("<tr>");
          browser.println("<td>Control Mode:</td>");   
          if (manualMode2 == "1")
          {
            browser.print("<td>Manual</td>");
          }
          else
          {
            browser.print("<td>Automatic</td>");
          }
        browser.println("</tr>");
      browser.println("</table>");
    browser.println("</center>");

    browser.println("<br>");
    
    browser.println("<center>"); 
      browser.println("<a href='/automatic2'><button style ='background-color:green'>Automatic </button></a>");
      browser.println("<a href='/manual2'><button style ='background-color:red'>Manual </button></a><br />");
    browser.println("</center>");
    
    browser.println("<br><br>");
    browser.println("<br><br>");
    
    browser.println("<h2> Data </h2>");
    
    browser.println("<center>");
      browser.println("<table border='1'>");
        
        browser.println("<tr>");
          browser.print("<td>Temperature</td>");
          browser.print("<td>");
            browser.print(temperature);
          browser.print(" &deg;C</td>");
        browser.println("</tr>");

        browser.println("<tr>");
          browser.print("<td>Humidity</td>");
          browser.print("<td>");
            browser.print(humidity);
          browser.print(" %</td>");
        browser.println("</tr>");

        browser.println("<tr>");
          browser.print("<td>Barometric Pressure</td>");
          browser.print("<td>");
            browser.print(pressure);
          browser.print(" hPa</td>");
        browser.println("</tr>");

        browser.println("<tr>");
          browser.print("<td>Altitude</td>");
          browser.print("<td>");
            browser.print(altitude);
          browser.print(" m</td>");
        browser.println("</tr>");

        browser.println("<tr>");
          browser.print("<td>Light Level</td>");
          browser.print("<td>");
            browser.print(sunlight());
          browser.print("</td>");
        browser.println("</tr>");
        
        browser.println("<tr>");
          browser.print("<td>Soil Moisture</td>");
          browser.print("<td>");
            browser.print(soilMoisture);
          browser.print(" %</td>");
        browser.println("</tr>");

        browser.println("<tr>");
          browser.print("<td>Water Reservoir</td>");
          browser.print("<td>");
            browser.print(waterLevel);
          browser.print(" %FULL</td>");
        browser.println("</tr>");
      
      browser.println("</table>");
        
      browser.println("<table border='1'>");    
        browser.println("<tr>");  
          if (slave1Status == "0")//slaveState == "1" && slaveid == "Irrigation"
          {
            browser.print("<td>Irrigation is OFF</td>");
          }
          else
          {
            browser.print("<td>Irrigation is ON</td>");
          }
          browser.println("<br />");
          if (slave2Status == "0") //slaveState == "1" && slaveid == "WaterPump"
          {
            browser.print("<td>Water Pump is OFF</td>");
          }
          else
          {
            browser.print("<td>Water Pump is ON</td>");
          }
        browser.println("</tr>");
      browser.println("</table>");
    browser.println("</center>");
  
  browser.println("</body></html>");
  delay(1);
}
