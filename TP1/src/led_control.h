#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#define PIN 2

#include <Arduino.h>

void initializeLed(uint8_t pin = PIN);
void toggleLed(uint8_t pin = PIN);
bool readLed(uint8_t pin = PIN);

#endif 