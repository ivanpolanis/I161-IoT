#include "event_queue.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>

static std::vector<AccessEvent> queue;

static void saveQueue() {
    File f = LittleFS.open(QUEUE_FILE, "w");
    if (!f) return;
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const AccessEvent& ev : queue) {
        JsonObject obj = arr.add<JsonObject>();
        obj["uid"]     = ev.uid;
        obj["outcome"] = ev.outcome;
        obj["source"]  = ev.source;
        obj["ts"]      = ev.ts;
    }
    serializeJson(doc, f);
    f.close();
}

void initQueue() {
    File f = LittleFS.open(QUEUE_FILE, "r");
    if (!f) return;

    JsonDocument doc;
    if (deserializeJson(doc, f) != DeserializationError::Ok) { f.close(); return; }
    f.close();

    for (JsonObject obj : doc.as<JsonArray>()) {
        AccessEvent ev{};
        strncpy(ev.uid,     obj["uid"]     | "", sizeof(ev.uid)     - 1);
        strncpy(ev.outcome, obj["outcome"] | "", sizeof(ev.outcome) - 1);
        strncpy(ev.source,  obj["source"]  | "", sizeof(ev.source)  - 1);
        strncpy(ev.ts,      obj["ts"]      | "", sizeof(ev.ts)      - 1);
        queue.push_back(ev);
    }
    Serial.printf("[Queue] Cargados %d eventos pendientes.\n", (int)queue.size());
}

void enqueueEvent(const AccessEvent& ev) {
    if ((int)queue.size() >= MAX_QUEUE_EVENTS) {
        Serial.println("[Queue] Cola llena. Descartando evento más antiguo.");
        queue.erase(queue.begin());
    }
    queue.push_back(ev);
    saveQueue();
}

void flushQueue(PubSubClient& client) {
    if (queue.empty() || !client.connected()) return;

    // Publicar de a 5 eventos por llamada para no bloquear el loop
    int flushed = 0;
    while (!queue.empty() && flushed < 5) {
        const AccessEvent& ev = queue.front();

        JsonDocument doc;
        doc["uid"]     = ev.uid;
        doc["outcome"] = ev.outcome;
        doc["source"]  = ev.source;
        doc["ts"]      = ev.ts;

        char payload[192];
        serializeJson(doc, payload, sizeof(payload));

        if (!client.publish(TOPIC_EVENT, payload, false)) {
            Serial.println("[Queue] Falló publicación. Reintentará luego.");
            break;
        }
        queue.erase(queue.begin());
        flushed++;
    }

    if (flushed > 0) {
        saveQueue();
        Serial.printf("[Queue] Flush: %d eventos publicados.\n", flushed);
    }
}
