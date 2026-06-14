#include "access_cache.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>

static JsonDocument cacheDoc;

static void saveCache() {
    File f = LittleFS.open(CACHE_FILE, "w");
    if (!f) { Serial.println("[Cache] Error abriendo para escritura."); return; }
    serializeJson(cacheDoc, f);
    f.close();
}

void initCache() {
    File f = LittleFS.open(CACHE_FILE, "r");
    if (!f) {
        Serial.println("[Cache] Archivo no encontrado, iniciando caché vacío.");
        cacheDoc.clear();
        return;
    }
    DeserializationError err = deserializeJson(cacheDoc, f);
    f.close();
    if (err) {
        Serial.printf("[Cache] Error parseando cache.json: %s\n", err.c_str());
        cacheDoc.clear();
    } else {
        Serial.printf("[Cache] Cargado con %d entradas.\n", cacheDoc.size());
    }
}

CacheStatus cacheGet(const char* uid) {
    if (cacheDoc[uid].isNull()) return CacheStatus::MISS;

    uint32_t expiry = cacheDoc[uid].as<uint32_t>();
    uint32_t now    = (uint32_t)(millis() / 1000UL);

    if (expiry == 0 || now < expiry) return CacheStatus::HIT;
    return CacheStatus::EXPIRED;
}

void cachePut(const char* uid, uint32_t ttlSeconds) {
    if ((int)cacheDoc.size() >= MAX_CACHE_ENTRIES && cacheDoc[uid].isNull()) {
        Serial.println("[Cache] Límite alcanzado. Limpiando caché.");
        cacheDoc.clear();
    }
    uint32_t expiry = (uint32_t)(millis() / 1000UL) + ttlSeconds;
    cacheDoc[uid]   = expiry;
    saveCache();
    Serial.printf("[Cache] PUT uid=%s ttl=%us\n", uid, ttlSeconds);
}

void cacheInvalidate(const char* uid) {
    if (!cacheDoc[uid].isNull()) {
        cacheDoc.remove(uid);
        saveCache();
        Serial.printf("[Cache] Invalidado uid=%s\n", uid);
    }
}

void cachePurgeAll() {
    cacheDoc.clear();
    saveCache();
    Serial.println("[Cache] Caché limpiado completamente.");
}

void cachePurgeExpired() {
    uint32_t now = (uint32_t)(millis() / 1000UL);
    int removed  = 0;

    std::vector<String> toRemove;
    for (JsonPair kv : cacheDoc.as<JsonObject>()) {
        if (kv.value().as<uint32_t>() != 0 && now >= kv.value().as<uint32_t>())
            toRemove.push_back(kv.key().c_str());
    }
    for (const String& key : toRemove) {
        cacheDoc.remove(key.c_str());
        removed++;
    }

    if (removed > 0) {
        saveCache();
        Serial.printf("[Cache] Purge: %d entradas expiradas eliminadas.\n", removed);
    }
}
