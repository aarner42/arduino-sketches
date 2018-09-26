/*
* Arduino Wireless Communication Tutorial
*     Example 1 - Transmitter Code
*                
* by Dejan Nedelkovski, www.HowToMechatronics.com
* 
* Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
 
#define SLAVE_ADDRESS 0x04
#define CHECK_STATUS 1
#define CYCLE_DOOR 2
RF24 radio(7, 8); // CE, CSN

const byte address[][6] = {"00001", "00002"};
int piResponse = 0;
String response = ""; 
String command = "";
int loopCount =0;  
void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[1]);
  radio.setPALevel(RF24_PA_HIGH);

   // initialize i2c as slave
 Wire.begin(SLAVE_ADDRESS);
 
 // define callbacks for i2c communication
 Wire.onReceive(receiveData);
 Wire.onRequest(sendData);
 
  Serial.println("*****SKETCH STARTUP******");
  
}

void loop() {
  loopCount++;

  //this if-block acts as a sort of command-loss-timer reset, forcing a status check of the door
  if ((loopCount % 3600)==0) {
    command = "CheckState";
    loopCount = 0;
  }
  
  // convert command to char array for signaling:
  int textLen = command.length()+1;
  char chars[textLen];
  command.toCharArray(chars, textLen);
  if (command == "Cycle") {
    radio.stopListening();
    radio.write(&chars, sizeof(chars));
    delay(10);
    command = "";
  }
  if (command == "CheckState")
    response = "";


  //if this was a checkState command, transmit continually until we get a response:
  while (command == "CheckState" && response=="") {
    Serial.print("Sending Query to remote: [");
    Serial.print(chars);
    Serial.println("]");
    
    radio.stopListening();
    radio.write(&chars, sizeof(chars));
    delay(10);
  
    int retry=0;
    Serial.print("RF (#");
    Serial.print(retry);
    Serial.print("-sig:");
    Serial.print(radio.available());
    Serial.print(") read:");
    boolean weGetSignal = 0;
    //try ten read attempts
    while (retry < 8 && weGetSignal==0 && response=="") {
      radio.startListening();
      retry++;
      weGetSignal = radio.available();
      if (weGetSignal) {
//        Serial.println("");
//        Serial.println("*******WEGETSIGNAL*******");
//        Serial.println("");
        char signalBuffer[32] = "";
        radio.read(&signalBuffer, sizeof(signalBuffer));
        Serial.print("*");
        Serial.print(signalBuffer);
        Serial.print("*");
        command="idle";
        response = String(signalBuffer);
      } else {
        delay(250);
        Serial.print("*NOSIG*");
      }
     Serial.print(" ");
     Serial.print(retry);
   }
    Serial.println("] RF Done");
    delay(90);
  }
  Serial.print("Command: [");
  Serial.print(command);
  Serial.print("] Last response: [");
  Serial.print(response);
  if (response == "Door State: OPEN") 
    piResponse = 1;
  else if (response == "Door State: CLOSED")
    piResponse = 2;
  else if (response == "Door State: TRAVELING")
    piResponse = 3;
  else
    piResponse = 4;
  Serial.print("-");
  Serial.print(piResponse);
  Serial.println("]");
   
  delay(500);
    
}

// callback for received data
void receiveData(int byteCount){
  while(Wire.available()) {
    int number = Wire.read();
   
    if (number == CHECK_STATUS){
       command = "CheckState";   
    }
    if (number == CYCLE_DOOR) {
      command = "Cycle";
    }
    Serial.print("Received command from rpi3: ");
    Serial.println(command);
  }
}
 
// callback for sending data
void sendData(){
  Serial.print("Sending state to rpi3: ");
  Serial.println(piResponse);
  Wire.write(piResponse);
}


