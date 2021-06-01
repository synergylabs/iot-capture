sudo ifconfig wlan1.99 192.168.99.1/24

sudo sed -i '$s/$/\nsubnet 192.168.99.0 netmask 255.255.255.0 {\n\trange 192.168.99.100 192.168.99.200;\n\toption routers 192.168.99.1;\n\toption subnet-mask 255.255.255.0;\n\toption domain-search \"capture-pi.andrew.cmu.edu\";\n\toption domain-name-servers 8.8.8.8;\n}/' "/etc/dhcp/dhcpd.conf"

sudo sed -i '/INTERFACES=.*/s/\"$/ wlan1.99\"/' "/etc/default/isc-dhcp-server"

sudo systemctl restart isc-dhcp-server


sudo brctl delif brvlan99 wlan1.99
sudo ifconfig brvlan99 down
sudo brctl delbr brvlan99
