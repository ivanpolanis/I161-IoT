#pragma once

// ─── Identidad del dispositivo ────────────────────────────────────────────────
#define DEVICE_ID "door01"

// ─── Pines RC522 (SPI) ────────────────────────────────────────────────────────
#define RC522_SS_PIN   5
#define RC522_RST_PIN  22
// SCK=18, MOSI=23, MISO=19 — pines SPI hardware del ESP32, no requieren define

// ─── Actuadores ───────────────────────────────────────────────────────────────
#define PIN_RELAY      26
#define PIN_LED_GREEN  25
#define PIN_LED_RED    27

// ─── Tiempos ──────────────────────────────────────────────────────────────────
#define RELAY_OPEN_MS        3000    // Tiempo que el relé permanece abierto
#define DEFAULT_CACHE_TTL_S  86400   // TTL por defecto del caché (24h)
#define SERVER_TIMEOUT_MS    1500    // Timeout esperando respuesta del servidor
#define MQTT_KEEPALIVE_S     30

// ─── Broker MQTT ──────────────────────────────────────────────────────────────
// Copiar este archivo a config.h y completar con datos reales
#define MQTT_HOST  "192.168.0.32"   // IP del host donde corre Docker
#define MQTT_PORT  1883
#define MQTT_USER  "esp32"
#define MQTT_PASS  "changeme_esp32"

// ─── Topics MQTT (derivados de DEVICE_ID) ─────────────────────────────────────
#define TOPIC_EVENT     "access/" DEVICE_ID "/event"
#define TOPIC_REQUEST   "access/" DEVICE_ID "/request"
#define TOPIC_RESPONSE  "access/" DEVICE_ID "/response"
#define TOPIC_CMD_INV   "access/" DEVICE_ID "/command/invalidate"
#define TOPIC_CMD_LOCK  "access/" DEVICE_ID "/command/lockdown"
#define TOPIC_CMD_CFG   "access/" DEVICE_ID "/command/config"
#define TOPIC_STATUS    "access/" DEVICE_ID "/status"

// ─── Almacenamiento LittleFS ──────────────────────────────────────────────────
#define CACHE_FILE        "/cache.json"
#define QUEUE_FILE        "/queue.json"
#define MAX_QUEUE_EVENTS  100
#define MAX_CACHE_ENTRIES 200
