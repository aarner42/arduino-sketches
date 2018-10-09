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

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* Adafruit.io Setup *********************************/
// if you like connect to adafruit you can use this setup
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "aarner"
#define AIO_KEY         "feaf5ab0e1414683993972d1ee3161cc"
 /*************************  *********************************/

/************ define pins *********/
#define RELAY D1     // D1 drives 120v relay
#define CONTACT D2   // D2 connects to momentary switch
#define LED D4       // D3 powers switch illumination
/************ define names for device in alexa, hostname, mqtt feed ****************/
#define ALEXA_NAME    "Cellar Office"
#define LAN_HOSTNAME  "cellar-office"
#define MQTT_FEED     "/feeds/ledgemont/power/cellarOffice"

String alexaCommand = ALEXA_NAME; // for example family room"
const char Switch_Name[] = LAN_HOSTNAME;
boolean lightState = false;

 /************ Global State (you don't need to change this!) ******************/
    // Create an ESP8266 WiFiClient class to connect to the MQTT server.
    WiFiClient client;
    // or... use WiFiFlientSecure for SSL
    //WiFiClientSecure client;
    
    // Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
    Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
    //Adafruit_MQTT_Publish diningLive = Adafruit_MQTT_Publish(&mqtt, HEALTH_DINING_FEED, MQTT_QOS_1);
    /****************************** Feeds ***************************************/
    
    // Setup a feed called 'onoff' for subscribing to changes.
    
    Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME MQTT_FEED);
    Adafruit_MQTT_Publish onoffbuttonPublish = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME MQTT_FEED);
    //Adafruit_MQTT_Publish lastwill = Adafruit_MQTT_Publish(&mqtt, WILL_FEED);

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
  
  digitalWrite(LED, LOW);   // default switch illumination to on
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
        
// No authentication by default
// ArduinoOTA.setPassword((const char *)"123");
        
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

////// MQTT

// Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoffbutton);

  // Setup MQTT will to set on/off to "OFF" when we disconnect
  //mqtt.will(WILL_FEED, "OFF");
}

void(* resetFunc) (void) = 0;//declare reset function at address 0
 
void loop()
{

   ArduinoOTA.handle();

    //onoffbuttonPublish.publish("OFF");  // make sure we publish OFF first thing after connecting
 
  if(wifiConnected){
    upnpBroadcastResponder.serverLoop();
      
    light->serverLoop();
	 

    MQTT_connect();
        
    Adafruit_MQTT_Subscribe *subscription;
      while ((subscription = mqtt.readSubscription(500))) {
                  if (subscription == &onoffbutton) {
            
                  Serial.print(F("Got: "));
                  Serial.println((char *)onoffbutton.lastread);
            
                  if (strcmp((char *)onoffbutton.lastread, "ON") == 0) {
                    digitalWrite(LED, LOW); 
                    digitalWrite(RELAY, HIGH);
                    Serial.print("shoule be ON");
                  }
                  if (strcmp((char *)onoffbutton.lastread, "OFF") == 0) {
                    digitalWrite(LED, HIGH);
                    digitalWrite(RELAY, LOW); 
                    Serial.print("shoule be OFF");
                  }
                  
                }
      } 
    }
    Serial.print("Checking for switch press...");
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
    if(WiFi.status() != WL_CONNECTED){
    Serial.println("resetting");
    resetFunc();  //call reset
  }
}

void MQTT_connect() {
          int8_t ret;
        
          // Stop if already connected.
          if (mqtt.connected()) {
            return;
          }
        
          Serial.print("Connecting to MQTT... ");
        
          uint8_t retries = 3;
          while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
               Serial.println(mqtt.connectErrorString(ret));
               Serial.println("Retrying MQTT connection in 5 seconds...");
               mqtt.disconnect();
               delay(5000);  // wait 5 seconds
               retries--;
               if (retries == 0) {
                 // basically die and wait for WDT to reset me
                // while (1);
                 resetFunc();  //call reset
               }
          }
          Serial.println("MQTT Connected!");
        }
        
void lightsOn() {
    Serial.print("Switch 1 turn on ...");
    digitalWrite(LED, LOW);
    digitalWrite(RELAY, HIGH);
    lightState = true;
    onoffbuttonPublish.publish("ON");
}

void lightsOff() {
    Serial.print("Switch 1 turn off ...");
    digitalWrite(LED, HIGH);
    digitalWrite(RELAY, LOW);
    lightState = false;
    onoffbuttonPublish.publish("OFF");
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
