#!/usr/bin/env bash

# Start the PSK network in the background
sudo hostapd -B /etc/hostapd/hostapd.conf.wpa.psk.conf

# Attach IP address to PSK interface
sudo ifconfig wlan2 192.168.11.1/24

# Reset default configuration files
sudo cp /etc/dhcp/dhcpd.conf.backup /etc/dhcp/dhcpd.conf
sudo cp /etc/default/isc-dhcp-server.backup /etc/default/isc-dhcp-server
sudo cp /etc/freeradius/3.0/mods-config/files/authorize.backup /etc/freeradius/3.0/mods-config/files/authorize

sudo systemctl restart freeradius

# Restart DHCP server to listen on wlan2
sudo systemctl restart isc-dhcp-server
