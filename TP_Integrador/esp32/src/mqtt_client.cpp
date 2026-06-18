#include "mqtt_client.h"
#include "config.h"
#include "access_logic.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static WiFiClient   wifiClient;
static PubSubClient mqttClient(wifiClient);

static unsigned long nextReconnectAt = 0;
static unsigned long reconnectDelay  = 1000; // backoff exponencial, máx 30s

static void onMqttMessage(const char* topic, byte* payload, unsigned int length) {
    // Buffer en stack, sin heap. 511 bytes es más que suficiente para nuestros JSON.
    if (length >= 512) { Serial.println("[MQTT] Payload demasiado grande, ignorado."); return; }
    char body[512];
    memcpy(body, payload, length);
    body[length] = '\0';

    if (strcmp(topic, TOPIC_RESPONSE) == 0) {
        onServerResponse(body);
    } else if (strcmp(topic, TOPIC_CMD_INV) == 0) {
        onCommandInvalidate(body);
    } else if (strcmp(topic, TOPIC_CMD_LOCK) == 0) {
        onCommandLockdown(body);
    } else if (strcmp(topic, TOPIC_CMD_CFG) == 0) {
        onCommandConfig(body);
    }
}

static void publishStatus(bool online) {
    JsonDocument doc;
    doc["online"] = online;
    if (online) {
        doc["fw"]     = "1.0.0";
        doc["uptime"] = millis() / 1000;
    }
    String payload;
    serializeJson(doc, payload);
    // Retained: el broker guarda el último estado para nuevos suscriptores
    mqttClient.publish(TOPIC_STATUS, payload.c_str(), true);
}

static bool reconnect() {
    // LWT: publicado automáticamente por el broker si el cliente se desconecta abruptamente
    String lwt;
    JsonDocument lwtDoc;
    lwtDoc["online"] = false;
    serializeJson(lwtDoc, lwt);

    bool ok = mqttClient.connect(
        "esp32-" DEVICE_ID,
        MQTT_USER, MQTT_PASS,
        TOPIC_STATUS, 1, true, lwt.c_str()
    );

    if (!ok) {
        Serial.printf("[MQTT] Falló conexión. Estado: %d\n", mqttClient.state());
        return false;
    }

    mqttClient.subscribe(TOPIC_RESPONSE, 1);
    mqttClient.subscribe(TOPIC_CMD_INV,  1);
    mqttClient.subscribe(TOPIC_CMD_LOCK, 1);
    mqttClient.subscribe(TOPIC_CMD_CFG,  1);

    publishStatus(true);
    Serial.printf("[MQTT] Conectado a %s:%d\n", MQTT_HOST, MQTT_PORT);
    return true;
}

void initMqtt() {
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(onMqttMessage);
    mqttClient.setKeepAlive(MQTT_KEEPALIVE_S);
    mqttClient.setBufferSize(512);
}

void mqttTick() {
    if (mqttClient.connected()) {
        mqttClient.loop();
        reconnectDelay = 1000; // reset backoff al estar conectado
        return;
    }

    unsigned long now = millis();
    if (now < nextReconnectAt) return;

    Serial.println("[MQTT] Intentando reconectar...");
    if (reconnect()) {
        nextReconnectAt = 0;
    } else {
        reconnectDelay  = min(reconnectDelay * 2, 30000UL);
        nextReconnectAt = now + reconnectDelay;
        Serial.printf("[MQTT] Próximo intento en %lums\n", reconnectDelay);
    }
}

bool mqttConnected() {
    return mqttClient.connected();
}

bool mqttPublish(const char* topic, const char* payload, bool retain) {
    return mqttClient.publish(topic, payload, retain);
}

PubSubClient& getMqttClient() {
    return mqttClient;
}
