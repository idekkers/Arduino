#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>

// ---------BME280 setup-----------//
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

void setup()
{
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);

  if (!bme.begin(0x76))
    {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1)
            ;
    }
}

void loop()
{
  //LM393 start analog light read
  unsigned int AnalogValue;
  AnalogValue = analogRead(A0);
// DynamicJsonDocument json(1024);
//
//  //---------BME280-------------
//  json["environment"]["temp"] = bme.readTemperature();
//  json["environment"]["pressure"] = bme.readPressure() / 100.0F;
//  json["environment"]["humidity"] = bme.readHumidity();
//  //-----------LM393--------------
//  json["environment"]["light"] = AnalogValue;

//  Serial.println(serializeJson(json, Serial));
  Serial.print("press");
  Serial.println(bme.readTemperature());
  delay(1500);
}
