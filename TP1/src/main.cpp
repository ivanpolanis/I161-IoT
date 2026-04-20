#include "wifi.h"
#include "dht.h"
#include "routes.h"
#include "spiffs_manager.h"
#include "led_control.h"
#include <WebServer.h>

#define DHTPIN 25
#define DHTTYPE DHT22

WebServer server(80);


void setup() {
  Serial.begin(115200);
  initializeDHT(DHTPIN, DHTTYPE);
  initializeWifi();
  initializeLed();

  if (!initSPIFFS()) {
    Serial.println("Fatal: SPIFFS init failed");
    return;
  }

  setupRoutes(server);
  server.begin();
}

void loop() {
  server.handleClient();
}


