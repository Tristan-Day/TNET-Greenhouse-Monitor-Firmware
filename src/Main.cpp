#include "Main.hpp"

uint8_t StatusRegister::dump()
{
    uint8_t status = WIRE;

    status += (WIFI << 1) + (MQTT << 2);
    status += (BME280 << 3) + (SGP30 << 4);

    return status;
}

void awaitNextReading()
{
    esp_sleep_enable_timer_wakeup(UPLOAD_INTERVAL * 1000 * 1000);
    esp_deep_sleep_start();
}

void uploadDataPacket(JsonDocument packet)
{
#ifdef DEBUG
    Serial.println("[INFO] WIFI INIT");
#endif

    WiFi.begin(Secrets::WIFI_SSID, Secrets::WIFI_PASSWORD);

    WIFI_CLIENT.setPrivateKey(Secrets::PRIVATE_KEY);
    WIFI_CLIENT.setCACert(Secrets::ROOT_CERTIFICATE);
    WIFI_CLIENT.setCertificate(Secrets::CLIENT_CERTIFICATE);

    // Wait for the connection to establish
    for (int i = 0; i <= WIFI_TIMEOUT * 10; i++)
    {
        if (WiFi.isConnected())
        {
            status->WIFI = true;
            break;
        }
        delay(100);
    }

    if (!status->WIFI)
    {
        Serial.println("[ERR] WIFI CONN FAIL");
        awaitNextReading();
    }

#ifdef DEBUG
    Serial.println("[INFO] MQTT CONN INIT");
#endif

    MQTT_CLIENT.setServer(Secrets::MQTT_BROKER_DOMAIN, Secrets::MQTT_BROKER_PORT);
    status->MQTT = MQTT_CLIENT.connect(Secrets::MQTT_CLIENT_ID);

    if (!status->MQTT)
    {
        Serial.printf("[ERR] MQTT CONN FAIL: %i", MQTT_CLIENT.state());
        awaitNextReading();
    }

    String data;
    serializeJson(packet, data);

#ifdef DEBUG
    serializeJson(packet, Serial);
#endif

    MQTT_CLIENT.publish(Secrets::MQTT_CLIENT_TOPIC, data.c_str());
    delay(sizeof(data));
}

void setup()
{
#ifdef DEBUG
    Serial.begin(115200);
    Serial.println("[INFO] HARDWARE INIT");
#endif

    status = new StatusRegister();

    status->WIRE = Wire.begin(23, 19);
    status->SPIFFS = SPIFFS.begin();

    status->BME280 = BME280.begin(0x76);
    BME280.setSampling(Adafruit_BME280::MODE_FORCED);

#ifdef INCLUDE_SGP30
    status->SGP30 = SGP30.begin();
#endif

    // Configure moisture sensors
    pinMode(SMS_A, INPUT);
    pinMode(SMS_B, INPUT);

    if (!status->SPIFFS)
    {
        Serial.println("[ERR] SPIFFS FAILURE");
        awaitNextReading();
    }

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

    JsonDocument packet;

#ifdef INCLUDE_SGP30

    if (status->SGP30)
    {
        // Wait for the sensor to warmup
        for (int i = 0; i <= SGP30_TIMEOUT; i++)
        {
            SGP30.IAQmeasure();
            delay(1000);

            if (SGP30.eCO2 != 400 && SGP30.TVOC != 0)
            {
                packet["CarbonDioxide"] = String(SGP30.eCO2);
                break;
            }
        }
    }

    // Power down the device
    SGP30.softReset();
#endif

    status->BME280 = BME280.takeForcedMeasurement();

    if (status->BME280)
    {
        packet["Temperature"] = String(BME280.readTemperature());
        packet["Humidity"] = String(BME280.readHumidity());
        packet["Pressure"] = String(BME280.readPressure());
    }
    else
    {
#ifdef DEBUG
        Serial.println("[ERR] BME280 ERROR");
#endif
    }

    packet["SoilMoisturePrimary"] = String(analogRead(SMS_A));
    packet["SoilMoistureSecondary"] = String(analogRead(SMS_B));

#ifdef DEBUG
    Serial.println("[INFO] SENDING DATA");
#endif

    uploadDataPacket(packet);

#ifdef DEBUG
    Serial.println("[INFO] SLEEPING");
#endif

    awaitNextReading();
}

void loop(){};