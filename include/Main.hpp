#include "Secrets.hpp"

// Hardware
#include <Wire.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <FS.h>

// I2C
#include <Adafruit_BME280.h>
#include <Adafruit_SGP30.h>

// MQTT
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// NTP
#include <NTPClient.h>
#include <ESP32Time.h>
#include <WiFiUdp.h>

// Upload interval in seconds
const short UPLOAD_INTERVAL_DAY = 600;
const short UPLOAD_INTERVAL_NIGHT = 3600;

// Overnight Sleep Configuration
// https://onlinetools.com/time/calculate-seconds-since-midnight

const long SLEEP_START = 79200;
const short SLEEP_END = 18000;

// Epoch configuration in seconds
const long MIDNIGHT = 86400;

// NTP configuration in seconds
const long NTP_INTERVAL = 172800;
const char NTP_TIMEOUT = 10;

// SGP30 measurement timeout in seconds
const short SGP30_TIMEOUT = 300;

// Soil Moisture
const char SMS_A = 33;
const char SMS_B = 32;

Adafruit_BME280 BME280;
Adafruit_SGP30 SGP30;

WiFiClientSecure WIFI_TLS_CLIENT;
WiFiUDP WIFI_UDP_CLIENT;

PubSubClient MQTT_CLIENT(WIFI_TLS_CLIENT);
NTPClient NTP_CLIENT(WIFI_UDP_CLIENT);

Preferences PREFS;
ESP32Time RTC;

struct StatusRegister
{
    bool NTP;

    bool BME280;
    bool SGP30;
};

StatusRegister* status;
