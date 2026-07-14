#!/usr/bin/env bash
# @note generate a self-signed TLS cert + key for local dev.
# Called automatically by Makefile if resources/ctx/server.key is missing.
set -eu

CERT_DIR="$(cd "$(dirname "$0")/.." && pwd)/resources/ctx"
KEY="$CERT_DIR/server.key"
CRT="$CERT_DIR/server.crt"

if [ -f "$KEY" ] && [ -f "$CRT" ]; then
    exit 0
fi

mkdir -p "$CERT_DIR"

openssl req -x509 -newkey rsa:2048 -keyout "$KEY" \
    -out "$CRT" -days 3650 -nodes \
    -subj "/CN=Gurotopia Dev Cert/O=Gurotopia/L=Local/C=XX" \
    -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"

echo "[gen-cert] generated self-signed cert: $CRT, $KEY"