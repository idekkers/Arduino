
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

  // The Hall-effect sensor is configured to trigger on a FALLING state change
  // (transition from HIGH state to LOW state)
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

  //CCS811 co2, voc and temp sensor
  // if(!ccs.begin())
  // {
  //   Serial.println("Failed to start ccs sensor! Please check your wiring.");
  //   while(1);
  // }

  //calibrate temperature sensor
  while (!ccs.available())
    ;
  float temp = ccs.calculateTemperature();
  ccs.setTempOffset(temp - 25.0);

  //    RTC clock setup
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  //    if (!rtc.isrunning())
  //    {
  //        Serial.println("RTC is NOT running!");
  //        // following line sets the RTC to the date & time this sketch was compiled
  //        // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //        // This line sets the RTC with an explicit date & time, for example to set
  //        // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2020, 9, 28, 6, 48, 0));
  //    }

  /*
    --------- if pump should work all the time
    or time is greater then pump start time and smaller then stop time -----------
  */
  if (waterPumpConstantFlow || (WaterPumpStartTime > rtc.now().hour() && WaterPumpStopTime < rtc.now().hour()))
  {
    digitalWrite(waterPumpRelayPin, HIGH);
  }

  // set software interrupt to check when to shut off nutrients and ph pumps
  //Timer1.initialize();
  //Timer1.attachInterrupt(checkPumpStop);
  //Timer1.setPeriod(interruptTimer);
}
