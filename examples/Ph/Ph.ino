#include <ArduinoJson.h>

char PHSensorPin = A0;
int update_interval = 15000;

void setup() {
  // initialize serial communication at 9600 bits per second:
 Serial.begin(9600);
}

void loop() {
  DynamicJsonDocument json(1024);
    if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    update_interval=data.toInt();
    json["recieved"]=data;
  }
   // read the input on analog pin 0:
  int sensorValue = analogRead(PHSensorPin);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0);
  // print out the value you read:
  //Serial.println(sensorValue);
  json["ph"]["phvalue"]=sensorValue;
  //Serial.println(voltage);
  Serial.println(serializeJson(json, Serial));
  delay(update_interval);
}
