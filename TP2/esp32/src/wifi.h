#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

#define WIFI_AP_NAME     "ESP32-IoT-Config"
#define WIFI_AP_PASSWORD "password"

void initializeWifi();

#endif
