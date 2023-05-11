# CM3040_smartPlant
Code developed for ESP8266 microcontrollers to create a smart irrigation system

The system works as a series of three different microcontroller stations:
  
  * WeatherStation 
    * central node in the system
    * monitors the temperature, barometric pressure, altitude, current light level and air moisture level
    * creates a web control hub for monitoring the system from afar
    * sends and recieves signals from the other microcontrollers in the system
    * displays the readout of all sensors and whether pumps are running or not
  
  * IrrigationStation
    * monitors the hydration level of the soil
    * activates and deactivates the pump to water the plant when conditions are right
  
  * WaterMonitoringStation
    * monitors the available water for irrigation 
    * measures the amount of water used for tracking purposes
    * refills the resovour as needed

This system was proof of concept and developed as the final project for CM3040_Internet of Things
