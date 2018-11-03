#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <functional>
#include <ArduinoOTA.h>

#include <DNSServer.h> //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include "switch.h"
#include "UpnpBroadcastResponder.h"
#include "CallbackFunction.h"

/************ define pins *********/
#define OPENER_RELAY D0        // D1 drives 120v relay
#define OPEN_SENSE D5   // D5 Connects to open sensor
#define CLOSE_SENSE D6  // D6 Connects to close sensor
/************ define names for device in alexa, hostname, mqtt feed ****************/
#define ALEXA_NAME    "Garage"
#define LAN_HOSTNAME  "ErxlebenGarage"

String alexaCommand = ALEXA_NAME; // for example family room"
const char Switch_Name[] = LAN_HOSTNAME;
boolean isDoorClosed = false;
unsigned int doorStatus = 0;
/*************************** Sketch Code ************************************/
// prototypes
boolean connectWifi();

//on/off callbacks 
void doorOpen();  
void doorClose();


// this can be empty as device uses WIFI manager to setup wifi
const char* ssid = "";
const char* password = "";
unsigned int wifiDropCount = 0;
boolean wifiConnected = false;
UpnpBroadcastResponder upnpBroadcastResponder;

Switch *door = NULL;

//called at startup
void setup() {
  
  pinMode(OPEN_SENSE,  INPUT_PULLUP);
  pinMode(CLOSE_SENSE, INPUT_PULLUP);

  pinMode(OPENER_RELAY, OUTPUT);
  digitalWrite(OPENER_RELAY, LOW);
  
  Serial.begin(115200);
  Serial.print("Allocating WifiManager...");
    
  WiFiManager wifiManager;

  Serial.println("Connecting To AP...");
  wifiManager.autoConnect("D1Mini.001");

  Serial.println("Verifying Connection present...");
  // Initialise wifi connection
  wifiConnected = connectWifi();
  
  if(wifiConnected){
    upnpBroadcastResponder.beginUdpMulticast();
    
    // Define your switches here. Max 14
    // Format: Alexa invocation name, local port no, on callback, off callback
    door = new Switch(alexaCommand, 80, doorOpen, doorClose);

    Serial.println("Adding switches upnp broadcast responder");
    upnpBroadcastResponder.addDevice(*door);
  }


// Port defaults to 8266
  ArduinoOTA.setPort(8266);
        
// Hostname defaults to esp8266-Switch_NAME
  ArduinoOTA.setHostname(Switch_Name);
     
  ArduinoOTA.onStart([]() {
     Serial.println("Start");
   });
   
  ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
   });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
       Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
   });
   
  ArduinoOTA.begin();   

}

void(* resetFunc) (void) = 0;//declare reset function at address 0
 
void loop()
{
  ArduinoOTA.handle();

  if (wifiConnected) {
    upnpBroadcastResponder.serverLoop();
    door->serverLoop();
  }

//  wifi connection test
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("\tWifi Awol - incrementing drop count - now:");
      wifiDropCount++;
      Serial.println(wifiDropCount);
      delay(250);
      if (wifiDropCount > 100) {
        Serial.println("Wifi Missing for 30+ seconds.  Resetting...");
        resetFunc();  //call reset        
      }
    } else {
//      Serial.println("\tWifi tests OK...");
      wifiDropCount = 0;
    }

  //poll the door sensors
  if (digitalRead(OPEN_SENSE) == LOW) {
    doorStatus = 0;
    door->setDoorState(0);
    Serial.println("Sensor reports door OPEN");
  }
  if (digitalRead(CLOSE_SENSE) == LOW) {
    doorStatus = 1;
    door->setDoorState(1);
    Serial.println("Sensor reports door CLOSED");
  }
  if ((digitalRead(OPEN_SENSE)==HIGH) && (digitalRead(CLOSE_SENSE)==HIGH)) {
    doorStatus = 2;
    door->setDoorState(2);
    Serial.println("Sensor reports door TRAVELING");
  }
  if ((digitalRead(CLOSE_SENSE) == LOW) && (digitalRead(OPEN_SENSE) == LOW)) {
    doorStatus = 3;
    door->setDoorState(3);
    Serial.println("Sensor reports door IS ALL JACKED UP!");
  }
  delay(1000);
}

void doorOpen() {
    Serial.print("Cycle garage door...");
    digitalWrite(OPENER_RELAY, HIGH);
    delay(350);
    digitalWrite(OPENER_RELAY, LOW);
    Serial.println("Done.");
}

void doorClose() {
    Serial.print("Cycle garage door...");
    digitalWrite(OPENER_RELAY, HIGH);
    delay(350);
    digitalWrite(OPENER_RELAY, LOW);
    Serial.println("Done.");    
}

// connect to wifi – returns true if successful or false if not
boolean connectWifi(){
  boolean state = true;
  int i = 0;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to Home WiFi");

  // Wait for connection
  Serial.print("Connecting ...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 100){
      state = false;
      break;
    }
    i++;
  }
  
  if (state){
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("");
    Serial.println("Connection failed.");
  }
  
  return state;
}
