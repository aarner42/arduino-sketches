#ifndef SINRICSWITCH_H
#define SINRICSWITCH_H

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

 
#include <Arduino.h>
#include <WebSocketsClient.h> 
#include <ArduinoJson.h> 

#include <ESP8266WebServer.h>
#include <StreamString.h>

#include "CallbackFunction.h"

 
class SinricSwitch {
private:
        ESP8266WebServer *server = NULL;
        String deviceID;
        boolean powerState;
        boolean connectedToSinric;
        CallbackFunction toggleCallback;
        CallbackFunction alertCallback;
        CallbackFunction resetCallback;
        WebSocketsClient webSocket;
        uint64_t heartbeatTimestamp;
        uint64_t pingTimeStamp;
        void startWebServer(unsigned int localPort);
        void startSinricClient(const char* apiKey);
        void handleRoot();
        void handleReset();
        void sinricOn(String id);
        void sinricOff(String id);
        void sinricLoop();
        void webLoop();
        void setPowerStateOnServer(String value);
public:
        SinricSwitch();
        SinricSwitch(const char* api_key, String device_id, unsigned int port, CallbackFunction toggleCB, CallbackFunction alertCB, CallbackFunction resetCallback);
        ~SinricSwitch();
        void loop();
        void setPowerState(boolean newState);
        boolean getPowerState();
        void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
};
 
#endif
