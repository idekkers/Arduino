void loop()
{
  DynamicJsonDocument json(1024);
  if (Serial.available() > 0)
  {
    StaticJsonDocument<jsoncapacity> doc;
    String data = Serial.readStringUntil('\n');
    DeserializationError err = deserializeJson(doc, data);
    if (err)
    {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
    }
    // pin assignment
    PHSensorPin = doc["pins"]["PHSensorPin"] | A0;
    TDSSensorPin = doc["pins"]["TDSSensorPin"] | A2;
    LightSensorPin = doc["pins"]["LightSensorPin"] | A1;
    waterTopLevelPin = doc["pins"]["waterTopLevelPin"] | 0;
    waterBottomLevelPin = doc["pins"]["waterBottomLevelPin"] | 1;
    flowSensorPin = doc["pins"]["flowSensorPin"] | 2;
    waterTempPin = doc["pins"]["waterTempPin"] | 3;
    waterFillSelonoidRelayPin = doc["pins"]["waterFillSelonoidRelayPin"] | 8;
    waterPumpRelayPin = doc["pins"]["waterPumpRelayPin"] | 9;
    nutrientPump1RelayPin = doc["pins"]["nutrientPump1RelayPin"] | 10;
    phMinusPumpRelayPin = doc["pins"]["phMinusPumpRelayPin"] | 11;
    phPlusPumpRelayPin = doc["pins"]["phPlusPumpRelayPin"] | 12;
    // variables assignment
    lightOnLuminosity = doc["variables"]["lightOnLuminosity"] | 1000;
    waterRefillNoDelay = doc["variables"]["waterRefillNoDelay"] | false;
    stopWaterPumpOnLowLevel = doc["variables"]["stopWaterPumpOnLowLevel"] | false;
    waterFillDelay = doc["variables"]["waterFillDelay"] | 1200;
    waterPumpDefaultState = doc["variables"]["waterPumpDefaultState"] | false;
    waterPumpConstantFlow = doc["variables"]["waterPumpConstantFlow"] | false;
    nutrientPump1FillDuration = doc["variables"]["nutrientPump1FillDuration"] | 5000000;
    minimumNutrientLevel = doc["variables"]["minimumNutrientLevel"] | 1000;
    requiredNutrientLevel = doc["variables"]["requiredNutrientLevel"] | 1500;
    nutrientFillDelay = doc["variables"]["nutrientFillDelay"] | 900;
    phMinusPumpFillDuration = doc["variables"]["phMinusPumpFillDuration"] | 5000000;
    phPlusPumpFillDuration = doc["variables"]["phPlusPumpFillDuration"] | 5000000;
    triggerPhPlusPump = doc["variables"]["triggerPhPlusPump"] | 4.5;
    triggerPhMinusPump = doc["variables"]["triggerPhMinusPump"] | 6.5;
    optimalPh = doc["variables"]["optimalPh"] | 5.5;
    phFillDelay = doc["variables"]["phFillDelay"] | 900;
  }

  /*
    --------- reduce delays by interval time --------------------
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
    --------------- water level fill check --------------------
    check if waterRefillNoDelay set to false and fillDelayStartTime not 0
    then check if current time is less then 'fill delay' plus 'fill delay start time',
    if so, open the fill selonoid
  */
  if (!waterRefillNoDelay && fillDelayStartTime != 0)
  {
    if (rtc.now().unixtime() < fillDelayStartTime + waterFillDelay)
    {
      fillDelayStartTime = 0;
      digitalWrite(waterFillSelonoidRelayPin, HIGH);
    }
  }

  /*
    -------------- read water level sensors -------------------------
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
    ------------ read flow rate and add to JSON -----------
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
    -------------- start and stop water pump if not supposed to work constantly -------------------------
  */
  if (!waterPumpConstantFlow)
  {
    if (WaterPumpStartTime > rtc.now().hour() && WaterPumpStopTime < rtc.now().hour())
    {
      digitalWrite(waterPumpRelayPin, HIGH);
    }
    else
    {
      digitalWrite(waterPumpRelayPin, LOW);
    }
  }

  /*
    -------------- Water temp read and add to JSON -------------------------
    call sensors.requestTemperatures() to issue a global temperature
    request to all devices on the bus - DS18b20
  */
  if (waterTempPin != 100)
  {
    sensors.requestTemperatures(); // Send the command to get temperature readings
    temperature = sensors.getTempCByIndex(0);
    json["water"]["temp"] = temperature;
  }

  /*
    -------------- PH read and add to JSON -------------------------
  */
  if (PHSensorPin != 100)
  {
    // read the input on analog pin 0:
    unsigned int sensorValue = analogRead(PHSensorPin);
    // print out the value you read:
    json["ph"]["phvalue"] = sensorValue;

    if (!nutrientLevelingCycle)
    {
      // if TDS level lower then minimum, set leveling cycle to true, and turn on the nutrients pump
      if (sensorValue > triggerPhMinusPump && !phLevelingCycle)
      {
        digitalWrite(phMinusPumpRelayPin, HIGH);
        phLevelingCycle = true;
      }
      // if in leveling cycle
      if (phLevelingCycle)
      {
        // if required level reached, and not below low setting, stop Ph leveling cycle, if below, start ph+ pump
        if (sensorValue <= optimalPh)
        {
          if (sensorValue <= triggerPhPlusPump)
          {
            digitalWrite(phPlusPumpRelayPin, HIGH);
          }
          else
          {
            phLevelingCycle = false;
          }
        }
        //if required level not reached, open nutrients pump and reset fill delay
        else if (tempPhtfillDelay <= 0)
        {
          digitalWrite(phMinusPumpRelayPin, HIGH);
          tempPhtfillDelay = phFillDelay;
        }
      }
    }
  }

  /*
    -------------- TDS read and add to JSON -------------------------
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
      averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)vref / 1024.0;                                                                                                  // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);                                                                                                               //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge = averageVoltage / compensationCoefficient;                                                                                                            //temperature compensation
      tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
    }
    // print out the value you read:
    json["tds"]["tdsvalue"] = tdsValue;

    // if TDS level lower then minimum, set leveling cycle to true, and turn on the nutrients pump
    if (tdsValue < minimumNutrientLevel && !nutrientLevelingCycle)
    {
      digitalWrite(nutrientPump1RelayPin, HIGH);
      nutrientLevelingCycle = true;
    }
    // if in leveling cycle
    if (nutrientLevelingCycle)
    {
      // if required level reached, stop nutrients leveling cycle
      if (tdsValue >= requiredNutrientLevel)
      {
        nutrientLevelingCycle = false;
      }
      //if required level not reached, open nutrients pump and reset fill delay
      else if (tempNutrientfillDelay <= 0)
      {
        digitalWrite(nutrientPump1RelayPin, HIGH);
        tempNutrientfillDelay = nutrientFillDelay;
      }
    }
  }

  /*
    -------------- Light sensor read and add to JSON -------------------------
  */
  if (LightSensorPin != 100)
  {
    //LM393 start analog light read
    unsigned int LightAnalogValue = analogRead(LightSensorPin);
    json["environment"]["light"] = LightAnalogValue;
  }

  /*
    -------------- Environmental sensor BME280 read and add to JSON -----------
  */
  json["environment"]["bmetemp"] = bme.readTemperature();
  json["environment"]["pressure"] = bme.readPressure() / 100.0F;
  json["environment"]["humidity"] = bme.readHumidity();

  /*
    -------------- Environmental sensor CCS811 read and add to JSON -----------
  */
  // if(ccs.available()){
  //   if(!ccs.readData()){
  //     json["environment"]["co2"] = ccs.geteCO2();
  //     json["environment"]["tvoc"] = ccs.getTVOC();
  //     json["environment"]["ccstemp"] = ccs.calculateTemperature();
  //   }
  //   else{
  //     Serial.println("ERROR!");
  //     while(1);
  //   }
  // }

  Serial.println(serializeJson(json, Serial));
  delay(update_interval);
}
