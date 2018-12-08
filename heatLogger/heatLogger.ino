#include <math.h>         //loads the more advanced math functions
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <functional>
#include <ArduinoOTA.h>
#include <DNSServer.h> //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* Adafruit.io Setup *********************************/
// if you like connect to adafruit you can use this setup
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "aarner"
#define AIO_KEY         "0495233650944ae981ed51e43303e6ce"
#define AIO_MQTT_FEED     "/feeds/ledgemont-slash-energy"
/*************************  *********************************/
 
#define CALIBRATION_SCALING  0.6035 

 /************ Global State (you don't need to change this!) ******************/
    // Create an ESP8266 WiFiClient class to connect to the MQTT server.
    WiFiClient client;
    
    // Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
    Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
    Adafruit_MQTT_Publish energyPublisher = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME AIO_MQTT_FEED);
/*******************************Sketch Code************************************/
// prototypes
boolean connectWifi();

// this can be empty as device uses WIFI manager to setup wifi
const char* ssid = "";
const char* password = "";
unsigned int wifiDropCount = 0;
boolean wifiConnected = false;

void setup() {            //This function gets called when the Arduino starts
  Serial.begin(115200);
  Serial.print("Allocating WifiManager...");
    
  WiFiManager wifiManager;

  Serial.println("Connecting To AP...");
  wifiManager.autoConnect("D1Mini.001");

  Serial.print("Verifying Connection present...");
  // Initialise wifi connection
  wifiConnected = connectWifi();
  Serial.println(wifiConnected);

  //handle OTA update of sketch
// Port defaults to 8266
  ArduinoOTA.setPort(8266);
        
// Hostname defaults to esp8266-Switch_NAME
  ArduinoOTA.setHostname("energyLogger");
        
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
}

void(* resetFunc) (void) = 0;//declare reset function at address 0
 
 
double Thermister(int RawADC) {  //Function to perform the fancy math of the Steinhart-Hart equation
 double Temp;
 Temp = log(((10240000/RawADC) - 10000));
 Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
 Temp = Temp - 273.15;              // Convert Kelvin to Celsius
 Temp = (Temp * (9.0/5.0)) + 32.0; // Celsius to Fahrenheit - comment out this line if you need Celsius
 return Temp;
}
 
void loop() {             //This function loops while the arduino is powered

  ArduinoOTA.handle();
  if (wifiConnected) {
    MQTT_connect();
  }
  double temp=0;            //Variable to hold a temperature value
  for (int l=0; l<=60; l++) {  //capture 60 samples to smooth odd values from analog sensor
    int val;                //Create an integer variable  
      
    val=analogRead(0);      //Read the analog port 0 and store the value in val
    temp += Thermister(val);   //Runs the fancy math on the raw analog value
    delay(100);
  }
  temp = temp / 60;  //mean average of last 60 samples
  double scaledTemp = temp * CALIBRATION_SCALING;
  Serial.print("Raw value:\t");
  Serial.print(temp);
  Serial.print("\tScaled:\t");
  Serial.println(scaledTemp);
  energyPublisher.publish(scaledTemp);
  
//  delay(6000);            //don't create more than 10 records/minute to stay under adafruit.io free MQTT tier of 30/min

  //  wifi connection test
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("\tWifi Awol - incrementing drop count - now:");
      wifiDropCount++;
      Serial.println(wifiDropCount);
      delay(250);
      if (wifiDropCount > 5) {
        Serial.println("Wifi Missing for 90+ seconds.  Resetting...");
        resetFunc();  //call reset        
      }
    } else {
//      Serial.println("\tWifi tests OK...");
      wifiDropCount = 0;
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

