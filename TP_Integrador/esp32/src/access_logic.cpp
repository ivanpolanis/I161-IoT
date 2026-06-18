#include "access_logic.h"
#include "config.h"
#include "access_cache.h"
#include "actuators.h"
#include "event_queue.h"
#include "mqtt_client.h"
#include <ArduinoJson.h>
#include <time.h>

static bool     lockdownEnabled = false;
static uint32_t defaultTtl      = DEFAULT_CACHE_TTL_S;

struct PendingRequest {
    bool          active = false;
    char          uid[15];
    char          reqId[20];
    unsigned long sentAt = 0;
};
static PendingRequest pending;

static void makeReqId(char* buf, size_t len) {
    snprintf(buf, len, "%lu", millis());
}

static void currentIso(char* buf, size_t len) {
    time_t now = time(nullptr);
    if (now > 1577836800UL) { // > 2020: NTP ya sincronizó
        struct tm t;
        gmtime_r(&now, &t);
        strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", &t);
    } else {
        snprintf(buf, len, "T+%lus", millis() / 1000);
    }
}

void initAccessLogic() {
    lockdownEnabled = false;
    pending.active  = false;
    Serial.println("[Logic] Inicializado.");
}

void onCardRead(const char* uid) {
    if (pending.active) {
        Serial.println("[Logic] Lectura ignorada: hay solicitud pendiente.");
        return;
    }

    Serial.printf("[Logic] Tarjeta: %s\n", uid);

    if (lockdownEnabled) {
        denyAccess();
        char ts[20]; currentIso(ts, sizeof(ts));
        enqueueEvent(makeEvent(uid, "denied", "lockdown", ts));
        return;
    }

    CacheStatus status = cacheGet(uid);

    if (status == CacheStatus::HIT) {
        Serial.printf("[Logic] Cache HIT uid=%s\n", uid);
        grantAccess();
        char ts[20]; currentIso(ts, sizeof(ts));
        enqueueEvent(makeEvent(uid, "granted", "cache", ts));
        return;
    }

    if (status == CacheStatus::EXPIRED) {
        Serial.printf("[Logic] Cache EXPIRED uid=%s — revalidando.\n", uid);
        cacheInvalidate(uid);
    }

    if (!mqttConnected()) {
        Serial.println("[Logic] Sin MQTT. Denegando (offline miss).");
        denyAccess();
        char ts[20]; currentIso(ts, sizeof(ts));
        enqueueEvent(makeEvent(uid, "denied", "offline_miss", ts));
        return;
    }

    char reqId[20]; makeReqId(reqId, sizeof(reqId));

    JsonDocument doc;
    doc["uid"]    = uid;
    doc["req_id"] = reqId;
    doc["ts"]     = (const char*)reqId; // uptime como timestamp
    char payload[128];
    serializeJson(doc, payload, sizeof(payload));

    if (!mqttPublish(TOPIC_REQUEST, payload)) {
        Serial.println("[Logic] Falló publicación del request. Denegando.");
        denyAccess();
        char ts[20]; currentIso(ts, sizeof(ts));
        enqueueEvent(makeEvent(uid, "denied", "server_timeout", ts));
        return;
    }

    pending.active = true;
    strncpy(pending.uid,   uid,   sizeof(pending.uid)   - 1);
    strncpy(pending.reqId, reqId, sizeof(pending.reqId) - 1);
    pending.uid[sizeof(pending.uid) - 1]     = '\0';
    pending.reqId[sizeof(pending.reqId) - 1] = '\0';
    pending.sentAt = millis();
    Serial.printf("[Logic] Request enviado req_id=%s\n", reqId);
}

void onServerResponse(const char* payload) {
    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) {
        Serial.println("[Logic] Response malformado.");
        return;
    }

    const char* reqId   = doc["req_id"];
    const char* uid     = doc["uid"];
    bool        allowed = doc["allowed"];
    uint32_t    ttl     = doc["ttl"] | defaultTtl;

    if (!pending.active || strcmp(pending.reqId, reqId) != 0) {
        Serial.println("[Logic] Response para req desconocido — ignorado.");
        return;
    }

    pending.active = false;
    char ts[20]; currentIso(ts, sizeof(ts));

    if (allowed) {
        cachePut(uid, ttl);
        grantAccess();
        enqueueEvent(makeEvent(uid, "granted", "server", ts));
        Serial.printf("[Logic] GRANTED uid=%s ttl=%u\n", uid, ttl);
    } else {
        denyAccess();
        enqueueEvent(makeEvent(uid, "denied", "server", ts));
        Serial.printf("[Logic] DENIED uid=%s\n", uid);
    }
}

void onCommandInvalidate(const char* payload) {
    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return;

    if (doc["all"].as<bool>()) {
        cachePurgeAll();
        Serial.println("[Logic] Invalidar TODO el caché.");
    } else {
        const char* uid = doc["uid"];
        if (uid) {
            cacheInvalidate(uid);
            Serial.printf("[Logic] Invalidar uid=%s\n", uid);
        }
    }
}

void onCommandLockdown(const char* payload) {
    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return;

    lockdownEnabled = doc["enabled"].as<bool>();
    Serial.printf("[Logic] Lockdown: %s\n", lockdownEnabled ? "ACTIVADO" : "desactivado");
    if (lockdownEnabled) lockdownIndicate();
}

void onCommandConfig(const char* payload) {
    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return;

    if (doc["cache_ttl_s"].is<uint32_t>()) {
        defaultTtl = doc["cache_ttl_s"].as<uint32_t>();
        cacheUpdateAllTtl(defaultTtl);
        Serial.printf("[Logic] Nuevo TTL default = %us\n", defaultTtl);
    }
}

void accessLogicTick() {
    if (pending.active && (millis() - pending.sentAt) > SERVER_TIMEOUT_MS) {
        Serial.printf("[Logic] Timeout uid=%s\n", pending.uid);
        pending.active = false;
        denyAccess();
        char ts[20]; currentIso(ts, sizeof(ts));
        enqueueEvent(makeEvent(pending.uid, "denied", "server_timeout", ts));
    }
}
