-- Schema para el sistema de control de acceso RFID
-- Aplicar con: CREATE IF NOT EXISTS (idempotente, seguro en cada arranque)

CREATE TABLE IF NOT EXISTS users (
    uid        TEXT PRIMARY KEY,           -- UID hex normalizado en MAYÚSCULAS (ej: "A1B2C3D4")
    name       TEXT NOT NULL,
    schedule   TEXT DEFAULT NULL,          -- JSON o NULL. Ej: {"days":[1,2,3,4,5],"from":"08:00","to":"18:00"}
                                           -- days: 0=dom,1=lun,...,6=sab. NULL = sin restricción horaria.
    expires_at TEXT DEFAULT NULL,          -- ISO 8601 UTC o NULL = no expira. Ej: "2025-12-31T23:59:59Z"
    enabled    INTEGER NOT NULL DEFAULT 1, -- 1=habilitado, 0=deshabilitado
    created_at TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
    updated_at TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))
);

CREATE TABLE IF NOT EXISTS audit (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    ts         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
    actor      TEXT NOT NULL,   -- username de Telegram o "system"
    action     TEXT NOT NULL,   -- 'create'/'enable'/'disable'/'delete'/'set_schedule'/'set_expiry'
                                -- 'lockdown_on'/'lockdown_off'/'set_ttl'/'invalidate'
    target_uid TEXT,            -- NULL para acciones globales (lockdown, ttl)
    details    TEXT             -- JSON con info adicional (nombre, schedule, etc.)
);

CREATE INDEX IF NOT EXISTS idx_audit_ts ON audit(ts DESC);
CREATE INDEX IF NOT EXISTS idx_audit_uid ON audit(target_uid);
