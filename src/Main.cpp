/*
##########################

TNET - Greenhouse Monitor

Revision: 2.3 - 04/03/2024
Author: Tristan Day

https://github.com/Tristan-Day/TNET-Greenhouse-Monitor-Firmware

##########################
*/

#include "Main.hpp"

uint64_t getSleepInterval()
{
    uint32_t now = RTC.getLocalEpoch();
    uint32_t lastSync = PREFS.getULong("LAST-SYNC", NTP_INTERVAL);

    if (now - lastSync > NTP_INTERVAL)
    {
#ifdef DEBUG
        Serial.println("[INFO] RTC RESYNC");
#endif

        // Resynchronise the RTC
        NTP_CLIENT.begin();

        // Wait for Timesync
        for (int i = 0; i < NTP_TIMEOUT * 100; i++)
        {
            status->NTP = NTP_CLIENT.forceUpdate();

            if (status->NTP)
            {
                break;
            }

            delay(100);
        }

        if (status->NTP)
        {
            PREFS.putULong("LAST-SYNC", NTP_CLIENT.getEpochTime());
            RTC.setTime(NTP_CLIENT.getEpochTime());
        }
        else
        {
            Serial.println("[ERR] RTC TIMESYNC FAIL");
            return UPLOAD_INTERVAL_DAY;
        }

#ifdef DEBUG
        Serial.printf("[INFO] TIME: %s\n", RTC.getTimeDate().c_str());
#endif
    }

    // Upload interval logic
    uint32_t timeSinceMidnight =
        RTC.getSecond() + RTC.getMinute() * 60 + RTC.getHour(true) * 3600;

    // Handle daytime interval
    if (SLEEP_END <= timeSinceMidnight && timeSinceMidnight < SLEEP_START)
    {
        return UPLOAD_INTERVAL_DAY;
    }

    uint32_t wakeup = timeSinceMidnight + UPLOAD_INTERVAL_NIGHT;

    // Ensure the interval does not enter daytime hours
    if (SLEEP_END < wakeup && wakeup < SLEEP_START)
    {
        return SLEEP_END - timeSinceMidnight;
    }

    // Handle nighttime interval
    return UPLOAD_INTERVAL_NIGHT;
}

void sleep()
{
    uint64_t interval = getSleepInterval();

    // Convert the time from seconds to microseconds
    interval = interval * 1000 * 1000;

#ifdef DEBUG
    Serial.printf("[INFO] SLEEP DURATION: %u", interval);
#endif

    esp_sleep_enable_timer_wakeup(interval);
    esp_deep_sleep_start();
}

void upload(JsonDocument packet)
{
#ifdef DEBUG
    Serial.println("[INFO] MQTT CONN INIT");
#endif

    MQTT_CLIENT.setServer(Secrets::MQTT_BROKER_DOMAIN, Secrets::MQTT_BROKER_PORT);
    bool MQTT_STATUS = MQTT_CLIENT.connect(Secrets::MQTT_CLIENT_ID);

    if (!MQTT_STATUS)
    {
        Serial.printf("[ERR] MQTT CONN FAIL: %i", MQTT_CLIENT.state());
        sleep();
    }

    String data;
    serializeJson(packet, data);

#ifdef DEBUG
    serializeJson(packet, Serial);
    Serial.write('\n');
#endif

    MQTT_CLIENT.publish(Secrets::MQTT_CLIENT_TOPIC, data.c_str());
    delay(sizeof(data) * 0.5);

#ifdef DEBUG
    Serial.println("[INFO] MQTT CONN END");
#endif

    MQTT_CLIENT.disconnect();
}

void setup()
{  
    Serial.begin(115200);
    PREFS.begin("MAIN");

#ifdef DEBUG
    Serial.println("[INFO] HARDWARE INIT");
    Serial.printf("[INFO] DEVICE ID: %s\n", Secrets::MQTT_CLIENT_ID);
#endif

    status = new StatusRegister();

    if (!Wire.begin(23, 19))
    {
        Serial.println("[ERR] I2C FAILURE");
        sleep();
    }

    if (!SPIFFS.begin())
    {
        Serial.println("[ERR] SPIFFS FAILURE");
        sleep();
    }

#ifndef EXCLUDE_BME280
    status->BME280 = BME280.begin(0x76);
    BME280.setSampling(Adafruit_BME280::MODE_FORCED);

    if (!status->BME280)
    {
        Serial.println("[ERR] BME280 ERROR");
    }
#endif

#ifndef EXCLUDE_SGP30
    status->SGP30 = SGP30.begin();

    if (!status->SGP30)
    {
        Serial.println("[ERR] SGP30 ERROR");
    }
#endif

    // Configure moisture sensors
    pinMode(SMS_A, INPUT);
    pinMode(SMS_B, INPUT);

#ifdef DEBUG
    Serial.println("[INFO] LOADING SECRETS");
#endif

    File file = SPIFFS.open("/secrets/private.key", "r");
    file.readBytes(Secrets::PRIVATE_KEY, 2048);
    file.close();

    file = SPIFFS.open("/secrets/cert.pem.crt", "r");
    file.readBytes(Secrets::CLIENT_CERTIFICATE, 2048);
    file.close();

    file = SPIFFS.open("/secrets/root.crt", "r");
    file.readBytes(Secrets::ROOT_CERTIFICATE, 2048);
    file.close();

#ifdef DEBUG
    Serial.println("[INFO] PREPARING DATA");
#endif

    JsonDocument packet;

#ifndef EXCLUDE_SGP30

    if (status->SGP30)
    {
        // Wait for the sensor to warmup
        for (int i = 0; i <= SGP30_TIMEOUT; i++)
        {
            if (!SGP30.IAQmeasure())
            {
                Serial.println("[ERR] SGP30 READ");
            }
            delay(1000);

            if (SGP30.eCO2 > 450)
            {
                packet["CarbonDioxide"] = String(SGP30.eCO2);
                break;
            }
        }
    }

    // Power down the device
    SGP30.softReset();
#endif

#ifndef EXCLUDE_BME280

    status->BME280 = BME280.takeForcedMeasurement();

    if (status->BME280)
    {
        packet["Temperature"] = String(BME280.readTemperature());
        packet["Humidity"] = String(BME280.readHumidity());
        packet["Pressure"] = String(BME280.readPressure());
    }
    else
    {
        Serial.println("[ERR] BME280 ERROR");
    }

#endif

    packet["SoilMoisturePrimary"] = String(analogRead(SMS_A));
    packet["SoilMoistureSecondary"] = String(analogRead(SMS_B));

#ifdef DEBUG
    Serial.println("[INFO] WIFI INIT");
#endif

    WiFi.begin(Secrets::WIFI_SSID, Secrets::WIFI_PASSWORD);

    WIFI_TLS_CLIENT.setPrivateKey(Secrets::PRIVATE_KEY);
    WIFI_TLS_CLIENT.setCertificate(Secrets::CLIENT_CERTIFICATE);
    WIFI_TLS_CLIENT.setCACert(Secrets::ROOT_CERTIFICATE);

    WiFi.onEvent(
        [](WiFiEvent_t event, WiFiEventInfo_t info) {
            sleep();
        },
        WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    WiFi.onEvent(
        [packet](WiFiEvent_t event, WiFiEventInfo_t info) {
            upload(packet);
            sleep();
        },
        WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
}

void loop(){};