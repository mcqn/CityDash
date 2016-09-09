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
const int kButtonPin = 2;
unsigned long gButtonPressedTime;
int gLastButtonState = HIGH;
const int kButtonDebounce = 50;

// Status LED details
const int kFaultLEDPin = 9;
const int kFault2LEDPin = 6;
const int kPendingLEDPin = 10;
const int kPending2LEDPin = 11;

// CityDash message globals
const byte kCityDashStatusOk = 0;
const byte kCityDashStatusPending = 1;
const byte kCityDashStatusFault = 2;
// How often we poll for downstream messages
const unsigned long kMessagePollingInterval = 30000UL;
unsigned long gLastMessageTime = 0;

byte gCurrentStatus = kCityDashStatusOk;

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
  
  // Set up status LEDs
  pinMode(kFaultLEDPin, OUTPUT);
  pinMode(kFault2LEDPin, OUTPUT);
  pinMode(kPendingLEDPin, OUTPUT);
  pinMode(kPending2LEDPin, OUTPUT);
  // Start with both status LEDs on, until we've joined the network
  digitalWrite(kFaultLEDPin, HIGH);
  digitalWrite(kFault2LEDPin, HIGH);
  digitalWrite(kPendingLEDPin, HIGH);
  digitalWrite(kPending2LEDPin, HIGH);

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
  // We're online now, default to both LEDs off
  digitalWrite(kFaultLEDPin, LOW);
  digitalWrite(kFault2LEDPin, LOW);
  digitalWrite(kPendingLEDPin, LOW);
  digitalWrite(kPending2LEDPin, LOW);
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
    gCurrentStatus = kCityDashStatusPending;
    bytesReceived = gTTN.sendBytes(&gCurrentStatus, sizeof(gCurrentStatus));
    // Remember when we last sent something
    gLastMessageTime = millis();
    // Let the user know we're on the case
    digitalWrite(kPendingLEDPin, HIGH);
  }
  gLastButtonState = buttonState;  

  if (millis() - gLastMessageTime > kMessagePollingInterval)
  {
    // We haven't checked for incoming messages for a while
    // Send our current status, which will provide a retry
    // mechanism for any lost packets too
    Serial.println("Polling for messages");
    bytesReceived = gTTN.sendBytes(&gCurrentStatus, sizeof(gCurrentStatus));
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
      // Ignore Ok status if we've just reported a fault
      if (gCurrentStatus != kCityDashStatusPending)
      {
        digitalWrite(kPendingLEDPin, LOW);
        digitalWrite(kFaultLEDPin, LOW);
        gCurrentStatus = kCityDashStatusOk;
      }
      break;
    case kCityDashStatusPending:
      // We shouldn't ever receive this status from the server.
      // Light up both LEDs to show the bug
      digitalWrite(kFaultLEDPin, HIGH);
      digitalWrite(kPendingLEDPin, HIGH);
      gCurrentStatus = kCityDashStatusPending;
      break;
    case kCityDashStatusFault:
      digitalWrite(kFaultLEDPin, HIGH);
      digitalWrite(kPendingLEDPin, LOW);
      gCurrentStatus = kCityDashStatusFault;
      break;
    };
  }
}
