#include "dht.h"
#include "wifi.h"
#include "mqtt.h"

#define DHTPIN      25
#define DHTTYPE     DHT11
#define PUBLISH_MS  5000   

void setup() {
  Serial.begin(115200);
  initializeDHT(DHTPIN, DHTTYPE);
  initializeWifi();
  initializeMQTT();
}

void loop() {
  ensureMQTTConnected();

  float temp = readTemperature();
  float hum  = readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Error leyendo el sensor DHT11");
  } else {
    publishSensorData(temp, hum);
  }

  delay(PUBLISH_MS);
}
