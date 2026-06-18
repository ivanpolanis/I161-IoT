#pragma once
#include <Arduino.h>

void initAccessLogic();

void onCardRead(const char* uid);

void onServerResponse(const char* payload);
void onCommandInvalidate(const char* payload);
void onCommandLockdown(const char* payload);
void onCommandConfig(const char* payload);

void accessLogicTick();
