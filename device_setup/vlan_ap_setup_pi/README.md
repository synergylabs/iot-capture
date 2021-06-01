# Installation

First of all we need to switch from `dnsmasq` to a more complicated DHCP server. Instructions: https://www.tecmint.com/install-dhcp-server-in-ubuntu-debian/

```bash
sudo apt install isc-dhcp-server
```

Go edit the following files
```
# /etc/default/isc-dhcp-server
INTERFACES="wlan1"
```

```
# /etc/dhcp/dhcpd.conf
option domain-name "<replace your domain>";
option domain-name-servers 8.8.8.8;

default-lease-time 600;
max-lease-time 7200;

subnet 192.168.5.0 netmask 255.255.255.0 {
  range 192.168.5.100 192.168.5.200;
  option routers 192.168.5.1;
  option subnet-mask 255.255.255.0;
  option domain-search "<replace your domain>";
  option domain-name-servers 8.8.8.8;
}
```

# Operation Procedure

During the new device connection, it is very important to do the following things step-by-step.

- Connect new client to WPA2-EAP AP
- Automically: a new VLAN is created, named after wlan1.X where X is VLAN-ID
- Manually assign IP address block to the wlan1.X by
```bash
sudo ifconfig wlan1.X 192.168.xxx.1 netmask 255.255.255.0
```
- Add the corresponding wlan1.X interface to
```
# /etc/default/isc-dhcp-server
INTERFACES="<existing-entries> wlan1.X"
```
- Add the corresponding IP subnet to dhcpd configuration
```
# /etc/dhcp/dhcpd.conf

subnet 192.168.110.0 netmask 255.255.255.0 {
  range 192.168.110.100 192.168.110.200;
  option routers 192.168.5.1;
  option subnet-mask 255.255.255.0;
  option domain-search "<replace your domain>";
  option domain-name-servers 8.8.8.8;
}
```
- Restart isc-dhcp-server
```bash
sudo systemctl restart isc-dhcp-server
```
