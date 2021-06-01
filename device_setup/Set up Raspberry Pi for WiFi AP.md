# Status

Outdated. This document is used to set up standard WPA-2 PSK wifi access point on RPi 3. As we have moved forward to more advanced AP mode, this document is obsolete. This document has no information on how to set up VLANs as well as the WPA-EAP mode.

# Useful Links

https://thepi.io/how-to-use-your-raspberry-pi-as-a-wireless-access-point/

https://www.shellvoide.com/wifi/setup-wireless-access-point-hostapd-dnsmasq-linux/

# Steps

1. Install hostapd and dnsmasq

```bash
sudo apt install hostapd
sudo apt install dnsmasq
```

```bash
sudo systemctl stop hostapd
sudo systemctl stop dnsmasq
```

2. Deny interfaces

Add this line to the bottom of the file.

```vim
# /etc/dhcpcd.conf

denyinterfaces wlan0
```

3. Configure the DHCP server (dnsmasq)

```vim
# /etc/dnsmasq.conf

interface=wlan0
listen-address=192.168.5.1
bind-interfaces
server=8.8.8.8
domain-needed
bogus-priv
dhcp-range=192.168.5.100,192.168.5.200,24h
```

4. Configure the access point host software (hostapd)

```vim
# /etc/hostapd/hostapd.conf

interface=wlan0
hw_mode=g
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
ssid=NETWORK_NAME
wpa_passphrase=PASSWORD
```

Remember to replace `NETWORK_NAME` and `PASSWORD` accordingly.

```vim
# /etc/default/hostapd

DAEMON_CONF="/etc/hostapd/hostapd.conf"
```

5. Set up traffic forwarding

Modify the following line in the file.

```vim
# /etc/sysctl.conf

net.ipv4.ip_forward=1
```

6. Add a new iptables rule

```bash
sudo iptables --table nat --append POSTROUTING --out-interface eth0 -j MASQUERADE
sudo iptables --append FORWARD --in-interface wlan0 -j ACCEPT
```

Then save the new iptables rule:
```bash
sudo sh -c "iptables-save > /etc/iptables.ipv4.nat"
```

Edit the file to load the new rules upon start.
```vim
# /etc/rc.local

iptables-restore < /etc/iptables.ipv4.nat
```

Definitely add the line **before** `exit 0`!!

7. Configure rc.local to run on start



8. Configure network interfaces

```vim
# /etc/network/interfaces

# interfaces(5) file used by ifup(8) and ifdown(8)

# Please note that this file is written to be used with dhcpcd
# For static IP, consult /etc/dhcpcd.conf and 'man dhcpcd.conf'

# Include files from /etc/network/interfaces.d:
source-directory /etc/network/interfaces.d

auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp

allow-hotplug wlan0
iface wlan0 inet static
    address 192.168.5.1
    netmask 255.255.255.0
    network 192.168.5.0
    broadcast 192.168.5.255
```
