#include <Wire.h>
#include <OneWire.h>
#ifdef ARDUINO_ARCH_MEGAAVR
#include "EveryTimerB.h"
#define Timer1 TimerB2    // use TimerB2 as a drop in replacement for Timer1
#else // assume architecture supported by TimerOne library ....
#include "TimerOne.h"
#endif
#include "RTClib.h"
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "Adafruit_CCS811.h"
#include <ArduinoJson.h>

/********************************************************************/

int update_interval = 60000;

long interruptTimer = 1000000;

/*
  ------------------ pin setup -------------------------------
  ! error color
  ? question color
  TODO color
  @param param
*/

/* --------------------------- sensor pin setup -------------------------------*/
/*
   --------- PH pin setup --------------------
*/
char PHSensorPin = A0;

/*
  --------- TDS pin setup --------------------
*/
char TDSSensorPin = A2;
#define vref 5.0          // analog reference voltage(Volt) of the ADC
#define SCOUNT 30         // sum of sample point
int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;

/*
  --------- Light sensor pin setup ----------
*/
char LightSensorPin = A1;
int lightOnLuminosity = 1000;

/*
  --------- Water top level pin setup -----------
*/
byte waterTopLevelPin = 0;
bool topLevelState = true; //top level sensor off

/*
  --------- Water bottom level pin setup -----------
*/
byte waterBottomLevelPin = 1;
bool bottomLevelState = true;
bool waterRefillNoDelay = false;
bool stopWaterPumpOnLowLevel = true;

/*
  --------- flow sensor pin setup -----------
*/
byte flowSensorPin = 2;
// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;
volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long flowOldTime;

/*
  --------- Water temp pin setup -----------
*/
byte waterTempPin = 3;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(waterTempPin);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

/* --------------------------- relay pin setup -------------------------------*/

/*
  --------- Water fill selonoid pin setup -----------
*/
byte waterFillSelonoidRelayPin = 8;
bool fillSelonoidState = false;
unsigned long waterFillDelay = 1200; // in seconds
unsigned long fillDelayStartTime = 0;

/*
  --------- Water pump relay pin setup -----------
*/
byte waterPumpRelayPin = 9;
bool waterPumpDefaultState = false;
bool waterPumpConstantFlow = false;
unsigned long WaterPumpStartTime;
unsigned long WaterPumpStopTime;

/*
  --------- nutrients pump 1 pin setup -----------
*/
byte nutrientPump1RelayPin = 10;
unsigned long nutrientPump1FillDuration = 5 * 1000000;     //in seconds
unsigned long tempNutrientPump1FillDuration = 5 * 1000000; //in seconds

/*
  --------- nutrients constants setup -----------
*/
unsigned int minimumNutrientLevel = 450; // low nutrients level to start leveling process
unsigned int requiredNutrientLevel = 700; // required nutrients level to stop leveling process
unsigned long nutrientFillDelay = 900;   // in seconds
unsigned long tempNutrientfillDelay = 0; // in seconds
bool nutrientLevelingCycle = false;

/*
  --------- ph- pump pin setup -----------
*/
byte phMinusPumpRelayPin = 11;
unsigned long phMinusPumpFillDuration = 5 * 1000000;     //in seconds
unsigned long tempPhMinusPumpFillDuration = 5 * 1000000; //in seconds

/*
  --------- ph+ pump pin setup -----------
*/
byte phPlusPumpRelayPin = 12;
unsigned long phPlusPumpFillDuration = 5 * 1000000;     //in seconds
unsigned long tempPhPlusPumpFillDuration = 5 * 1000000; //in seconds

/*
  --------- ph- constants setup -----------
*/
float triggerPhPlusPump = 4.5; // Low Ph level to start Ph leveling
float triggerPhMinusPump = 6.5; // High Ph level to start Ph leveling
float optimalPh = 5.5;
unsigned long phFillDelay = 900;    // delay between Ph leveling atempts in seconds
unsigned long tempPhtfillDelay = 0; // in seconds
bool phLevelingCycle = false; // is Ph leveling in progress

/* --------------------------- I2C setup -------------------------------*/

/*
  --------- BME280 setup --------------------
*/
Adafruit_BME280 bme;

/*
  --------- CCS811 setup --------------------
*/
Adafruit_CCS811 ccs;

/*
  --------- RTC clock setup -----------
*/
RTC_DS1307 rtc;

/********************************************************************/
