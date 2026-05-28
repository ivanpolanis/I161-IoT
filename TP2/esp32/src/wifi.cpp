#include "wifi.h"

void initializeWifi() {
  WiFiManager wifiManager;

  wifiManager.setConnectTimeout(30);
  wifiManager.setConfigPortalTimeout(180);

  Serial.println("Iniciando WiFiManager...");

  if (!wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD)) {
    Serial.println("Error: no se pudo conectar. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("Conectado a WiFi");
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());
}
