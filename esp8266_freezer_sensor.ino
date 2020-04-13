#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal_PCF8574.h>
#include <OneWire.h>
#include <PubSubClient.h>
#include <Wire.h>

#define DEBUG  // uncomment to remove serial debug printing
#include "debug.h"

#define ONE_WIRE_BUS 14  // DS18B20 sensors (a 4.7K pullup is necessary)
#define TEMPERATURE_PRECISION 9
#define MQTT_MSG_BUFFER_SIZE  (50)

const unsigned long MEASUREMENT_INTERVAL = 15 * 1000;  // 15 seconds
const unsigned long PUBLISH_INTERVAL = 2 * 60 * 1000;  // 2 minutes

// DS18B20
DeviceAddress fridgeSensor = { 0x28, 0x0E, 0x53, 0x16, 0xA8, 0x01, 0x3C, 0xED};
DeviceAddress freezerSensor = { 0x28, 0x2B, 0x2A, 0x16, 0xA8, 0x01, 0x3C, 0x2E};
float fridgeMeasurement = -273.15;
float freezerMeasurement = -273.15;
unsigned long previousMeasurementMillis = MEASUREMENT_INTERVAL; // begin measuring immediately
unsigned long previousPublishMillis = 0;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// LCD
LiquidCrystal_PCF8574 lcd(0x27);
char lcdData[16];

// WiFi
const char* ssid = "";
const char* password = "";
WiFiClient espClient;

// MQTT
PubSubClient client(espClient);
char mqttMessage[MQTT_MSG_BUFFER_SIZE];
const char* mqttServer = "";
const char* mqttUsername = "";
const char* mqttPassword = "";
const char* mqttFridgeStateTopic = "homeassistant/sensor/sensorFridgeTemperature/state";
const char* mqttFreezerStateTopic = "homeassistant/sensor/sensorFreezerTemperature/state";

void setupDallasSensors() {
  sensors.begin();
  sensors.setResolution(fridgeSensor, TEMPERATURE_PRECISION);
  sensors.setResolution(freezerSensor, TEMPERATURE_PRECISION);
}

void setupSerial() {
  #ifdef DEBUG
  Serial.begin(115200);
  delay(10);
  Serial.println();
  #endif
}

void setupLCD() {
  lcd.begin(16, 2);  // 16x2 LCD
  lcd.setBacklight(255);
}

void setupWifi() {
  delay(10);
  DEBUG2("Connecting to ", ssid);
  WiFi.mode(WIFI_STA);  // make sure ESP8266 does not advertise an AP
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  randomSeed(micros());
  DEBUG("WiFi connected");
  DEBUG2("IP address: ", WiFi.localIP());
}

void setupMQTT() {
  client.setServer(mqttServer, 1883);
}

void reconnectMQTT() {
  // Loop until we're reconnected
  // TODO: LCD stops updating if MQTT connection is lost - make this "async" with millis intervals
  while (!client.connected()) {
    DEBUG("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUsername, mqttPassword)) {
      DEBUG("connected");
    } else {
      DEBUG3("failed, rc=", client.state(), "try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishMeasurementsMQTT() {
  if (fridgeMeasurement > DEVICE_DISCONNECTED_C) {
    snprintf(mqttMessage, MQTT_MSG_BUFFER_SIZE, "%0.1f", fridgeMeasurement);
    DEBUG4("Publish message:", mqttMessage, "in topic:", mqttFridgeStateTopic);
    client.publish(mqttFridgeStateTopic, mqttMessage);
  }
  if (freezerMeasurement > DEVICE_DISCONNECTED_C) {
    snprintf(mqttMessage, MQTT_MSG_BUFFER_SIZE, "%0.1f", freezerMeasurement);
    DEBUG4("Publish message:", mqttMessage, "in topic:", mqttFreezerStateTopic);
    client.publish(mqttFreezerStateTopic, mqttMessage);
  }
}

void lcdPrintMeasurements() {
  if (fridgeMeasurement > DEVICE_DISCONNECTED_C) {
    snprintf(lcdData, 16, "Fridge:  %.1f", fridgeMeasurement);
  } else {
    snprintf(lcdData, 16, "Fridge:  ...");
  }
  lcd.setCursor(0, 0);
  lcd.print(lcdData);
  
  if (freezerMeasurement > DEVICE_DISCONNECTED_C) {
    snprintf(lcdData, 16, "Freezer: %.1f", freezerMeasurement);
  } else {
    snprintf(lcdData, 16, "Freezer:  ...");
  }
  lcd.setCursor(0, 1);
  lcd.print(lcdData);
}

float getTemperature(DeviceAddress deviceAddress) {
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC == DEVICE_DISCONNECTED_C) {
    DEBUG("Error: Could not read temperature data");
  }
  DEBUG2("Temp C: ", tempC);
  return tempC;
}

void setup() {
  setupSerial();
  setupDallasSensors();
  setupWifi();
  setupMQTT();
  setupLCD();
}

void loop() {
  // MQTT connection keepalive
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // measure
  unsigned long currentMillis = millis();
  if ((unsigned long)(currentMillis - previousMeasurementMillis) >= MEASUREMENT_INTERVAL) {
    sensors.requestTemperatures();
    fridgeMeasurement = getTemperature(fridgeSensor);
    freezerMeasurement = getTemperature(freezerSensor);
    lcdPrintMeasurements();
    previousMeasurementMillis = millis();
  }

  // publish
  currentMillis = millis();
  if ((unsigned long)(currentMillis - previousPublishMillis) >= PUBLISH_INTERVAL) {
    publishMeasurementsMQTT();
    previousPublishMillis = millis();
  }
}
