#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include "CurrentSense.h"
#include "SinricSwitch.h"

#define MyApiKey "d66b0116-ac4f-43ed-9087-5d0e154554c4" 
#define MySSID "Dutenhoefer"        
#define MyWifiPassword "CepiCire99" 

#define DEVICE_ID "5c452df5fd3cb538a2ba449f"  // deviceId is the ID assgined to your smart-home-device in sinric.com dashboard. Copy it from dashboard and paste it here
#define LAN_HOSTNAME  "TangoWhiskeyZero"
/************ define pins *********/
#define RELAY   D1   // D1 drives 120v relay
#define CONTACT D3   // D3 connects to momentary switch
#define LED D4       // D4 powers switch illumination

void alertViaLed();
void toggleRelay();
void resetModule();
void setLedState(boolean currentFlow);

SinricSwitch *sinricSwitch = NULL;

void(* resetFunc) (void) = 0;//declare reset function at address 0
 
void setup() {

    pinMode(LED, OUTPUT);
    pinMode(RELAY, OUTPUT);
    pinMode(CONTACT, INPUT_PULLUP);
    pinMode(A0, INPUT);
  
    digitalWrite(LED, HIGH);   // default switch illumination to on
  
    Serial.begin(115200);
  
    WiFi.begin(MySSID, MyWifiPassword);
    Serial.println();
    Serial.print("Connecting to Wifi: ");
    Serial.println(MySSID);  

    // Waiting for Wifi connect
    while(WiFi.status() != WL_CONNECTED) {
        delay(150);
        Serial.print(".");
    }
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("WiFi connected. ");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("OTA Host: ");
        Serial.println(LAN_HOSTNAME);


        Serial.println("Setting up ArduinoOTA handlers...");  
        ArduinoOTA.setPort(8266);
        ArduinoOTA.setHostname(LAN_HOSTNAME);
        ArduinoOTA.onStart([]() {  Serial.println("Start");  });
        ArduinoOTA.onEnd([]()   {  Serial.println("\nEnd");  });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));   });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });
        ArduinoOTA.begin();   
        
        sinricSwitch = new SinricSwitch(MyApiKey, DEVICE_ID, 80, toggleRelay, alertViaLed, resetModule);

  }
}

void loop() {
  sinricSwitch -> loop();

  //check whether the light is on.  (samples for 250 msec so effectively debounces the momentary switch)
  float currentFlow = calcCurrentFlow(false);
  boolean newState = (currentFlow > CURRENT_FLOW_NONZERO_THRESHOLD);
  sinricSwitch -> setPowerState(newState);
    
  boolean buttonState = digitalRead(CONTACT);
  if (buttonState == LOW) {
      toggleRelay(); 
  }
  setLedState(newState);
  ArduinoOTA.handle();
}

void toggleRelay() {
    Serial.println("Toggling switch...");
    digitalWrite(RELAY, !digitalRead(RELAY));
}

void setLedState(boolean currentFlowing) {
    digitalWrite(LED, !currentFlowing);
}

void alertViaLed() {
    //blink the led for 2.4 sec
    for(int i=0; i<24; i++) {
        digitalWrite(LED, LOW);
        delay(50);
        digitalWrite(LED, HIGH);
        delay(50);
    }
}

void resetModule() {
  Serial.println("Someone requested a reset...");
  resetFunc();
}
  




