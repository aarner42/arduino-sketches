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
#define RELAY D1     // D1 drives 120v relay
#define CONTACT D3   // D3 connects to momentary switch
#define LED D4       // D4 powers switch illumination
/************ define names for device in alexa, hostname, mqtt feed ****************/
#define ALEXA_NAME    "Zed Alpha Three"
#define LAN_HOSTNAME  "ZedAlphaThree"

String alexaCommand = ALEXA_NAME; // for example family room"
const char Switch_Name[] = LAN_HOSTNAME;
boolean lightState = false;

/*************************** Sketch Code ************************************/
// prototypes
boolean connectWifi();

//on/off callbacks 
void lightsOn();  
void lightsOff();


// this can be empty as device uses WIFI manager to setup wifi
const char* ssid = "";
const char* password = "";
unsigned int wifiDropCount = 0;
boolean wifiConnected = false;
UpnpBroadcastResponder upnpBroadcastResponder;

Switch *light = NULL;

//called at startup
void setup() {
  
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(CONTACT, INPUT_PULLUP);
  
  digitalWrite(LED, HIGH);   // default switch illumination to on
  digitalWrite(RELAY, LOW);  // shut off the high voltage relay side
  
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
    light = new Switch(alexaCommand, 80, lightsOn, lightsOff);

    Serial.println("Adding switches upnp broadcast responder");
    upnpBroadcastResponder.addDevice(*light);
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
    light->serverLoop();
  }
    boolean buttonState = digitalRead(CONTACT);
    if (buttonState == LOW) {
      if(lightState == true) {
        lightsOff();
      } else {
        lightsOn();
      }
      //affect a debounce - keep momentary switch looping multiple times/sec
      delay(1000);
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

void lightsOn() {
    Serial.println("Switch 1 turn on ...");
    digitalWrite(LED, LOW);
    digitalWrite(RELAY, HIGH);
    lightState = true;
}

void lightsOff() {
    Serial.println("Switch 1 turn off ...");
    digitalWrite(LED, HIGH);
    digitalWrite(RELAY, LOW);
    lightState = false;
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
