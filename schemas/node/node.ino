// Include libraries
#include <DHT.h> // DHT sensor library
#include <LowPower.h> // low power library
#include <RFM69.h> // RFM69 radio library
#include <SPI.h> // SPI library
#include <avr/sleep.h> // sleep library
#include <stdlib.h> // library for maths

// Soil moisture sensor define
#define MOIST_PIN_1 A2 // soil probe pin 1 with 56kohm resistor
#define MOIST_PIN_2 7 // soil probe pin 2 with 100ohm resistor
#define MOIST_READ_PIN_1 A0 // analog read pin. connected to A2 PIN with 56kohm resistor
#define MOIST_CYCLES 3 // how many times to read the moisture level. default is 3 times

// DHT Humidity + Temperature sensor define
#define DHT_PIN 5 // Data pin (D5) for DHT
#define DHT_PWR 4 // turn DHT on and off via transistor
#define DHT_TYPE DHT11 // sensor model DHT11

DHT dht(DHT_PIN, DHT_TYPE); // define DHT11

// Battery meter
#define VOLTAGE_PIN_READ A7 // analog voltage read pin for batery meter
#define VOLTAGE_PIN_READ_ENABLE 3 // current sink pin. ( enable voltage divider )
#define VOLTAGE_REF 3.3 // reference voltage on system. use to calculate voltage from ADC
#define VOLTAGE_DIVIDER 3.02 // if you have a voltage divider to read voltages, enter the multiplier here.

// RADIO SETTINGS
// You will need to initialize the radio by telling it what ID it has and what network it's on
// The Node_ID takes values from 1-127, 0 is reserved for sending broadcast messages (send to all nodes)
// The Network ID takes values from 0-255
#define NODE_ID        20  // The ID of this node. Has to be unique. 1 is reserved for the gateway!
#define NETWORK_ID    10  //the network ID we are on
#define GATEWAY_ID     1  //the gateway Moteino ID (default is 1)
#define FREQUENCY     RF69_433MHZ
#define SECRET "sixteencharacter"

// Power Management Sleep cycles
#define SLEEP_CYCLE_DEFAULT 1 // Sleep cycle 450*8 seconds = 1 hour. DEFAULT 450

RFM69 radio; // Define radio

void setup() {
    // Debug only
    Serial.begin(9600);

    // Battery Meter setup
    pinMode(VOLTAGE_PIN_READ, INPUT);
    pinMode(VOLTAGE_PIN_READ_ENABLE, OUTPUT);
    digitalWrite(VOLTAGE_PIN_READ_ENABLE, LOW);

    // // Moisture sensor pin setup
    // pinMode(MOIST_PIN_1, OUTPUT);
    // pinMode(MOIST_PIN_2, OUTPUT);
    // pinMode(MOIST_READ_PIN_1, INPUT);

    // Humidity sensor setup
    pinMode(DHT_PWR, OUTPUT);
    dht.begin();

    // Radio setup
    radio.initialize(FREQUENCY,NODE_ID,NETWORK_ID);
    radio.setHighPower();
    radio.encrypt(SECRET);
    radio.promiscuous(false);
    radio.sleep();
}

void loop() {
    float voltage;
    readBatteryVoltage(voltage);
    //
    // int moisture;
    // readMoisture(moisture);

    int temperature;
    int humidity;
    readDht(temperature, humidity);

    // transmitMeasures(moisture, temperature, humidity, voltage);
    // printMeasures(moisture, temperature, humidity, voltage, elapsedTime);
    //sleepOneHourMinusOneSecond();
    printDHT(temperature, humidity);
    printBatteryVoltage(voltage);
    sleepEightSeconds();
}

// Battery voltage reading function
// Reads the voltage three times, keeps the last reading
void readBatteryVoltage(float& voltage) {
    int voltageADC;
    // Battery level check
    digitalWrite(VOLTAGE_PIN_READ_ENABLE, HIGH); // turn on the battery meter
    delay(20);
    for ( int i = 0 ; i < 3 ; i++ ) {
        delay(10); // delay, wait for circuit to stabalize
        voltageADC = analogRead(VOLTAGE_PIN_READ); // read the voltage 3 times. keep last reading
    }
    voltage = voltageADC * VOLTAGE_REF / 1023 * VOLTAGE_DIVIDER; // calculate the voltage
    digitalWrite(VOLTAGE_PIN_READ_ENABLE, LOW); // turn off the battery meter
    delay(20);
}


// Moisture sensor reading function
// function reads 3 times and averages the data
void readMoisture(int& moisture) {
    // Soil Moisture sensor reading
    int moistAvgValue = 0; // reset the moisture level before reading
    for ( int moistReadCount = 0; moistReadCount < MOIST_CYCLES; moistReadCount++ ) {
        int moistReadDelay = 88; // delay to reduce capacitive effects
        // polarity 1 read
        digitalWrite(MOIST_PIN_1, HIGH);
        digitalWrite(MOIST_PIN_2, LOW);
        delay (moistReadDelay);
        int moistVal1 = analogRead(MOIST_READ_PIN_1);
        digitalWrite(MOIST_PIN_1, LOW);
        delay (moistReadDelay);
        // polarity 2 read
        digitalWrite(MOIST_PIN_1, LOW);
        digitalWrite(MOIST_PIN_2, HIGH);
        delay (moistReadDelay);
        int moistVal2 = analogRead(MOIST_READ_PIN_1);
        //Make sure all the pins are off to save power
        digitalWrite(MOIST_PIN_2, LOW);
        digitalWrite(MOIST_PIN_1, LOW);
        moistVal1 = 1023 - moistVal1; // invert the reading
        moistAvgValue += (moistVal1 + moistVal2) / 2; // average readings. report the levels
    }
    moisture = moistAvgValue / MOIST_CYCLES; // average the results
}

// DHT sensor reading function
// Reads the humidity and temperature
void readDht(int& temperature, int& humidity) {
    // Humidity + Temperature sensor reading
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    digitalWrite(DHT_PWR, HIGH); // turn on sensor
    delay (10000); // wait for sensor to stabilize
    temperature = dht.readTemperature(); // read temperature as celsius
    humidity = dht.readHumidity(); // read humidity
    // check if returns are valid, if they are NaN (not a number) then something went wrong!
    if (isnan(temperature) || isnan(humidity)) {
        temperature = 0;
        humidity = 0;
    }
    delay (18);
    digitalWrite(DHT_PWR, LOW); // turn off sensor
}

// Transmit the measures by radio
void transmitMeasures(int moisture, int temperature, int humidity, float voltage) {
    // PREPARE READINGS FOR TRANSMISSION
    String senseData = String(NODE_ID);
    senseData += ":";
    senseData += String(moisture);
    senseData += ":";
    senseData += String(temperature);
    senseData += ":";
    senseData += String(humidity);
    senseData += ":";
    char voltageBufTemp[10];
    dtostrf(voltage,5,3,voltageBufTemp); // convert float Voltage to string
    senseData += voltageBufTemp;
    byte sendSize = senseData.length();
    sendSize = sendSize + 1;
    char sendBuf[sendSize];
    senseData.toCharArray(sendBuf, sendSize); // convert string to char array for transmission
    delay(30);
    //Transmit the data
    radio.send(GATEWAY_ID, sendBuf, strlen(sendBuf), false); // send the data
    radio.sleep(); // sleep the radio to save power
}

// Sleep for one hour
void sleepOneHourMinusOneSecond() {
    // POWER MANAGEMENT DEEP SLEEP
    // after everything is done, go into deep sleep to save power
    int sleepCycle = 449; // Sleep cycle reset
    for ( int sleepCycleCounter = 0; sleepCycleCounter < sleepCycle; sleepCycleCounter++ ) {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); //sleep duration is 8 seconds multiply by the sleep cycle variable.
    }
    for ( int sleepCycleCounter = 0; sleepCycleCounter < 7; sleepCycleCounter++ ) {
        LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF); //sleep duration is 8 seconds multiply by the sleep cycle variable.
    }
}

// Sleep for eight seconds
void sleepEightSeconds() {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

/**
 * Print battery voltage.
 * @param  {float&} voltage       battery voltage
 */
void printBatteryVoltage(float& voltage) {
    Serial.print("Battery voltage : ");
    Serial.println(voltage);
}

// Print DHT measures
void printDHT(int& temperature, int& humidity) {
    Serial.print("Temperature : ");
    Serial.println(temperature);
    Serial.print("Humidity : ");
    Serial.println(humidity);
}

// Send the measures to the serial
void printMeasures(int moisture, int temperature, int humidity, float voltage, unsigned long duration) {
    Serial.println();
    Serial.print("Moisture : ");
    Serial.println(moisture);
    Serial.print("Temperature : ");
    Serial.println(temperature);
    Serial.print("Humidity : ");
    Serial.println(humidity);
    Serial.print("Voltage : ");
    Serial.println(voltage);
    Serial.print("Duration : ");
    Serial.println(duration);
}

