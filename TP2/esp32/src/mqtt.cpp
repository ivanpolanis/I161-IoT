#include "mqtt.h"

static WiFiClient   espClient;
static PubSubClient mqttClient(espClient);

void initializeMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  Serial.println("MQTT configurado");
}

void ensureMQTTConnected() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando a MQTT...");
    if (mqttClient.connect(MQTT_CLIENT)) {
      Serial.println(" OK");
    } else {
      Serial.print(" Error (rc=");
      Serial.print(mqttClient.state());
      Serial.println("). Reintentando en 5s...");
      delay(5000);
    }
  }
}

void publishSensorData(float temp, float hum) {
  char payload[64];
  snprintf(payload, sizeof(payload),
           "{\"temp\": %.1f, \"hum\": %.1f}", temp, hum);

  bool ok = mqttClient.publish(MQTT_TOPIC, payload);
  Serial.print("Publicado en ");
  Serial.print(MQTT_TOPIC);
  Serial.print(": ");
  Serial.print(payload);
  Serial.println(ok ? " [OK]" : " [ERROR]");
}
