#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <functional>

#include <DNSServer.h> //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include "switch.h"
#include "UpnpBroadcastResponder.h"
#include "CallbackFunction.h"

/************ define pins *********/
#define RELAY D1     // D1 drives 120v relay
#define CONTACT D2   // D2 connects to momentary switch
#define LED D4       // D3 powers switch illumination
/************ define names for device in alexa, hostname, mqtt feed ****************/
#define ALEXA_NAME    "Zed Alpha One"
#define LAN_HOSTNAME  "ZedAlphaOne"

//Global variables for Alexa/Siri
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

boolean wifiConnected = false;
UpnpBroadcastResponder upnpBroadcastResponder;

Switch *light = NULL;

//called at startup
void setup() {
  
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(CONTACT, INPUT);
  
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
}

void(* resetFunc) (void) = 0;//declare reset function at address 0
 
void loop()
{

 
  if(wifiConnected){
    upnpBroadcastResponder.serverLoop();
      
    light->serverLoop();
	  
    }
    Serial.print("Checking for HW switch press...");
    boolean buttonState = digitalRead(CONTACT);
    Serial.print("button state:");
    Serial.println(buttonState);
    if (buttonState == LOW) {
      if(lightState == true) {
        lightsOn();
        lightState = false;
      } else {
        lightsOff();
        lightState = true;
      }
    }
     

//  wifi connection test
    if(WiFi.status() != WL_CONNECTED ){
      boolean wifiOK = (WiFi.status() != WL_CONNECTED);
      for (int i=0; i<10; i++ && !wifiOK) {
        Serial.println("Possible wifi failure...rechecking");
        delay(1000);
        wifiOK = (WiFi.status() != WL_CONNECTED);
      }
      if (!wifiOK) {
         Serial.println("No WiFi - resetting");
         resetFunc();  //call reset
      }
  }
}

        
void lightsOn() {
    Serial.print("Switch 1 turn on ...");
    digitalWrite(LED, LOW);
    digitalWrite(RELAY, HIGH);
    lightState = true;
}

void lightsOff() {
    Serial.print("Switch 1 turn off ...");
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
