#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include "Arduino.h"
#define emergencyTempPin 5

int main(void) {
  init();
  Adafruit_MAX31855 t_1(13, 10, 12); //SCLK, CS, MISO
  Adafruit_MAX31855 t_2(13, 9, 12);
  const long interval = 5000; //Logging interval
  unsigned long previousMillis = 0;
  int tempLimit = 255;
  bool emergency = false;
  pinMode(5, OUTPUT); //Emergency relay
  Serial.begin(9600);
  delay(500);

  unsigned long startTime = millis();
  byte requestIndex = 1;
  Serial.println("$init"); //Send settings request to server (tempLimit)
  do {
    if (millis() - startTime > 5000 * requestIndex) {
      Serial.println("$init");
      requestIndex++;
    }
  } while (Serial.available() == 0 && requestIndex <= 5); //Wait for response, if server does not respond => Use default tempLimit settings
  if (Serial.available() > 0) {
    tempLimit = Serial.parseInt();
  }

  while (true) { //Main loop
    float mes_1 = t_1.readCelsius();
    //mes_1 = 21.01;
    float mes_2 = t_2.readCelsius();
    //mes_2 = 25.02;

    //Toggle emergency pin to HIGH if measured temperature is greater than tempLimit.
    if ((mes_1 >= tempLimit && mes_1 != 2047.75) || (mes_2 >= tempLimit &&  mes_2 != 2047.75)) {
      digitalWrite(emergencyTempPin, HIGH);
      emergency = true;
    } else {
      //Toggle emergency pin to LOW if measured temperature is lower than tempLimit - 5 celsius (Hysteresis)
      if ((emergency && mes_1 <= tempLimit - 5) || (mes_2 <= tempLimit - 5)) { //Hysteresis
        digitalWrite(emergencyTempPin, LOW);
        emergency = false;
      }
    }

    unsigned long currentMillis = millis();

    //Log measurements, also log warning message if measured temperature is greater than tempLimit.
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      String _message = "{\"origin\":\"Thermocouple\",\"type\":\"measurement\",\"value\":\"" + String(mes_1, 3) + "," + String(mes_2, 3) + "\"}";
      Serial.println(_message);
      if (emergency) {
        _message = "{\"origin\":\"Thermocouple\",\"type\":\"log\",\"msg\":\"High temperature\"}";
        Serial.println(_message);
      }
    }
  }
}
