#pragma once
#include <Arduino.h>
#include <PubSubClient.h>

struct AccessEvent {
    char uid[15];     // max 7 bytes × 2 hex = 14 + null
    char outcome[8];  // "granted" | "denied"
    char source[20];  // "cache" | "server" | "offline_miss" | "lockdown" | "server_timeout"
    char ts[24];      // "T+Xs" uptime
};

inline AccessEvent makeEvent(const char* uid, const char* outcome, const char* source, const char* ts) {
    AccessEvent ev{};
    strncpy(ev.uid,     uid,     sizeof(ev.uid)     - 1);
    strncpy(ev.outcome, outcome, sizeof(ev.outcome) - 1);
    strncpy(ev.source,  source,  sizeof(ev.source)  - 1);
    strncpy(ev.ts,      ts,      sizeof(ev.ts)      - 1);
    return ev;
}

void initQueue();
void enqueueEvent(const AccessEvent& ev);
void flushQueue(PubSubClient& client);
