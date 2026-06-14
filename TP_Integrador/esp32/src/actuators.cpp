#include "actuators.h"
#include "config.h"
#include <Arduino.h>

static unsigned long relayOffAt    = 0;
static unsigned long denyBlinkAt   = 0;
static uint8_t       denyBlinkCnt  = 0;
static bool          ledRedState   = false;

void initActuators() {
    pinMode(PIN_RELAY,     OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_RED,   OUTPUT);
    digitalWrite(PIN_RELAY,     LOW);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED,   LOW);
#ifdef PIN_BUZZER
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
#endif
    Serial.println("[Actuators] Inicializados.");
}

void grantAccess() {
    digitalWrite(PIN_RELAY,     HIGH);
    digitalWrite(PIN_LED_GREEN, HIGH);
    digitalWrite(PIN_LED_RED,   LOW);
    relayOffAt   = millis() + RELAY_OPEN_MS;
    denyBlinkCnt = 0;
    Serial.println("[Actuators] Acceso concedido — relé abierto.");
}

void denyAccess() {
    // 3 parpadeos del LED rojo sin bloquear el loop
    denyBlinkCnt = 6; // 3 ciclos on/off
    denyBlinkAt  = millis();
    Serial.println("[Actuators] Acceso denegado.");
}

void lockdownIndicate() {
    digitalWrite(PIN_LED_RED,   HIGH);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_RELAY,     LOW);
}

void actuatorsTick() {
    const unsigned long now = millis();

    // Cerrar relé + apagar LED verde al vencer el tiempo
    if (relayOffAt && now >= relayOffAt) {
        digitalWrite(PIN_RELAY,     LOW);
        digitalWrite(PIN_LED_GREEN, LOW);
        relayOffAt = 0;
    }

    // Parpadeo LED rojo (denyAccess)
    if (denyBlinkCnt > 0 && now >= denyBlinkAt) {
        ledRedState = !ledRedState;
        digitalWrite(PIN_LED_RED, ledRedState ? HIGH : LOW);
        denyBlinkAt = now + 150;
        denyBlinkCnt--;
        if (denyBlinkCnt == 0) {
            digitalWrite(PIN_LED_RED, LOW);
            ledRedState = false;
        }
    }
}
