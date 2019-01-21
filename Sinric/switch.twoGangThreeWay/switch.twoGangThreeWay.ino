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
/* CT Sensor consts */
#define CURRENT_FLOW_NONZERO_THRESHOLD 150
#define RESISTANCE 82.0
#define SAMPLING_MSEC 333
#define ZERO_CROSSING -126.2
#define SCALE_FACTOR .25  //compensate for the quadruple-wired coil

uint64_t heartbeatTimestamp = 0;
uint64_t printTimeStamp =0;
uint64_t pingTimeStamp =0;
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
  Serial.print("WebSocketEvent:");
  Serial.println(type);

  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: iot.sincric.com %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");        
      }
      break;
    case WStype_TEXT: {
      isConnected = true;
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
  digitalWrite(RELAY_ONE, LOW);  // shut off the high voltage relay side
  digitalWrite(RELAY_TWO, LOW);  // shut off the high voltage relay side
  
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
  ArduinoOTA.handle();
  
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
        for (int c=0; c<40; c++) {
          digitalWrite(LED, LOW);
          delay(50);
          digitalWrite(LED, HIGH);
          delay(50);
  }   
 }
   //check whether the light is on.  (samples for 250 msec so effectively debounces the momentary switch)
  float currentFlow = getCurrentFlowInAmps(false);
  boolean newState = (currentFlow > CURRENT_FLOW_NONZERO_THRESHOLD);

  if (newState != lightState) {
    Serial.print("Current Flow is presently: ");
    Serial.print(currentFlow,2);
    Serial.print(" light appears to be on: ");
    Serial.println(newState);

  	//notify server only if the state we believe we're in has changed;
    String powerStatus = newState ? "ON" : "OFF";
    setPowerStateOnServer(DEVICE_ID, powerStatus);
  
    lightState = newState;    //could update this outside the loop but if it didn't change, why bother?
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

float getCurrentFlowInAmps(boolean debug) {

  float nVPP;   // Voltage measured across resistor
  float nCurrThruResistorPP; // Peak Current Measured Through Resistor
  float nCurrThruResistorRMS; // RMS current through Resistor
  float nCurrentThruWire;     // Actual RMS current in Wire

   
   nVPP = readCtSensor();
   nCurrThruResistorPP = (nVPP/RESISTANCE) * 1000.0;      //Use Ohms law to calculate current across resistor
   nCurrThruResistorRMS = nCurrThruResistorPP * 0.707;    //Use Formula for SINE wave to convert to RMS 
   nCurrentThruWire = ((nCurrThruResistorRMS * 1000) + ZERO_CROSSING) * SCALE_FACTOR;  //Current Transformer Ratio is 1000:1...scaled by half because current-carrying wire passes thru 4x

   if (debug) {
     Serial.print("Volts Peak : ");
     Serial.println(nVPP,3);
  
     Serial.print("Current Through Resistor (Peak) : ");
     Serial.print(nCurrThruResistorPP,3);
     Serial.println(" mA Peak to Peak");
     Serial.print("Current Through Resistor (RMS) : ");
     Serial.print(nCurrThruResistorRMS,3);
     Serial.println(" mA RMS");
     Serial.print("Current Through Wire : ");
     Serial.print(nCurrentThruWire,3);
     Serial.println(" mA RMS");
   }
   return nCurrentThruWire;
 }


float readCtSensor() {
  float result;
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
   uint32_t start_time = millis();
   while((millis()-start_time) < SAMPLING_MSEC) //sample for some non-trivial time
   {
       readValue = analogRead(A0);
       delay(10);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
   }
   
   // Convert the digital data to a voltage
   result = (maxValue * 5.0)/1024.0;
  
   return result;
}

