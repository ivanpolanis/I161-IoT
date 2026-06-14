#pragma once

void initActuators();
void grantAccess();      // Abre relé + LED verde por RELAY_OPEN_MS
void denyAccess();       // Parpadeo LED rojo (no bloquea el loop)
void lockdownIndicate(); // LED rojo fijo mientras esté en lockdown
void actuatorsTick();    // Llamar en cada iteración del loop para manejar timings
