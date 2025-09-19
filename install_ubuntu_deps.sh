#!/bin/bash
# Script to install the necessary dependencies for Gurotopia on Debian-based systems (like Ubuntu).

# Update package list and install dependencies
sudo apt-get update && sudo apt-get install -y \
    build-essential \
    libssl-dev \
    openssl \
    sqlite3 \
    libsqlite3-dev

echo "All necessary dependencies have been installed."
