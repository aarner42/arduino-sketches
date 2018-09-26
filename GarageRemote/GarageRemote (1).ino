#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>

/*
* Arduino Wireless Communication Tutorial
*       Example 1 - Receiver Code
*                
* by Dejan Nedelkovski, www.HowToMechatronics.com
* 
* Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/

RF24 radio(7, 8); // CE, CSN
const byte address[][6] = {"00001", "00002"};
const int openIndicatorLED = 2;
const int closeIndicatorLED = 3;
const int relayIndicatorLED=4;
const int relayActuatePin = 5;

const int openSensorPin = 9;
const int closeSensorPin = 10;
char text[32] = "";
  
void setup() {
  Serial.begin(9600);
  Serial.println("***** SKETCH START ********");
  
  pinMode(openIndicatorLED,  OUTPUT);
  pinMode(closeIndicatorLED, OUTPUT);
  pinMode(relayIndicatorLED, OUTPUT);
  pinMode(relayActuatePin,   OUTPUT);
    
  pinMode(openSensorPin, INPUT);
  pinMode(closeSensorPin, INPUT);
  
  radio.begin();
  radio.openReadingPipe(0, address[0]);
  radio.openWritingPipe(address[1]);
  radio.setPALevel(RF24_PA_HIGH);
  radio.startListening();
}

void loop() {

  if (radio.available()) {
    radio.read(&text, sizeof(text));
  }

   //determine door state
   boolean isOpen = checkState(openSensorPin, openIndicatorLED);
   boolean isClosed = checkState(closeSensorPin, closeIndicatorLED);
  
   String state = "Door State: ";
   if (isOpen)   
      state += "OPEN";
         
   if (isClosed)   
        state += "CLOSED";
              
   if (!(isOpen || isClosed)) 
        state += "TRAVELING";

   Serial.print(state);
   Serial.print(" RF: ");
   Serial.println(text);
   
    if (strcmp(text,"CheckState")==0) {
       Serial.println("Responding to Interrogation request");
       memset(&text[0], 0, sizeof(text));
       radio.stopListening();
       char reply[32] = "";
       int replyLen = state.length() + 1;
       state.toCharArray(reply, replyLen);
       radio.write(&reply, sizeof(reply));
       delay(10);
       radio.startListening();
    }
    if (strcmp(text,"Cycle")==0) {
       Serial.println("Received Cycle (open/close) command");
       memset(&text[0], 0, sizeof(text));
       digitalWrite(relayIndicatorLED, HIGH);
       digitalWrite(relayActuatePin,   HIGH);
       delay(500);
       digitalWrite(relayActuatePin,   LOW);
       digitalWrite(relayIndicatorLED, LOW);
    }

    memset(&text[0], 0, sizeof(text));


   delay(1000);
}

boolean checkState(int sensorPin, int monitorPin) {
    // check if the proximitySensor is closed:
  if (digitalRead(sensorPin) == LOW) {
    // turn LED on:
    digitalWrite(monitorPin, HIGH);
    return true;
  } else {
    // turn LED off:
    digitalWrite(monitorPin, LOW);
    return false;
  }
}
