#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>

/********************************************************************/

int update_interval = 1000;

/*
 *------------------ pin setup -------------------------------
 ! error color
 ? question color
 TODO color
 @param param
 */
/*
 *--------- PH pin setup --------------------
 */
char PHSensorPin = A0;

/*
 *--------- TDS pin setup --------------------
 */
char TDSSensorPin = A2;

/*
 *--------- Light sensor pin setup ----------
 */
char LightSensorPin = A1;

/*
 *--------- Water pump relay pin setup -----------
 */
byte waterPumpRelayPin = 9;
bool waterPumpDefaultState = false;
unsigned long pumpOldTime;

/*
 *--------- Water top level pin setup -----------
 */
byte waterTopLevelPin = 0;
bool topLevelState = true; //top level sensor off

/*
 *--------- Water bottom level pin setup -----------
 */
byte waterBottomLevelPin = 1;
bool bottomLevelState = true;
bool noDelayRefill = false;
bool stopPumpOnLowLevel = true;

/*
 *--------- Water fill selonoid pin setup -----------
 */
byte waterFillSelonoidRelayPin = 8;
bool fillSelonoidState = false;
unsigned long fillOldTime;

/*
 *--------- BME280 setup --------------------
 */
Adafruit_BME280 bme;

/*
 *--------- flow sensor pin setup -----------
 */
byte flowSensorPin = 7;
// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;
volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long flowOldTime;

/*
 *--------- Water temp pin setup -----------
 */
byte waterTempPin = 6;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(waterTempPin);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

/********************************************************************/

void setup()
{
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  // initialize sensor library for water temp DS18b20
  sensors.begin();

  pinMode(flowSensorPin, INPUT);
  pinMode(waterTopLevelPin, INPUT);
  pinMode(waterBottomLevelPin, INPUT);
  pinMode(waterPumpRelayPin, OUTPUT);
  pinMode(waterFillSelonoidRelayPin, OUTPUT);
  digitalWrite(flowSensorPin, HIGH);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  flowOldTime = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);

  // top level water sensor triggers when closed
  attachInterrupt(digitalPinToInterrupt(waterTopLevelPin), stopFill, RISING);

  // bottom level water sensor triggers when open
  attachInterrupt(digitalPinToInterrupt(waterBottomLevelPin), stopPump, FALLING);

  // BME280 temp humidity and pressure
  if (!bme.begin(0x76))
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }
}

void loop()
{
  DynamicJsonDocument json(1024);
  if (Serial.available() > 0)
  {
    String data = Serial.readStringUntil('\n');
    update_interval = data.toInt();
    json["recieved"] = data;
  }

  /*
  *-------------- read water level sensors -------------------------
  */
  topLevelState = digitalRead(waterTopLevelPin);
  json["water"]["topLevelState"] = topLevelState;
  bottomLevelState = digitalRead(waterBottomLevelPin);
  json["water"]["bottomLevelState"] = bottomLevelState;
  if (!bottomLevelState)
  {
    digitalWrite(waterPumpRelayPin, 0);
  }

  /*
  *-------------- PH read and add to JSON -------------------------
  */
  if (PHSensorPin != 100)
  {
    // read the input on analog pin 0:
    unsigned int sensorValue = analogRead(PHSensorPin);
    // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
    float voltage = sensorValue * (5.0 / 1023.0);
    // print out the value you read:
    json["ph"]["phvalue"] = sensorValue;
  }

  /*
  *-------------- Water temp read and add to JSON -------------------------
  * call sensors.requestTemperatures() to issue a global temperature
  * request to all devices on the bus - DS18b20
  */
  if (waterTempPin != 100)
  {
    sensors.requestTemperatures(); // Send the command to get temperature readings
    json["water"]["temp"] = sensors.getTempCByIndex(0);
  }

  /*
   *-------------- Light sensor  read and add to JSON -------------------------
   */
  if (LightSensorPin != 100)
  {
    //LM393 start analog light read
    unsigned int LightAnalogValue = analogRead(LightSensorPin);
    json["environment"]["light"] = LightAnalogValue;
  }

  /* 
  * -------------- Environmental sensor BME280 read and add to JSON -----------
  */
  json["environment"]["temp"] = bme.readTemperature();
  json["environment"]["pressure"] = bme.readPressure() / 100.0F;
  json["environment"]["humidity"] = bme.readHumidity();

  /*
  * ------------flow rate read and add to JSON -----------
  */
  if (flowSensorPin != 100)
  {
    if ((millis() - flowOldTime) > update_interval) // Only process counters once per second
    {
      // Disable the interrupt while calculating flow rate and sending the value to
      // the host
      detachInterrupt(digitalPinToInterrupt(flowSensorPin));

      // Because this loop may not complete in exactly 1 second intervals we calculate
      // the number of milliseconds that have passed since the last execution and use
      // that to scale the output. We also apply the calibrationFactor to scale the output
      // based on the number of pulses per second per units of measure (litres/minute in
      // this case) coming from the sensor.
      flowRate = ((update_interval / (millis() - flowOldTime)) * pulseCount) / calibrationFactor;

      // Note the time this processing pass was executed. Note that because we've
      // disabled interrupts the millis() function won't actually be incrementing right
      // at this point, but it will still return the value it was set to just before
      // interrupts went away.
      flowOldTime = millis();

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
      attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);
    }
  }

  Serial.println(serializeJson(json, Serial));
  delay(update_interval);
}

/*
* Insterrupt Service Routine - pulse counter increment for flow meter
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}

/*
* Insterrupt Service Routine - 
* stop water pump when water level low if stopPumpOnLowLevel == true
* start filling reservoir immediatly if noDelayRefill == true
 */
void stopPump()
{
  // if stopPumpOnLowLevel is set to true,
  // stop the water pump when low level is triggered
  if (stopPumpOnLowLevel)
  {
    digitalWrite(waterPumpRelayPin, LOW);
  }
  // if noDelayRefill is set to true, start water reservoir fill immediatly
  if (noDelayRefill)
  {
    digitalWrite(waterFillSelonoidRelayPin, HIGH);
  }
}

/*
* Insterrupt Service Routine - 
* stop water fill through selonoid when reservoir is full
 */
void stopFill()
{
  digitalWrite(waterFillSelonoidRelayPin, LOW);
}