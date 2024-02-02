#include "Secrets.hpp"

#include <Wire.h>
#include <SPIFFS.h>
#include <FS.h>

#include <Adafruit_BME280.h>
#include <Adafruit_SGP30.h>

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include <ArduinoJson.h>

// Upload interval in seconds
const short UPLOAD_INTERVAL = 300;

// WiFi timeout in seconds
const char WIFI_TIMEOUT = 30;

// SGP30 measurement timeout in seconds
const char SGP30_TIMEOUT = 60;

// Soil Moisture
const char SMS_A = 33;
const char SMS_B = 32;

Adafruit_BME280 BME280;
Adafruit_SGP30 SGP30;

WiFiClientSecure WIFI_CLIENT;
PubSubClient MQTT_CLIENT(WIFI_CLIENT);

struct StatusRegister
{
    bool WIRE;
    bool SPIFFS;

    bool WIFI;
    bool MQTT;

    bool BME280;
    bool SGP30;

    uint8_t dump();
};

StatusRegister* status;
