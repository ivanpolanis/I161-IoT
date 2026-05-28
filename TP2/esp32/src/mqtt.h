#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#define MQTT_SERVER   "192.168.0.241"
#define MQTT_PORT     1883
#define MQTT_CLIENT   "ESP32-Ambiente"
#define MQTT_TOPIC    "sensor/ambiente"

void initializeMQTT();
void ensureMQTTConnected();
void publishSensorData(float temp, float hum);

#endif
