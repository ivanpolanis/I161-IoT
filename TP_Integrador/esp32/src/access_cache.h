#pragma once
#include <Arduino.h>

enum class CacheStatus { HIT, MISS, EXPIRED };

void initCache();

CacheStatus cacheGet(const char* uid);
void        cachePut(const char* uid, uint32_t ttlSeconds);
void        cacheInvalidate(const char* uid);
void        cachePurgeAll();
void        cachePurgeExpired();
