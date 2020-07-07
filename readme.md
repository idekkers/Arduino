# Arduino code for hydrophonics system

## optional inputs from web interface (Raspberry pi)

# analog pin assignments
* PHSensorPin : analog pin assignment for PH sensor (defaults to A0)
* TDSSensorPin : analog pin assignment for TDS sensor (defaults to A2)
* LightSensorPin : analog pin assignment for light intensity sensor (defaults to A1)

# digital input pin assignments
* waterTopLevelPin : digital input pin for top water level sensor (defaults to 0)
* waterBottomLevelPin : digital input pin for bottom water level sensor (defaults to 1)
* flowSensorPin : digital input pin for top water flow sensor (defaults to 7)
* waterTempPin : digital input pin for top water temprature sensor (defaults to 6)

# digital output pin assignments (relays)
* waterPumpRelayPin : digital output pin for water pump relay (defaults to 9)
* waterFillSelonoidRelayPin : digital output pin for water fill selonoid relay (defaults to 8)

# setup parameters
* waterPumpDefaultState : default water pump state (defaults to off == 0)
* noDelayRefill : start tank fiil on low level with no delay (defaults to off == 0)
* stopPumpOnLowLevel : stop water pump on low water level (defaults to on == 1) 