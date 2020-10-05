# Arduino code for hydrophonics sensor system
The system recievs optional external variables, defaults are set in software.
The system measures air and water data (all sensors optional)- 
- PH
- TDS
- water temp
- air temp
- air humidity
- barometric preasure
- light intensity
- CO2 and VOC levels

## optional inputs from web interface (Raspberry pi - json format)
# TODO 
- [ ] add json input for variables and pins
- [x] add logic to open water selonoid after 15 min from low level triger
- [x] add tds sensor input
- [x] add logic for nutrients pump activation
- [x] add logic for ph+ pump activation
- [x] add logic for ph- pump activation
- [ ] ph calibration code

### analog pin assignments
* PHSensorPin: char - analog pin assignment for PH sensor (defaults to A0)
* TDSSensorPin: char - analog pin assignment for TDS sensor (defaults to A2)
* LightSensorPin: char - analog pin assignment for light intensity sensor (defaults to A1)

### digital input pin assignments
* waterTopLevelPin: byte -  digital input pin for top water level sensor (defaults to 0)
* waterBottomLevelPin: byte - digital input pin for bottom water level sensor (defaults to 1)
* flowSensorPin: byte - digital input pin for top water flow sensor (defaults to 2)
* waterTempPin: byte - digital input pin for top water temprature sensor (defaults to 3)

### digital output pin assignments (relays)
* waterPumpRelayPin: byte - digital output pin for water pump relay (defaults to 9)
* waterFillSelonoidRelayPin: byte - digital output pin for water fill selonoid relay (defaults to 8)
* nutrientPump1RelayPin: digital output pin for nutrient pump relay (defaults to 10)
* phMinusPumpRelayPin: digital output pin for PH- pump relay (defaults to 11)
* phPlusPumpRelayPin: digital output pin for PH+ pump relay (defaults to 12)

### setup parameters
* water
    * waterPumpDefaultState: default water pump state (defaults to false)
    * waterFillDelay: delay from waterBottomLevelPin set LOW until water fill starts if noDelayRefill set to false (defaults to 1200)
    * waterPumpConstantFlow: set if water pump works constantly (defaults to false)
    * noDelayRefill: start tank fill on low level with no delay (defaults to false)
    * stopPumpOnLowLevel: stop water pump on low water level (defaults to true)
* light
    * lightOnLuminosity: luminosity level to switch lights on/off (defaults to 1000)
* Ph
    * phPlusPumpFillDuration: duration of PH+ pump on state (defaults to 5)
    * phMinusPumpFillDuration: duration of PH- pump on state (defaults to 5)
    * phFillDelay: delay from last Ph change to let the acid/base mix properly (defaults to 900)
    * triggerPhMinusPump: start adding acid on this level (defaults to 6.5)
    * triggerPhPlusPump: start adding base on this level (defaults to 4.5)
    * optimalPh: optimal Ph level (defaults to 5.5)
* nutrients
    * nutrientPump1FillDuration: duration of nutrients pump on state (defaults to 5)
    * nutrientFillDelay: delay from last nutrients change to let the nutrients mix properly (defaults to 900)
    * requiredNutrientLevel: stop nutrients fill on this level (defaults to 700)
    * minimumNutrientLevel: start nutrients fill on this level (defaults to 450)

## json output from arduino
```
{
    "water":{
        "topLevelState": int,
        "bottomLevelState": bool,
        "temp": float,
        "flowRate": 
    },
    "ph":{
        "phvalue": int
    },
    "tds":{
        "tdsvalue": float
    },
    "environment":{
        "light": int,
        "temp": float,
        "pressure": float,
        "humidity": float
    }
}
```