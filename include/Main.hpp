#include <Wire.h>
#include <FS.h>

#include <Adafruit_BME280.h>
#include <Adafruit_SGP30.h>

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "Secrets.hpp"

// Upload interval in secconds
const char UPLOAD_INTERVAL = 60;

// WiFi timeout in secconds
const char WIFI_TIMEOUT = 30;

// Soil Moisture
const char SMS_A = 32;
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
