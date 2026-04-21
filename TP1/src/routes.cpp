#include "routes.h"
#include "spiffs_manager.h"
#include "dht.h"
#include "led_control.h"

void setupRoutes(WebServer& server) {
  // -------------------- Static files --------------------
  server.on("/", [&]() { serveFile(server, "/index.html", "text/html"); });
  server.on("/style.css", [&]() { serveFile(server, "/style.css", "text/css"); });
  server.on("/index.js", [&]() { serveFile(server, "/index.js", "application/javascript"); });

  // -------------------- API routes --------------------
  server.on("/state", HTTP_GET, [&]()
            {
    float temperature = readTemperature();
    float humidity = readHumidity();
    bool ledState = readLed();
    String tempStr = isnan(temperature) ? "--" : String(temperature, 1);
    String humStr  = isnan(humidity)    ? "--" : String(humidity, 1);
    String response = "{\"temperature\": " + tempStr + ", \"humidity\": " + humStr + ", \"ledOn\": " + String(ledState ? "true" : "false") + "}";
    server.send(200, "application/json", response); }
  );

  server.on("/toggle-led", HTTP_POST, [&]()
            {
    toggleLed(); 
    server.send(200, "application/json", "{\"status\": \"LED toggled\"}"); }
  );
}