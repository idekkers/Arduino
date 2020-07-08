# Arduino code for hydrophonics system

## optional inputs from web interface (Raspberry pi - json format)
//TODO add input json format

### analog pin assignments
* PHSensorPin: char - analog pin assignment for PH sensor (defaults to A0)
* TDSSensorPin: char - analog pin assignment for TDS sensor (defaults to A2)
* LightSensorPin: char - analog pin assignment for light intensity sensor (defaults to A1)

### digital input pin assignments
* waterTopLevelPin: byte -  digital input pin for top water level sensor (defaults to 0)
* waterBottomLevelPin: byte - digital input pin for bottom water level sensor (defaults to 1)
* flowSensorPin: byte - digital input pin for top water flow sensor (defaults to 7)
* waterTempPin: byte - digital input pin for top water temprature sensor (defaults to 6)

### digital output pin assignments (relays)
* waterPumpRelayPin: byte - digital output pin for water pump relay (defaults to 9)
* waterFillSelonoidRelayPin: byte - digital output pin for water fill selonoid relay (defaults to 8)

### setup parameters
* waterPumpDefaultState : default water pump state (defaults to false)
* noDelayRefill : start tank fiil on low level with no delay (defaults to false)
* stopPumpOnLowLevel : stop water pump on low water level (defaults to true) 

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
    "environment":{
        "light": int,
        "temp": float,
        "pressure": float,
        "humidity": float
    }
}
```