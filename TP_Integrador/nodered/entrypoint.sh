#!/bin/sh
set -e
node /scripts/init-creds.js
exec node-red --userDir /data "$@"
