FROM nodered/node-red:latest

# Instalar nodos adicionales en el directorio del sistema de Node-RED.
# NO usar /data porque es un volumen montado en runtime — lo que se instale
# ahí durante el build queda oculto al montar ./nodered/data:/data.
RUN npm install --prefix /usr/src/node-red \
    node-red-contrib-influxdb@~0.7.0 \
    node-red-node-sqlite@~2.0.1 \
    node-red-contrib-telegrambot@~17.4.13

COPY scripts/init-creds.js /scripts/init-creds.js
COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
