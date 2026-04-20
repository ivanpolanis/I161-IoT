#include "led_control.h"

void initializeLed(uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW); 
}

void toggleLed(uint8_t pin) {
  digitalWrite(pin, !digitalRead(pin));
}

bool readLed(uint8_t pin) {
  return digitalRead(pin) == HIGH;
}