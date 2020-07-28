#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>

int update_interval = 5000;
// ---------flow sensor pin setup-----------//
byte sensorInterrupt = 0; // 0 = digital pin 2
byte sensorPin = 2;

/********************************************************************/
// Data wire is plugged into pin 5 on the Arduino
int ONE_WIRE_PIN =  6;

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;

/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_PIN);
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
/********************************************************************/

void setup()
{
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);

  // Start up the sensor library for water temp DS18b20
  sensors.begin();

  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  oldTime = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}

void loop()
{
  DynamicJsonDocument json(1024);
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus - DS18b20
  /********************************************************************/
  sensors.requestTemperatures(); // Send the command to get temperature readings
  /********************************************************************/
  json["water"]["temp"] = sensors.getTempCByIndex(0);
  delay(update_interval);

  //------------flow rate-----------
  if ((millis() - oldTime) > update_interval) // Only process counters once per second
  {
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((update_interval / (millis() - oldTime)) * pulseCount) / calibrationFactor;

    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

    unsigned int frac;

    // Print the flow rate for this second in litres / minute
    json["water"]["flowRate"] = int(flowRate);
    
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
  Serial.println(serializeJson(json, Serial));
}

/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
