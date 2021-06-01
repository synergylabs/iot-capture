#!/usr/bin/env python3
import sys
import socket
from threading import Thread, Lock
from struct import pack, unpack
from http.server import BaseHTTPRequestHandler, HTTPServer
import requests
from time import sleep


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

syn_bd_host = "<server-name>"
syn_bd_uri = "command/"
https_port = 443

local_controller_address = "192.168.6.16"
local_controller_port = 54321

device_connection = None
http_port = 8099


class WebServer_Driver:
    def __init__(self):
        self.get_command()

    def get_command(self):
        sess = requests.Session()
        while True:
            sleep(0.1)
            r = sess.get(f"https://{syn_bd_host}/{syn_bd_uri}")
            if r.status_code != 200 or \
                    r.text == '0':
                continue

            # Send notification feedback to controller
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((local_controller_address, local_controller_port))
            s.send(b"7")


def main():
    WebServer_Driver()


if __name__ == "__main__":
    main()
