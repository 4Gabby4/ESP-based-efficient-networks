/*
  Name: Topology based on LoRa communication - Sender
  Year: 2018/2019
  Author: Gabriela Chmelarova
  Purpose: LoRa Communication between reciever and multiple senders
  Credits: this work was inspired on following projects
   1) LoRa communication - https://github.com/dragino/Arduino-Profile-Examples/tree/master/libraries/Dragino/examples/LoRa/multi-nodes-with-temperature-sensor
   2) Deep-sleep usage - https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/
*/

#include <SPI.h>
#include <RH_RF95.h>
#include <String.h>

#include "Adafruit_Sensor.h"
#include "DHT.h"

#include <WiFi.h>

#include <esp_wifi.h>
#include <esp_bt.h>

#include "esp_deep_sleep.h"

#define RFM95_CS 18
#define RFM95_RST 9
#define RFM95_INTERR 26

RH_RF95 rf95(RFM95_CS, RFM95_INTERR);
float frequency = 868.0; // Change the frequency here. 

/* define raindrop sensor pins */
// lowest and highest sensor readings:
const int sensorMin = 600;   // sensor minimum
const int sensorMax = 5000;  // sensor maximum

int analogPin = 14; // select the input pin for raindrop sensor

/* Variable for storing old measured value */
RTC_DATA_ATTR float oldVal = 0;

int sensorReading;
int range;

String mac;
String deviceMac;

/* Deep sleep variables */
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10       /* Time ESP32 will go to sleep (in seconds) */

/* Variables for measuring time */
unsigned long bootMs, setupMs, sendMs, sentMs, delMs;

/* function prototypes */
void InitDHT(); // Initiate DHT11
void ReadDHT(); // Read Temperature and Humidity value 
byte read_dht_dat();

/* Struct for storing recieved LoRa message */
struct __attribute__((packed)) SENSOR_DATA {
  char  deviceType;
  char  type;
  float value;
  char  devMac[6] = {};
} sensorData;

/* function prototypes */
void readSensor();

void setup() {
  setupMs = millis();
  Serial.begin(9600); Serial.println();
  while (!Serial) ; // Wait for serial port to be available
  Serial.println("LoRa_Simple_Client_DHT11");

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup LoRa to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

  // read the sensor on analog A0:
  sensorReading = analogRead(analogPin);
  // map the sensor range (four options):
  // ex: 'long int map(long int, long int, long int, long int, long int)'
	range = map(sensorReading, sensorMin, sensorMax, 0, 3);

  Serial.println(range);
  Serial.println(oldVal);

  float diff = fabs(sensorData.value - oldVal);

  if (range == oldVal) {
    Serial.println("Old value didn't change");
    Serial.println("Going to sleep now");
    Serial.printf("Boot: %lu ms, setup: %lu ms, send: %lu ms, sent: %lu ms, delivery: %lu ms, now %lu ms, going to sleep for %i secs...\n", bootMs, setupMs, sendMs, sentMs, delMs, millis(), TIME_TO_SLEEP);
    Serial.flush();
    rf95.sleep();
    esp_deep_sleep_start();
  }
  oldVal = range;

  if (!rf95.init())
    Serial.println("init failed");

  Serial.println("Raindrop sensor\n\n");

  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);

  sensorData.deviceType = 'l';
  sensorData.type = 'r';

  mac = WiFi.macAddress();
  deviceMac = "";
  deviceMac += String(mac[0], HEX);     
  deviceMac += String(mac[1], HEX);
  deviceMac += String(mac[2], HEX);
  deviceMac += String(mac[3], HEX);
  deviceMac += String(mac[4], HEX);
  deviceMac += String(mac[5], HEX);

  for (size_t i = 0; i < 6; i++) {
    sensorData.devMac[i] = deviceMac[i];
  }

  readSensor();
  sendMs = millis();
  rf95.send((uint8_t*)&sensorData, sizeof(sensorData)); // Send out ID + Sensor data to LoRa gateway
  sentMs = millis();
  Serial.printf("Device type=%c, ValueType=%c, value=%.2f\n", sensorData.deviceType, sensorData.type, sensorData.value);
  
  Serial.print("Device Mac: ");
  for (size_t i = 0; i < 6; i++) {
    Serial.printf("%c", sensorData.devMac[i]);
  }
  Serial.println();

  // after message is send we start deep sleep
  Serial.println("Going to sleep now");
  Serial.printf("Boot: %lu ms, setup: %lu ms, send: %lu ms, sent: %lu ms, delivery: %lu ms, now %lu ms, going to sleep for %i secs...\n", bootMs, setupMs, sendMs, sentMs, delMs, millis(), TIME_TO_SLEEP);
  Serial.flush();
  rf95.sleep();
  esp_deep_sleep_start();
}

void loop() {
  // do nothing
}

/* Function to read sensor value */
void readSensor() {
  // range value:
  switch (range) {
    case 0:    // Sensor getting wet
      Serial.println("Flood");
      break;
    case 1:    // Sensor getting wet
      Serial.println("Rain Warning");
      break;
    case 2:    // Sensor dry - To shut this up delete the " Serial.println("Not Raining"); " below.
      Serial.println("Not Raining");
      break;
  }
  sensorData.value = range;
}