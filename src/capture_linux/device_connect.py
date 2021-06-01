#!/usr/bin/env python3
import os
import hydra
import string
import random
import socket
from omegaconf import DictConfig
from struct import pack, unpack
import netifaces
import time

CAPTURE_MSG_NO_TYPE = 0
CAPTURE_MSG_EAP_CREDS = 1
CAPTURE_MSG_EAP_REQUEST = 2
EAP_CREDS_HEADER_LENGTH = 16

ENCODING = 'utf-8'


class EAPCredsMsg:
    def __init__(self):
        pass


def random_string(string_length=10):
    """Generate a random string of fixed length """
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for i in range(string_length))


def stop_wpa_supplicant():
    os.system("sudo killall wpa_supplicant")


def recv_all(sock, msg_size):
    total = b''
    while len(total) < msg_size:
        buf = sock.recv(msg_size - len(total))
        total += buf
    return total


@hydra.main(config_path="device_config.yaml")
def main(cfg: DictConfig):
    # Join PSK network
    psk_ssid = cfg['psk_ssid']
    psk_pass = cfg['psk_pass']
    psk_config_filename = os.path.join('/tmp', random_string())
    psk_config = open(psk_config_filename, 'w')
    psk_config.write(f"""
        network={{
            ssid="{psk_ssid}"
            psk="{psk_pass}"
        }}
    """)
    psk_config.close()

    stop_wpa_supplicant()

    os.system(f"sudo wpa_supplicant -B -i {cfg['wlan_iface']} -c {psk_config_filename}")
    os.system("sudo dhclient")
    os.system("echo 'wait for 5 seconds' && sleep 5")

    # # Install dependency
    # os.system("sudo apt update -qq")
    # os.system("sudo apt install -y chromium-browser")

    # Get credentials and join EAP network
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    hub_ip = '192.168.11.1'
    hub_port = 8080
    s.connect((hub_ip, hub_port))

    # Retrieve EAP credentials
    driver_name = 'DEFAULT_MAGIC_MIRROR_DRIVER'
    request_msg = pack('II', socket.htonl(CAPTURE_MSG_EAP_REQUEST),
                       socket.htonl(len(driver_name))) + driver_name.encode(ENCODING)
    s.send(request_msg)

    payload = recv_all(s, EAP_CREDS_HEADER_LENGTH)
    print(f"length of payload: {len(payload)}, content: {payload}")
    response_msg = EAPCredsMsg()
    (response_msg.msg_type, response_msg.driver_port, response_msg.username_len, response_msg.password_len) = \
        unpack('IIII', payload)
    # network order conversion
    response_msg.msg_type = socket.ntohl(response_msg.msg_type)
    response_msg.driver_port = socket.ntohl(response_msg.driver_port)
    response_msg.username_len = socket.ntohl(response_msg.username_len)
    response_msg.password_len = socket.ntohl(response_msg.password_len)
    response_msg.username = s.recv(response_msg.username_len)
    response_msg.password = s.recv(response_msg.password_len)

    addrs = netifaces.ifaddresses(cfg['wlan_iface'])
    psk_ip = addrs[netifaces.AF_INET][0]['broadcast']

    stop_wpa_supplicant()

    eap_config_filename = os.path.join('/tmp', random_string())
    eap_config = open(eap_config_filename, 'w')
    eap_config.write(f"""
    network={{
      ssid="{cfg['eap_ssid']}"
      scan_ssid=1
      key_mgmt=WPA-EAP
      identity="{response_msg.username.decode(ENCODING)}"
      password="{response_msg.password.decode(ENCODING)}"
      eap=PEAP
      phase1="peaplabel=0"
      phase2="auth=MSCHAPV2"
    }}
    """)
    eap_config.close()
    os.system(f"sudo wpa_supplicant -B -i {cfg['wlan_iface']} -c {eap_config_filename}")
    os.system("sudo dhclient")
    os.system("echo 'wait for 5 seconds' && sleep 5")

    # Retrieve driver IP and port number
    broadcast_ip = psk_ip
    while broadcast_ip == psk_ip:
        time.sleep(1)
        addrs = netifaces.ifaddresses(cfg['wlan_iface'])
        broadcast_ip = addrs[netifaces.AF_INET][0]['broadcast']
    # replace the last 255 of broadcast ip to 1
    driver_ip = '.'.join(broadcast_ip.split('.')[:3]) + '.1'
    driver_port = response_msg.driver_port
    
    return
    print(f"Starting chromium browser now for http://{driver_ip}:{driver_port}, have patience, it takes a minute")
    os.system(f"chromium-browser -noerrdialogs -kiosk -start_maximized --app=http://{driver_ip}:{driver_port}"
              " --disable-infobars --ignore-certificate-errors-spki-list --ignore-ssl-errors"
              "--ignore-certificate-errors 2>/dev/null")


if __name__ == '__main__':
    main()
