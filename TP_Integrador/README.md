# Sistema Inteligente de Control de Acceso Híbrido

**Murray Agustín · Polanis Iván Valentín · Savenia Manuel**  
TP Integrador — I161 IoT

## Arquitectura

```
ESP32 + RC522 ──MQTT auth──► Mosquitto ──► Node-RED
                                              ├─ SQLite  (padrón usuarios)
                                              ├─ InfluxDB (telemetría)
                                              └─ Telegram Bot (admin)
                                                       │
                                                  Grafana (dashboards)
```

El ESP32 usa un **caché local** (LittleFS) como primera línea de verificación en cada lectura de tarjeta. Solo ante un cache miss consulta al servidor por MQTT. El servidor (Node-RED) es la única fuente de verdad y evalúa `enabled`, `expires_at` y `schedule` al responder.

## Requisitos

- Docker + Docker Compose
- PlatformIO (VS Code extension o CLI)
- Cuenta de Telegram para el bot

---

## Setup del stack servidor

### 1. Copiar y completar credenciales

```bash
cp .env.example .env
```

Editar `.env` con contraseñas reales para Mosquitto, el token del bot de Telegram y los IDs de chat de admins.

### 2. Generar el archivo de contraseñas de Mosquitto

El archivo `mosquitto/config/passwd` se genera con `mosquitto_passwd` (no se commitea):

```bash
# Crear archivo nuevo con el usuario esp32
docker run --rm -it eclipse-mosquitto:2 mosquitto_passwd -c /dev/stdout esp32
# Pegar el hash en mosquitto/config/passwd

# Agregar usuarios nodered y admin (sin -c para no sobrescribir)
docker run --rm -it eclipse-mosquitto:2 mosquitto_passwd -b /dev/stdout nodered TU_PASS_NODERED
docker run --rm -it eclipse-mosquitto:2 mosquitto_passwd -b /dev/stdout admin TU_PASS_ADMIN
```

Alternativa con mosquitto_passwd instalado localmente:

```bash
mosquitto_passwd -c mosquitto/config/passwd esp32
mosquitto_passwd mosquitto/config/passwd nodered
mosquitto_passwd mosquitto/config/passwd admin
```

### 3. Obtener el Token del Bot de Telegram

1. Hablar con [@BotFather](https://t.me/BotFather) en Telegram
2. Ejecutar `/newbot` y seguir las instrucciones
3. Copiar el token en `.env` → `TELEGRAM_TOKEN`

Para obtener tu chat ID, hablar con [@userinfobot](https://t.me/userinfobot) y copiar el `Id` en `TELEGRAM_ADMIN_IDS`.

### 4. Levantar el stack

```bash
docker compose up -d
```

Verificar que todos los servicios están saludables:

```bash
docker compose ps
```

Servicios disponibles:
- **Mosquitto**: `localhost:1883`
- **Node-RED**: `http://localhost:1880`
- **InfluxDB**: `http://localhost:8086`
- **Grafana**: `http://localhost:3000` (admin/admin por defecto)

### 5. Verificar MQTT (diagnóstico)

```bash
# Suscribirse a todos los topics de acceso
mosquitto_sub -h localhost -p 1883 -u admin -P TU_PASS_ADMIN -t 'access/#' -v

# En otra terminal, publicar un evento de prueba
mosquitto_pub -h localhost -p 1883 -u admin -P TU_PASS_ADMIN \
  -t 'access/door01/event' \
  -m '{"uid":"AABBCCDD","outcome":"granted","source":"test","ts":"2025-01-01T00:00:00Z"}'
```

---

## Setup del firmware ESP32

### 1. Crear config.h

```bash
cp esp32/src/config.example.h esp32/src/config.h
```

Editar `esp32/src/config.h`:
- `MQTT_HOST`: IP de la máquina donde corre Docker
- `MQTT_USER` / `MQTT_PASS`: credenciales del usuario `esp32` definidas en el paso anterior

### 2. Build y upload

```bash
cd esp32

# Compilar
pio run

# Subir filesystem LittleFS (vacío al inicio)
pio run -t uploadfs

# Subir firmware
pio run -t upload

# Monitor serial
pio device monitor
```

### 3. Provisioning WiFi

Al primer arranque (o si no hay WiFi guardada), el ESP32 levanta un AP llamado `ESP32-AccessControl`. Conectarse y configurar la red WiFi desde el portal cautivo en `http://192.168.4.1`.

---

## Comandos del Bot de Telegram

| Comando | Descripción |
|---|---|
| `/alta <uid> <nombre>` | Dar de alta un usuario |
| `/baja <uid>` | Deshabilitar usuario (invalida caché) |
| `/habilitar <uid>` | Habilitar usuario deshabilitado |
| `/deshabilitar <uid>` | Deshabilitar temporalmente |
| `/usuario <uid>` | Ver datos de un usuario |
| `/horario <uid> <dias_csv> <HH:MM> <HH:MM>` | Configurar franja horaria permitida |
| `/expira <uid> <ISO8601\|none>` | Configurar/quitar fecha de expiración |
| `/ultimos [n]` | Ver últimos n eventos (default 10) |
| `/lockdown on\|off` | Bloqueo global de accesos |
| `/ttl <segundos>` | Cambiar TTL del caché en el ESP32 |
| `/ayuda` | Ver todos los comandos |

### Ejemplo de horario

```
/horario AABBCCDD 1,2,3,4,5 08:00 18:00
```
Permite acceso de lunes (1) a viernes (5) entre las 08:00 y las 18:00.  
`days`: 0=domingo, 1=lunes, ..., 6=sábado.

### Ejemplo de expiración

```
/expira AABBCCDD 2025-12-31T23:59:59Z
/expira AABBCCDD none    # quita expiración
```

---

## Topics MQTT

| Topic | Dirección | Descripción |
|---|---|---|
| `access/door01/event` | ESP32 → Server | Telemetría de cada acceso (`granted`/`denied`) |
| `access/door01/request` | ESP32 → Server | Cache miss: solicita validación |
| `access/door01/response` | Server → ESP32 | Respuesta del servidor con `allowed` y `ttl` |
| `access/door01/command/invalidate` | Server → ESP32 | Borrar UID(s) del caché |
| `access/door01/command/lockdown` | Server → ESP32 | Activar/desactivar bloqueo global |
| `access/door01/command/config` | Server → ESP32 | Actualizar parámetros (TTL, etc.) |
| `access/door01/status` | ESP32 → Server (retained) | Heartbeat y Last Will (offline detection) |

---

## Flujo de decisión del ESP32

```
Leer tarjeta RFID
        │
        ▼
¿Lockdown activo? ──sí──► Denegar + encolar evento
        │ no
        ▼
¿UID en caché y no expirado? ──sí──► Abrir relé + encolar evento (source: cache)
        │ no
        ▼
¿MQTT conectado? ──no──► Denegar + encolar evento (source: offline_miss)
        │ sí
        ▼
Publicar request → esperar response (timeout 1.5s)
        │
   ┌────┴────┐
  sí       no/timeout
   │           │
Abrir +    Denegar +
encolar    encolar
(source:   (source:
server)    server_timeout)
```

---

## Estructura del proyecto

```
TP_Integrador/
├── docker-compose.yml
├── .env.example
├── .gitignore
├── mosquitto/
│   └── config/         mosquitto.conf, acl, passwd (no en repo)
├── nodered/
│   ├── Dockerfile
│   ├── data/
│   │   ├── flows.json  5 tabs: Init, CacheMiss, Events, Status, Telegram
│   │   └── package.json
│   └── scripts/schema.sql
├── grafana/
│   ├── provisioning/   datasources + dashboards
│   └── dashboards/accesos.json
└── esp32/
    ├── platformio.ini
    └── src/
        ├── main.cpp
        ├── config.example.h  (copiar a config.h y completar)
        ├── wifi_mgr.h/.cpp
        ├── mqtt_client.h/.cpp
        ├── rfid_reader.h/.cpp
        ├── access_cache.h/.cpp  (LittleFS, TTL configurable)
        ├── event_queue.h/.cpp   (cola offline persistente)
        ├── actuators.h/.cpp     (relé, LEDs)
        └── access_logic.h/.cpp  (FSM principal)
```
