#!/usr/bin/env bash

# Set up FreeRadius
sudo apt update -qq
sudo apt install -qq freeradius

# Install iptables
sudo apt install -qq iptables

# Updata freeradius EAP configuration
FREERADIUS_DIR="/etc/freeradius/3.0"
sudo sed -i 's/use_tunneled_reply = no/use_tunneled_reply = yes/' "${FREERADIUS_DIR}/mods-enabled/eap"

echo "Change the shared secret in ${FREERADIUS_DIR}/clients.conf where secret = testing123!!"
