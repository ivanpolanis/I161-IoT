#pragma once
#include <Arduino.h>

void initRfid();

// Devuelve true y escribe el UID en hex mayúsculas si hay una tarjeta presente.
// Retorna false si no hay tarjeta o si hubo un error de lectura.
bool readUid(String& uid);
