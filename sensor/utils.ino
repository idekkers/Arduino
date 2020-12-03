
/*
  Insterrupt Service Routine - pulse counter increment for flow meter
*/
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}

/*
  Insterrupt Service Routine -
  stop water pump when water level low if stopWaterPumpOnLowLevel == true
  start filling reservoir immediatly if waterRefillNoDelay == true
*/
void stopPump()
{
  // if stopWaterPumpOnLowLevel is set to true,
  // stop the water pump when low level is triggered
  if (stopWaterPumpOnLowLevel)
  {
    digitalWrite(waterPumpRelayPin, LOW);
  }
  // if waterRefillNoDelay is set to true, start water reservoir fill immediatly
  if (waterRefillNoDelay)
  {
    digitalWrite(waterFillSelonoidRelayPin, HIGH);
  }
  else
  {
    fillDelayStartTime = rtc.now().unixtime();
  }
}

/*
  Insterrupt Service Routine -
  stop water fill through selonoid when reservoir is full
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

/*
  software Insterrupt Service Routine -
  stop nutrients and ph pumps after set interval
*/
void checkPumpStop()
{
  // check nutrients pump working
  if (digitalRead(nutrientPump1RelayPin) == HIGH)
  {
    // check nutrients pump fill duration is done
    if (tempNutrientPump1FillDuration <= 0)
    {
      digitalWrite(nutrientPump1RelayPin, LOW);
      tempNutrientPump1FillDuration = nutrientPump1FillDuration;
    }
    else
    {
      tempNutrientPump1FillDuration = tempNutrientPump1FillDuration - interruptTimer;
    }
  }
  // check PH- pump working
  if (digitalRead(phMinusPumpRelayPin) == HIGH)
  {
    // check PH- pump fill duration is done
    if (tempPhMinusPumpFillDuration <= 0)
    {
      digitalWrite(phMinusPumpRelayPin, LOW);
      tempPhMinusPumpFillDuration = phMinusPumpFillDuration;
    }
    else
    {
      tempPhMinusPumpFillDuration = tempPhMinusPumpFillDuration - interruptTimer;
    }
  }
  // check PH+ pump working
  if (digitalRead(phPlusPumpRelayPin) == HIGH)
  {
    // check PH+ pump fill duration is done
    if (tempPhPlusPumpFillDuration <= 0)
    {
      digitalWrite(phPlusPumpRelayPin, LOW);
      tempPhPlusPumpFillDuration = phPlusPumpFillDuration;
    }
    else
    {
      tempPhPlusPumpFillDuration = tempPhPlusPumpFillDuration - interruptTimer;
    }
  }
}
