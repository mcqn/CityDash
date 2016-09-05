// CityDashButton
//
// (c) Copyright 2016 MCQN Ltd
//
// A simple sketch to register button presses on The Things Network
// and show a light when they're awaiting service

#include "SoftwareSerial.h"
#include "TheThingsNetwork.h"
// Include the file containing our Application ID and key
#include "Config.h"

// "Take a reading" button details
// Expects a button which will pull the pin to ground when pressed
const int kButtonPin = 6;
unsigned long gButtonPressedTime;
int gLastButtonState = HIGH;
const int kButtonDebounce = 50;

// Status LED details
const int kLEDPin = 7;


// CityDash message globals
const int kCityDashStatusOk = 0;
const int kCityDashStatusPending = 1;
const char* kCityDashMsgButtonPressed = "1";
const char* kCityDashMsgNOP = "0";
// How often we poll for downstream messages
const unsigned long kMessagePollingInterval = 30000UL;
unsigned long gLastMessageTime = 0;


// LoRaWAN network details

// RN2483 pins on the sensor dev shield
const int kRN_Rx = 3;
const int kRN_Tx = 4;
const int kRN_Reset = 5;
SoftwareSerial gLoRaSerial(kRN_Rx, kRN_Tx);

TheThingsNetwork gTTN;

void setup() {
  Serial.begin(9600);
  Serial.println("Let's go!");

  // Configure button
  pinMode(kButtonPin, INPUT_PULLUP);
  
  // Set up status LED
  pinMode(kLEDPin, OUTPUT);
  digitalWrite(kLEDPin, LOW);

  // Set up LoRaWAN module
  gLoRaSerial.begin(57600);
  pinMode(kRN_Reset, OUTPUT);
  digitalWrite(kRN_Reset, HIGH);

  delay(3000);
  gTTN.init(gLoRaSerial, Serial);
  gTTN.reset();
//  gTTN.personalize(devAddr, nwkSKey, appSKey);
  while (!gTTN.join(gAppEui, gAppKey))
  {
    Serial.println("Failed to join.  Retrying...");
    delay(6000);
  }
  gTTN.showStatus();

  Serial.println("Set up on the Things Network.");

  delay(1000);
}

void loop() {
  // We haven't received any bytes of data this time round
  // This will get updated by us receiving a message if we happen
  // to send something out this time round the loop.  Either
  // because the button was pressed or we polled for messages
  int bytesReceived = 0;
  
  // Check for the button being pressed
  int buttonState = digitalRead(kButtonPin);
  if ((buttonState == LOW) && (gLastButtonState != LOW))
  {
    // Button has been pressed, start timing when that happened
    gButtonPressedTime = millis();
  }
  else if ( (buttonState == HIGH) && 
            (gLastButtonState == LOW) &&
            ((millis() - gButtonPressedTime) > kButtonDebounce) )
  {
    // Button has been released (and was down long enough to be
    // a proper button press)
    Serial.println("Button pressed!");
    // Report the button press to the Things Network
    bytesReceived = gTTN.sendString(kCityDashMsgButtonPressed);
    // Remember when we last sent something
    gLastMessageTime = millis();
  }
  gLastButtonState = buttonState;  

  if (millis() - gLastMessageTime > kMessagePollingInterval)
  {
    // We haven't checked for incoming messages for a while
    // so send a NOP message
    Serial.println("Polling for messages");
    bytesReceived = gTTN.sendString(kCityDashMsgNOP);
    gLastMessageTime = millis();
  }

  // See if we happened to receive a message
  if (bytesReceived > 0)
  {
    Serial.print("Receive a message ");
    Serial.print(bytesReceived);
    Serial.println(" bytes long");
    // Read in the message
    for (int i = 0; i < bytesReceived; i++) {
      Serial.print(String(gTTN.downlink[i]) + " ");
    }
    Serial.println();
    // The first byte is our CityDash status
    switch (gTTN.downlink[0])
    {
    case kCityDashStatusOk:
      digitalWrite(kLEDPin, LOW);
      break;
    case kCityDashStatusPending:
      digitalWrite(kLEDPin, HIGH);
      break;
    };
  }
}
