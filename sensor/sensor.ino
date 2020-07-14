#include <Wire.h>
#include <OneWire.h>
#include "RTClib.h"
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

/* --------------------------- sensor pin setup -------------------------------*/
/*
 *--------- PH pin setup --------------------
 */
char PHSensorPin = A0;

/*
 *--------- TDS pin setup --------------------
 */
char TDSSensorPin = A2;
#define VREF 5.0          // analog reference voltage(Volt) of the ADC
#define SCOUNT 30         // sum of sample point
int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;

/*
 *--------- Light sensor pin setup ----------
 */
char LightSensorPin = A1;

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

/* --------------------------- relay pin setup -------------------------------*/
/*
 *--------- Water pump relay pin setup -----------
 */
byte waterPumpRelayPin = 9;
bool waterPumpDefaultState = false;
bool waterPumpConstantFlow = false;
unsigned long pumpStartTime;
unsigned long pumpStopTime;

/*
 *--------- Water fill selonoid pin setup -----------
 */
byte waterFillSelonoidRelayPin = 8;
bool fillSelonoidState = false;
unsigned long fillDelay = 900; // in seconds
unsigned long fillDelayStartTime = 0;

/*
 *--------- nutrients pump 1 pin setup -----------
 */
byte nutrientPump1RelayPin = 10;
unsigned long nutrientPump1Duration = 5; //in seconds
unsigned int minimumNutrientLevel = 450;
unsigned int requiredNutrientLevel = 700;
unsigned long nutrientfillDelay = 900;     // in seconds
unsigned long tempNutrientfillDelay = 900; // in seconds
byte nutrientLevelingTry = 0;

/*
 *--------- ph- pump pin setup -----------
 */
byte phMinusPumpRelayPin = 11;
unsigned long phMinusPumpDuration = 5; //in seconds
float minimumPhLevel = 5.5;
float maximumPhLevel = 6.5;
unsigned long phfillDelay = 900; // in seconds
unsigned long phTestDelayStartTime = 0;

/*
 *--------- ph+ pump pin setup -----------
 */
byte phPlusPumpRelayPin = 12;
unsigned long phPlusPumpDuration = 5; //in seconds

/* --------------------------- I2C setup -------------------------------*/

/*
 *--------- BME280 setup --------------------
 */
Adafruit_BME280 bme;

/*
 *--------- RTC clock setup -----------
 */
RTC_DS1307 rtc;

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
  pinMode(TDSSensorPin, INPUT);
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

  //RTC clock setup
  // if (!rtc.begin())
  // {
  //   Serial.println("Couldn't find RTC");
  //   while (1);
  // }
  if (!rtc.isrunning())
  {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  /*
 *--------- if pump should work all the time 
 * or time is greater then pump start time and smaller then stop time -----------
 */
  if (waterPumpConstantFlow || (pumpStartTime > rtc.now().hour() && pumpStopTime < rtc.now().hour()))
  {
    digitalWrite(waterPumpRelayPin, HIGH);
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
 *--------- reduce delays by interval time --------------------
 */
  if (tempNutrientfillDelay > update_interval / 1000)
  {
    tempNutrientfillDelay = tempNutrientfillDelay - update_interval / 1000;
  }
  else
  {
    tempNutrientfillDelay = 0;
  }

  /*
  * --------------- water level fiil check --------------------
  * check if noDelayRefill set to false and fillDelayStartTime not 0 
  * then check if current tiem is less then fill delay plus fill delay start time, 
  * if so, open the fill selonoid
  */
  if (!noDelayRefill && fillDelayStartTime != 0)
  {
    if (rtc.now().unixtime() < fillDelayStartTime + fillDelay)
    {
      fillDelayStartTime = 0;
      digitalWrite(waterFillSelonoidRelayPin, HIGH);
    }
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
  * ------------ read flow rate and add to JSON -----------
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

  /*
  *-------------- start and stop water pump -------------------------
  */
  if (!waterPumpConstantFlow)
  {
    if (pumpStartTime > rtc.now().hour() && pumpStopTime < rtc.now().hour())
    {
      digitalWrite(waterPumpRelayPin, HIGH);
    }
    else
    {
      digitalWrite(waterPumpRelayPin, LOW);
    }
  }

  /*
  *-------------- Water temp read and add to JSON -------------------------
  * call sensors.requestTemperatures() to issue a global temperature
  * request to all devices on the bus - DS18b20
  */
  if (waterTempPin != 100)
  {
    sensors.requestTemperatures(); // Send the command to get temperature readings
    temperature = sensors.getTempCByIndex(0);
    json["water"]["temp"] = temperature;
  }

  /*
  *-------------- PH read and add to JSON -------------------------
  */
  if (PHSensorPin != 100)
  {
    // read the input on analog pin 0:
    unsigned int sensorValue = analogRead(PHSensorPin);
    // print out the value you read:
    json["ph"]["phvalue"] = sensorValue;
  }

  /*
  *-------------- TDS read and add to JSON -------------------------
  */
  if (TDSSensorPin != 100)
  {
    static unsigned long analogSampleTimepoint = millis();
    if (millis() - analogSampleTimepoint > 40U) //every 40 milliseconds,read the analog value from the ADC
    {
      analogSampleTimepoint = millis();
      analogBuffer[analogBufferIndex] = analogRead(TDSSensorPin); //read the analog value and store into the buffer
      analogBufferIndex++;
      if (analogBufferIndex == SCOUNT)
        analogBufferIndex = 0;
    }
    static unsigned long printTimepoint = millis();
    if (millis() - printTimepoint > 800U)
    {
      printTimepoint = millis();
      for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      {
        analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      }
      averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;                                                                                                  // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);                                                                                                               //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge = averageVoltage / compensationCoefficient;                                                                                                            //temperature compensation
      tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
    }
    // print out the value you read:
    json["tds"]["tdsvalue"] = tdsValue;
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
  else
  {
    fillDelayStartTime = rtc.now().unixtime();
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

int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
