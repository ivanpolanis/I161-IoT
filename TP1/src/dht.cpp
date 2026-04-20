#include "dht.h"

DHT* dht = nullptr;

void initializeDHT(uint8_t pin, uint8_t type) {
  dht = new DHT(pin, type);
  dht->begin();
}

float readTemperature(bool force) {
  if (dht == nullptr) {
    return NAN;
  } 
  return dht->readTemperature(force);
}

float readHumidity(bool force) {
  if (dht == nullptr) {
    return NAN;
  }
  return dht->readHumidity(force);
}