#ifndef DHT22_H
#define DHT22_H

#include <DHT.h>

void initializeDHT(uint8_t pin = 25, uint8_t type = DHT22);
float readTemperature(bool force = false);
float readHumidity(bool force = false);

#endif
