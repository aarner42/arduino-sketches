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
#define RELAY_ONE D7     // D7 drives 120v relay
#define RELAY_TWO D0     // D0 drives 120v relay
#define CONTACT_ONE D2   // D2 connects to momentary switch
#define CONTACT_TWO D3   // D3 connects to momentary switch
#define LED_ONE D5       // D5 powers switch illumination
#define LED_TWO D6       // D6 powers switch illumination
/************ define names for device in alexa, hostname, mqtt feed ****************/
#define ALEXA_NAME_ONE    "Tango Golf One"
#define LAN_HOSTNAME      "TangoGolf1-2"
#define ALEXA_NAME_TWO    "Tango Golf Two"


String alexaCommandOne = ALEXA_NAME_ONE; // for example family room"
const char Switch_Name[] = LAN_HOSTNAME;
String alexaCommandTwo = ALEXA_NAME_TWO; // for example family room"


/*************************** Sketch Code ************************************/
// prototypes
boolean connectWifi();

//on/off callbacks 
void lightsOnOne();  
void lightsOffOne();

void lightsOnTwo();  
void lightsOffTwo();


// this can be empty as device uses WIFI manager to setup wifi
const char* ssid = "";
const char* password = "";
unsigned int wifiDropCount = 0;
boolean wifiConnected = false;
UpnpBroadcastResponder upnpBroadcastResponder;

Switch *lightOne = NULL;
Switch *lightTwo = NULL;
boolean lightStateOne;
boolean lightStateTwo;

//called at startup
void setup() {
  
  pinMode(LED_ONE, OUTPUT);
  pinMode(RELAY_ONE, OUTPUT);
  pinMode(CONTACT_ONE, INPUT_PULLUP);

  pinMode(LED_TWO, OUTPUT);
  pinMode(RELAY_TWO, OUTPUT);
  pinMode(CONTACT_TWO, INPUT_PULLUP);
  
  digitalWrite(LED_ONE, HIGH);   // default switch illumination to on
  digitalWrite(LED_TWO, HIGH);   // default switch illumination to on
  digitalWrite(RELAY_ONE, HIGH);  // shut off the high voltage relay side
  digitalWrite(RELAY_TWO, HIGH);  // shut off the high voltage relay side

  
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
    lightOne = new Switch(alexaCommandOne, 80, lightsOnOne, lightsOffOne);
    lightTwo = new Switch(alexaCommandTwo, 88, lightsOnTwo, lightsOffTwo);

    Serial.println("Adding switches upnp broadcast responder");
    upnpBroadcastResponder.addDevice(*lightOne);
    upnpBroadcastResponder.addDevice(*lightTwo);
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
    lightOne->serverLoop();
    lightTwo->serverLoop();
  }
    boolean buttonStateOne = digitalRead(CONTACT_ONE);
    if (buttonStateOne == LOW) {
      if(lightStateOne == true) {
        lightsOffOne();
      } else {
        lightsOnOne();
      }
      //affect a debounce - keep momentary switch looping multiple times/sec
      delay(333);
    }

    boolean buttonStateTwo = digitalRead(CONTACT_TWO);
    if (buttonStateTwo == LOW) {
      if(lightStateTwo == true) {
        lightsOffTwo();
      } else {
        lightsOnTwo();
      }
      //affect a debounce - keep momentary switch looping multiple times/sec
      delay(333);
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
}

void lightsOnOne() {
    Serial.println("Switch 1 turn on ...");
    digitalWrite(LED_ONE, LOW);
    digitalWrite(RELAY_ONE, LOW);
    lightStateOne = true;
    lightOne->setState(1);
    
}

void lightsOffOne() {
    Serial.println("Switch 1 turn off ...");
    digitalWrite(LED_ONE, HIGH);
    digitalWrite(RELAY_ONE, HIGH);
    lightStateOne = false;
    lightTwo->setState(0);
}


void lightsOnTwo() {
    Serial.println("Switch 2 turn on ...");
    digitalWrite(LED_TWO, LOW);
    digitalWrite(RELAY_TWO, LOW);
    lightStateTwo = true;
    lightTwo->setState(1);
    
}

void lightsOffTwo() {
    Serial.println("Switch 2 turn off ...");
    digitalWrite(LED_TWO, HIGH);
    digitalWrite(RELAY_TWO, HIGH);
    lightStateTwo = false;
    lightTwo->setState(0);
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
