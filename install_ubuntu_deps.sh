#!/bin/bash
# Script to install the necessary dependencies for Gurotopia on Debian-based systems (like Ubuntu).

echo "Adding PPA for updated GCC toolchain..."
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test

echo "Updating package list..."
sudo apt-get update

echo "Installing dependencies, including g++-12..."
sudo apt-get install -y \
    build-essential \
    g++-12 \
    libssl-dev \
    openssl \
    sqlite3 \
    libsqlite3-dev

echo "All necessary dependencies have been installed."
