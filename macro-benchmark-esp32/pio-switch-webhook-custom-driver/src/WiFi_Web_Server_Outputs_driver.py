#!/usr/bin/env python3
import sys
import socket
from threading import Thread, Lock
from struct import pack, unpack
from http.server import BaseHTTPRequestHandler, HTTPServer
import requests

NO_TYPE = 0
GPIO_26_OFF = 1
GPIO_26_ON = 2
GPIO_27_OFF = 3
GPIO_27_ON = 4

HEADER_LENGTH = 4

TESTING = False

GPIO_26_STATUS = "off"
GPIO_27_STATUS = "off"

GPIO_26_ON_PAGE = "<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>"
GPIO_26_OFF_PAGE = "<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>"
GPIO_27_ON_PAGE = "<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>"
GPIO_27_OFF_PAGE = "<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>"
HTML_PAGE = """<!DOCTYPE html><html>
<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
<link rel=\"icon\" href=\"data:,\">
<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
.button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;
text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}
.button2 {background-color: #555555;}</style></head>
<body><h1>ESP32 Web Server</h1>
<p>GPIO 26 - State %s</p>
%s
<p>GPIO 27 - State %s</p>
%s
</body></html>"""

IFTTT_URL = "https://maker.ifttt.com/trigger/append_to_sheet/with/key/cUaZNrS7U0wAsZOzakzVUe"
syn_bd_host = "<server-name>"
syn_bd_uri = "command/"
https_port = 443

local_controller_address = "192.168.5.250"
local_controller_port = 54321

device_connection = None
http_port = 8099


class Message:
    def __init__(self, message):
        self.type = None
        self.type = unpack('I', message)[0]
        self.type = socket.ntohl(self.type)

    def construct_message(self):
        message = pack('I', socket.htonl(self.type))
        return message


class WebServer_Driver:
    def __init__(self, device_port_number, vlan_id):
        self.device_connected = False
        self.device_connection = None
        self.device_socket = None
        self.device_socket = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        self.device_socket.setsockopt(
            socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.device_socket.bind((f'192.168.{vlan_id}.1', device_port_number))
        self.device_socket.listen(1)
        self.device_socket_listener_thread = Thread(
            target=self.accept_device_connection, args=())
        self.device_socket_listener_thread.start()
        self.device_socket_listener_thread.join()

    def accept_device_connection(self):
        # while True:
        (conn, address) = self.device_socket.accept()
        self.device_connection = conn
        self.get_command()

    def get_command(self):
        while True:
            r = requests.get(f"https://{syn_bd_host}/{syn_bd_uri}")
            if r.status_code != 200 or \
                    r.text == '0':
                continue
            # Turn on switch
            message = Message(bytes(HEADER_LENGTH))
            message.type = GPIO_26_ON
            msg = message.construct_message()
            self.device_connection.sendall(msg)
            # Send notification feedback to controller
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((local_controller_address, local_controller_port))
            s.send(b"7")


def main():
    if len(sys.argv) <= 2:
        print("Error number of arguments")
        print("Usage: ${1} <network_port_number> <optional-vlan-id>")
        return -1

    webserver_driver = WebServer_Driver(int(sys.argv[1]), int(sys.argv[2]))


if __name__ == "__main__":
    main()
