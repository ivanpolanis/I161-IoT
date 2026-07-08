#!/bin/sh
set -e
mkdir -p /data/sqlite
node /scripts/init-creds.js
exec node-red --userDir /data "$@"
