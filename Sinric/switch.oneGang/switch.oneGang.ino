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
#define DEVICE_ID "5c3826353d4dd357f795d360"  // deviceId is the ID assgined to your smart-home-device in sinric.com dashboard. Copy it from dashboard and paste it here
#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 
#define LAN_HOSTNAME  "AlphaBravoTwo"
/************ define pins *********/
#define RELAY   D1   // D1 drives 120v relay
#define CONTACT D3   // D3 connects to momentary switch
#define LED D4       // D4 powers switch illumination

uint64_t heartbeatTimestamp = 0;
uint64_t printTimeStamp =0;
uint64_t pingTimeStamp =0;

bool isConnected = false;
boolean lightState = false;

void setPowerStateOnServer(String deviceId, String value);
void lightsOn(boolean notifyServer);  
void lightsOff(boolean notifyServer);

void turnOn(String deviceId) {
  if (deviceId == DEVICE_ID) // Device ID of first device
  {  
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    lightsOn(false);
  } else {
    Serial.print("Turn on for unknown device id: ");
    Serial.println(deviceId);    
  }     
}

void turnOff(String deviceId) {
   if (deviceId == DEVICE_ID) // Device ID of first device
   {  
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
     lightsOff(false);
   } else {
     Serial.print("Turn off for unknown device id: ");
     Serial.println(deviceId);    
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  Serial.print("WebSocketEvent:");
  Serial.println(type);
  
  switch(type) {
    case WStype_ERROR: {
      isConnected=false;
      Serial.printf("[WSc] Webservice ERROR!\n");
      }
      break;
    case WStype_FRAGMENT_TEXT_START: {
      isConnected=true;
      Serial.printf("[WSc] Webservice Text Fragment begin\n");
    }
    break;
    case WStype_FRAGMENT_BIN_START: {
      isConnected=true;
      Serial.printf("[WSc] Webservice Binary Fragment begin\n");
    }
    break;
    case WStype_FRAGMENT_FIN: {
      isConnected=true;
      Serial.printf("[WSc] Webservice Fragment finished\n");
    }
    break;
    case WStype_FRAGMENT: {
      isConnected=true;
      Serial.printf("[WSc] Webservice Fragment\n");
    }
    break;
    case WStype_DISCONNECTED: {
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      }
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: iot.sincric.com %s\n", payload);
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

  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(CONTACT, INPUT_PULLUP);
  
  digitalWrite(LED, HIGH);   // default switch illumination to on
  digitalWrite(RELAY, LOW);  // shut off the high voltage relay side
  
  Serial.begin(115200);
  
  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);  

  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(150);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
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

  }
  Serial.println("Connecting to sinric.com");
  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/");
 
  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  Serial.println("WebSocket handlers/authorization registered...");
  
  // try again every 30 s if connection has failed
  webSocket.setReconnectInterval(30000);
  webSocket.sendPing();
  Serial.println("Setup complete...");   
}

void loop() {
  webSocket.loop();
  uint64_t time = millis();
  if ((time - printTimeStamp) > 30000) {
    Serial.println("in loop...");
    printTimeStamp = time;
  }
  
  if(isConnected) {
      uint64_t now = millis();
      pingTimeStamp = now;
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now < 10000) || (now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");  
          webSocket.sendPing();        
      }
  } else {
      if (pingTimeStamp == 0 || ((millis() - pingTimeStamp) > 30000)) {
        Serial.println("No connection to Sinric cloud - sending data to force webSocket to realize disconnect happened");
        webSocket.sendPing();
        webSocket.sendTXT("H");      
        pingTimeStamp = millis();
      
        //blink the LED to indicate connection fail
        for (int c=0; c<20; c++) {
          digitalWrite(LED, LOW);
          delay(100);
          digitalWrite(LED, HIGH);
          delay(100);
        }
    }
  }

  boolean buttonState = digitalRead(CONTACT);
  if (buttonState == LOW) {
    if(lightState == true) {
      lightsOff(true);
    } else {
      lightsOn(true);
    }
    //affect a debounce - stop momentary switch looping multiple times/sec
    delay(333);
  }
  ArduinoOTA.handle();
}

void lightsOn(boolean notifyServer) {
    Serial.println("Switch 1 turn on ...");
    digitalWrite(LED, LOW);
    digitalWrite(RELAY, HIGH);
    lightState = true;
    if (notifyServer) { setPowerStateOnServer(DEVICE_ID, "ON"); }
}

void lightsOff(boolean notifyServer) {
    Serial.println("Switch 1 turn off ...");
    digitalWrite(LED, HIGH);
    digitalWrite(RELAY, LOW);
    lightState = false;
    if (notifyServer) { setPowerStateOnServer(DEVICE_ID, "OFF"); }
}


// Call ONLY If status changed manually. DO NOT CALL THIS IN loop() and overload the server. 

void setPowerStateOnServer(String deviceId, String value) {
  Serial.print("Updating status on server.  Setting deviceID:");
  Serial.print(deviceId);
  Serial.print(" new state:");
  Serial.println(value);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["deviceId"] = deviceId;
  root["action"] = "setPowerState";
  root["value"] = value;
  StreamString databuf;
  root.printTo(databuf);
  
  webSocket.sendTXT(databuf);
}


