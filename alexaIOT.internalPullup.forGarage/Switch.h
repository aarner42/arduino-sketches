#ifndef SWITCH_H
#define SWITCH_H
 
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUDP.h>
#include "CallbackFunction.h"

 
class Switch {
private:
        ESP8266WebServer *server = NULL;
        WiFiUDP UDP;
        String serial;
        String persistent_uuid;
        String device_name;
        unsigned int localPort;
        unsigned int state;
        CallbackFunction onCallback;
        CallbackFunction offCallback;
    
        void startWebServer();
        void handleEventservice();
        void handleUpnpControl();
        void handleRoot();
        void handleSetupXml();
        void handleCheckDoorStatus();
        void handleToggleDoor();
public:
        Switch();
        Switch(String alexaInvokeName, unsigned int port, CallbackFunction onCallback, CallbackFunction offCallback);
        ~Switch();
        String getAlexaInvokeName();
        void serverLoop();
        void respondToSearch(IPAddress& senderIP, unsigned int senderPort);
        void setDoorState(unsigned int doorState);
};
 
#endif
