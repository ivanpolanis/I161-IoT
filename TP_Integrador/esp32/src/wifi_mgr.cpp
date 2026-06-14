#include "wifi_mgr.h"
#include <WiFiManager.h>
#include <Arduino.h>

static WiFiManager wifiManager;

void initWifi() {
    wifiManager.setConfigPortalTimeout(180);
    wifiManager.setConnectTimeout(30);
    wifiManager.setAPCallback([](WiFiManager*) {
        Serial.println("[WiFi] Portal AP activo: ESP32-AccessControl");
    });

    if (!wifiManager.autoConnect("ESP32-AccessControl")) {
        Serial.println("[WiFi] Timeout. Reiniciando...");
        ESP.restart();
    }
    Serial.printf("[WiFi] Conectado. IP: %s\n", WiFi.localIP().toString().c_str());
}

void handleWifi() {
    // WiFiManager no requiere polling continuo post-conexión.
    // Si se pierde la conexión, PubSubClient lo detecta y reconnecta.
}
