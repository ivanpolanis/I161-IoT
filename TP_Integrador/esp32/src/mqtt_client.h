#pragma once
#include <PubSubClient.h>

// Inicializa el cliente MQTT con los callbacks de respuesta y comandos.
void initMqtt();

// Llamar en cada iteración del loop. Gestiona reconexión con backoff exponencial.
void mqttTick();

// Devuelve si hay conexión activa con el broker.
bool mqttConnected();

// Publica un payload JSON en un topic dado.
bool mqttPublish(const char* topic, const char* payload, bool retain = false);

// Expone el cliente para que event_queue pueda usarlo en flush.
PubSubClient& getMqttClient();
