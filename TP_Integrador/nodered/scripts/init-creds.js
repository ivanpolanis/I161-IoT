#!/usr/bin/env node
/**
 * Generates flows_cred.json from environment variables using the same
 * AES-256-CTR encryption that Node-RED uses internally.
 * Must run before node-red starts so credentials are always in sync with .env.
 */
const crypto = require('crypto');
const fs = require('fs');

const secret = process.env.NODE_RED_CREDENTIAL_SECRET;
if (!secret) {
    console.error('[init-creds] ERROR: NODE_RED_CREDENTIAL_SECRET is not set');
    process.exit(1);
}

const telegramToken = process.env.TELEGRAM_TOKEN || '';
const mqttUser = process.env.MOSQ_USER || '';
const mqttPass = process.env.MOSQ_PASS || '';

// Remove the system-generated _credentialSecret so Node-RED uses credentialSecret
// from settings.js directly without attempting a key migration.
const runtimeConfigPath = '/data/.config.runtime.json';
if (fs.existsSync(runtimeConfigPath)) {
    try {
        const cfg = JSON.parse(fs.readFileSync(runtimeConfigPath, 'utf8'));
        if (cfg._credentialSecret) {
            delete cfg._credentialSecret;
            fs.writeFileSync(runtimeConfigPath, JSON.stringify(cfg, null, 4));
            console.log('[init-creds] removed stale _credentialSecret from runtime config');
        }
    } catch (e) {
        console.warn('[init-creds] could not patch runtime config:', e.message);
    }
}

const creds = {};
if (mqttUser) creds['config-mqtt'] = { user: mqttUser, password: mqttPass };
if (telegramToken) creds['config-telegram'] = { token: telegramToken };

const key = crypto.createHash('sha256').update(secret).digest();
const iv = crypto.randomBytes(16);
const cipher = crypto.createCipheriv('aes-256-ctr', key, iv);

const encrypted = cipher.update(JSON.stringify(creds), 'utf8', 'base64') + cipher.final('base64');

fs.writeFileSync(
    '/data/flows_cred.json',
    JSON.stringify({ '$': iv.toString('hex') + encrypted }, null, 4)
);

console.log('[init-creds] flows_cred.json written (mqtt=' + !!mqttUser + ', telegram=' + !!telegramToken + ')');
