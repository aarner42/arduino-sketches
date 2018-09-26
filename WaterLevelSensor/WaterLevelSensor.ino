
// constants won't change. They're used here to set pin numbers:
const int buttonPin = 4;     // the number of the pushbutton pin
const int ledPin =  13;      // the number of the LED pin
const int relayPin = 10;
const int cycleDelay = 1250;
// variables will change:
int liquidPresent = 0;         // variable for reading the pushbutton status

void setup() {
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  
  // initialize the pushbutton pin as an input and enable internal pull-up resistor:
  pinMode(buttonPin, INPUT_PULLUP);
}


void loop() {
  // read the state of the pushbutton value:
  liquidPresent = digitalRead(buttonPin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (liquidPresent == HIGH) {
    // turn LED off/cycle relay through on/off in 2.5 sec
    digitalWrite(ledPin, LOW);
    digitalWrite(relayPin, LOW);
    delay(cycleDelay);
    digitalWrite(relayPin, HIGH);
    delay(cycleDelay);
  } else {
    // turn LED on/throw relay on
    digitalWrite(ledPin, HIGH);
    digitalWrite(relayPin, LOW);
  }
 
}
