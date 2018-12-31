/*
 Version 0.3 - March 06 2018
*/ 

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h> 
#include <ArduinoJson.h> 
#include <StreamString.h>
#include <ArduinoOTA.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

#define MyApiKey "d66b0116-ac4f-43ed-9087-5d0e154554c4" 
#define MySSID "Dutenhoefer"        
#define MyWifiPassword "CepiCire99" 
#define DEVICE_ID1 "5c27cd56c37e0372a25b6fdd"  // deviceId is the ID assgined to your smart-home-device in sinric.com dashboard. Copy it from dashboard and paste it here
#define DEVICE_ID2 "5c27cd6ac37e0372a25b6fde"  // deviceId is the ID assgined to your smart-home-device in sinric.com dashboard. Copy it from dashboard and paste it here

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 
#define LAN_HOSTNAME  "TangoGolf1-2"
/************ define pins *********/
#define RELAY_ONE D7     // D7 drives 120v relay
#define RELAY_TWO D0     // D0 drives 120v relay
#define CONTACT_ONE D2   // D2 connects to momentary switch
#define CONTACT_TWO D3   // D3 connects to momentary switch
#define LED_ONE D5       // D5 powers switch illumination
#define LED_TWO D6       // D6 powers switch illumination

uint64_t heartbeatTimestamp = 0;
boolean isConnected = false;
boolean lightStateOne = false;
boolean lightStateTwo = false;

void setPowerStateOnServer(String deviceId, String value);
void lightsOn(uint8_t relayPin, uint8_t ledPin, String deviceId, boolean notifyServer);  
void lightsOff(uint8_t relayPin, uint8_t ledPin, String deviceId, boolean notifyServer);

void turnOn(String deviceId) {
  if (deviceId == DEVICE_ID1) { // Device ID of first device 
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    lightsOn(RELAY_ONE, LED_ONE, deviceId, false);
  } else if (deviceId == DEVICE_ID2) {
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    lightsOn(RELAY_TWO, LED_TWO, deviceId, false);  
  } else {
    Serial.print("Turn on for unknown device id: ");
    Serial.println(deviceId);    
  }     
}

void turnOff(String deviceId) {
   if (deviceId == DEVICE_ID1) { // Device ID of first device
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
     lightsOff(RELAY_ONE, LED_ONE, deviceId, false);
   } else if (deviceId == DEVICE_ID2) {
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
     lightsOff(RELAY_TWO, LED_TWO, deviceId, false);
   } else {
     Serial.print("Turn off for unknown device id: ");
     Serial.println(deviceId);    
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");        
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);
        // Example payloads

        // For Switch or Light device types
        // {"deviceId": xxxx, "action": "setPowerState", value: "ON"} // https://developer.amazon.com/docs/device-apis/alexa-powercontroller.html

        // For Light device type
        // Look at the light example in github
          
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload); 
        String deviceId = json ["deviceId"];     
        String action = json ["action"];
        
        if(action == "setPowerState") { // Switch or Light
            String value = json ["value"];
            if(value == "ON") {
                turnOn(deviceId);
            } else {
                turnOff(deviceId);
            }
        }
        else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
  }
}


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
  
  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);  

  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    
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

  }

  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  
  // try again every 30000ms if connection has failed
  webSocket.setReconnectInterval(30000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets
}

void loop() {
  ArduinoOTA.handle();
  
  webSocket.loop();
  
  if(isConnected) {
      uint64_t now = millis();
      
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");          
      }
  }   
    boolean buttonStateOne = digitalRead(CONTACT_ONE);
    if (buttonStateOne == LOW) {
      if(lightStateOne == true) {
        lightsOff(RELAY_ONE, LED_ONE, DEVICE_ID1, true);
      } else {
         lightsOn(RELAY_ONE, LED_ONE, DEVICE_ID1, true);
      }
      //affect a debounce - keep momentary switch looping multiple times/sec
      delay(333);
    }

    boolean buttonStateTwo = digitalRead(CONTACT_TWO);
    if (buttonStateTwo == LOW) {
      if(lightStateTwo == true) {
        lightsOff(RELAY_TWO, LED_TWO, DEVICE_ID2, true);
      } else {
         lightsOn(RELAY_TWO, LED_TWO, DEVICE_ID2, true);
      }
      //affect a debounce - keep momentary switch looping multiple times/sec
      delay(333);
    }
}

void lightsOn(uint8_t relayPin, uint8_t ledPin, String deviceId, boolean notifyServer) {
    Serial.print("Switch ");
    Serial.print(deviceId);
    Serial.println(" turn on ...");
    digitalWrite(ledPin, LOW);
    digitalWrite(relayPin, LOW);
    if (deviceId == DEVICE_ID1) { lightStateOne=true; } 
    if (deviceId == DEVICE_ID2) { lightStateTwo=true; }
    if (notifyServer) { setPowerStateOnServer(deviceId, "ON"); }
}

void lightsOff(uint8_t relayPin, uint8_t ledPin, String deviceId, boolean notifyServer) {
    Serial.print("Switch ");
    Serial.print(deviceId);
    Serial.println(" turn off ...");
    digitalWrite(ledPin, HIGH);
    digitalWrite(relayPin, HIGH);
    if (deviceId == DEVICE_ID1) { lightStateOne=false; } 
    if (deviceId == DEVICE_ID2) { lightStateTwo=false; }
    if (notifyServer) { setPowerStateOnServer(deviceId, "OFF"); }
}

// If you are going to use a push button to on/off the switch manually, use this function to update the status on the server
// so it will reflect on Alexa app.
// eg: setPowerStateOnServer("deviceid", "ON")

// Call ONLY If status changed. DO NOT CALL THIS IN loop() and overload the server. 

void setPowerStateOnServer(String deviceId, String value) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["deviceId"] = deviceId;
  root["action"] = "setPowerState";
  root["value"] = value;
  StreamString databuf;
  root.printTo(databuf);
  
  webSocket.sendTXT(databuf);
}

//eg: setPowerStateOnServer("deviceid", "CELSIUS", "25.0")

// Call ONLY If status changed. DO NOT CALL THIS IN loop() and overload the server. 

void setTargetTemperatureOnServer(String deviceId, String value, String scale) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["action"] = "SetTargetTemperature";
  root["deviceId"] = deviceId;
  
  JsonObject& valueObj = root.createNestedObject("value");
  JsonObject& targetSetpoint = valueObj.createNestedObject("targetSetpoint");
  targetSetpoint["value"] = value;
  targetSetpoint["scale"] = scale;
   
  StreamString databuf;
  root.printTo(databuf);
  
  webSocket.sendTXT(databuf);
}

