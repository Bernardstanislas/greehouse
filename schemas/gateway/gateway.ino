// Include libraries
#include <RFM69.h> // RFM69 radio library
#include <SPI.h> // SPI library

// RADIO SETTINGS
// You will need to initialize the radio by telling it what ID it has and what network it's on
// The Node_ID takes values from 1-127, 0 is reserved for sending broadcast messages (send to all nodes)
// The Network ID takes values from 0-255
#define NODE_ID        1  // The ID of this node. Has to be unique. 1 is reserved for the gateway!
#define NETWORK_ID    10  //the network ID we are on
#define FREQUENCY     RF69_433MHZ
#define SECRET "sixteencharacter"

RFM69 radio; // Define radio

void setup() {
  // Debug only
  Serial.begin(9600);
  delay(30);
  // Radio setup
  radio.initialize(FREQUENCY,NODE_ID,NETWORK_ID);
  radio.setHighPower();
  radio.encrypt(SECRET);
  radio.promiscuous(false);
}

void loop() {
  if (radio.receiveDone()) // radio finishes recieving data
  {
    Serial.print('[');
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i]);
    Serial.print(":");Serial.print(radio.RSSI);Serial.print("]");
    Serial.println();
  }
}
