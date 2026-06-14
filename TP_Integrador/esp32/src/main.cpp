#include <Arduino.h>
#include <LittleFS.h>

#include "config.h"
#include "wifi_mgr.h"
#include "mqtt_client.h"
#include "rfid_reader.h"
#include "access_cache.h"
#include "event_queue.h"
#include "actuators.h"
#include "access_logic.h"

static unsigned long lastCachePurge  = 0;
static unsigned long lastHeartbeat   = 0;
static unsigned long lastCardRead    = 0;
static const unsigned long CACHE_PURGE_INTERVAL_MS = 5UL * 60UL * 1000UL;
static const unsigned long HEARTBEAT_INTERVAL_MS   = 10UL * 1000UL;
static const unsigned long CARD_COOLDOWN_MS        = 2000UL;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[Boot] Sistema de Control de Acceso RFID");
    Serial.printf("[Boot] Device: %s  FW: 1.0.0\n", DEVICE_ID);

    if (!LittleFS.begin(true)) {
        Serial.println("[Boot] Error crítico: no se pudo montar LittleFS. Reiniciando...");
        delay(1000);
        ESP.restart();
    }
    Serial.println("[Boot] LittleFS OK.");

    initActuators();
    initCache();
    initQueue();
    initRfid();
    initWifi();
    initMqtt();
    initAccessLogic();

    Serial.println("[Boot] Inicialización completa.");
}

void loop() {
    mqttTick();
    actuatorsTick();
    accessLogicTick();

    // Leer tarjeta RFID con cooldown entre lecturas
    if (millis() - lastCardRead >= CARD_COOLDOWN_MS) {
        String uid;
        if (readUid(uid)) {
            lastCardRead = millis();
            onCardRead(uid.c_str());
        }
    }

    // Intentar vaciar la cola de eventos pendientes si hay conexión
    flushQueue(getMqttClient());

    // Purge periódico de entradas expiradas en el caché
    if (millis() - lastCachePurge >= CACHE_PURGE_INTERVAL_MS) {
        cachePurgeExpired();
        lastCachePurge = millis();
    }

    // Heartbeat: confirma que el sistema está vivo y monitorea memoria
    if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
        Serial.printf("[Alive] uptime=%lus  mqtt=%s  heap=%u bytes libres\n",
                      millis() / 1000,
                      mqttConnected() ? "OK" : "OFFLINE",
                      ESP.getFreeHeap());
        lastHeartbeat = millis();
    }
}
