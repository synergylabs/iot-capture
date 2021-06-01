#!/usr/bin/env python3
import sys
import socket
from threading import Thread, Lock
from struct import pack, unpack
from http.server import BaseHTTPRequestHandler, HTTPServer
import requests

from capture_drvier_api import WebServerDriver, Message, HEADER_LENGTH, GPIO_26_ON, GPIO_26_OFF, GPIO_27_ON, GPIO_27_OFF

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
#IFTTT_URL = "https://maker.ifttt.com/trigger/turn_on_light/with/key/cUaZNrS7U0wAsZOzakzVUe"

dev_conn = None
http_port = 8088

class WebHTTPServer:
    def __init__(self, http_port):
        self.http_port = http_port

    def start_http_server(self):
        server = HTTPServer(('', self.http_port), WebServerRequestHandler)
        server.serve_forever()


def postIFTTT(gpio_number, state):
    payload = {'value1': str(gpio_number), 'value2': str(state)}
    requests.post(IFTTT_URL, data=payload)


class WebServerRequestHandler(BaseHTTPRequestHandler):

    def send_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def send_webpage(self):
        self.send_headers()
        if GPIO_26_STATUS == "off":
            gpio26page = GPIO_26_ON_PAGE
        else:
            gpio26page = GPIO_26_OFF_PAGE
        if GPIO_27_STATUS == "off":
            gpio27page = GPIO_27_ON_PAGE
        else:
            gpio27page = GPIO_27_OFF_PAGE
        webpage = HTML_PAGE % (GPIO_26_STATUS, gpio26page, GPIO_27_STATUS, gpio27page)
        print(webpage)
        self.wfile.write(bytes(webpage, encoding='utf-8'))

    def do_GET(self):
        global GPIO_26_STATUS, GPIO_27_STATUS
        if self.path == '/':
            print("Processing request for /")
            self.send_webpage()
        elif self.path == '/26/on':
            print("Processing request for /26/on")
            GPIO_26_STATUS = "on"
            self.send_webpage()

            # Forward command to device
            message = Message(bytes(HEADER_LENGTH))
            message.type = GPIO_26_ON
            msg = message.construct_message()
            dev_conn.sendall(msg)

            postIFTTT("26", "on")
        elif self.path == '/26/off':
            print("Processing request for /26/off")
            GPIO_26_STATUS = "off"
            self.send_webpage()

            # Forward command to device
            message = Message(bytes(HEADER_LENGTH))
            message.type = GPIO_26_OFF
            msg = message.construct_message()
            dev_conn.sendall(msg)
        elif self.path == '/27/on':
            print("Processing request for /27/on")
            GPIO_27_STATUS = "on"
            self.send_webpage()

            # Forward command to device
            message = Message(bytes(HEADER_LENGTH))
            message.type = GPIO_27_ON
            msg = message.construct_message()
            dev_conn.sendall(msg)

            postIFTTT("27", "on")
        elif self.path == '/27/off':
            print("Processing request for /27/off")
            GPIO_27_STATUS = "off"
            self.send_webpage()

            # Forward command to device
            message = Message(bytes(HEADER_LENGTH))
            message.type = GPIO_27_OFF
            msg = message.construct_message()
            dev_conn.sendall(msg)
        else:
            print("Processing request for undefined")
            pass

    def finish(self):
        print("Handled Request")


class SwitchDriver(WebServerDriver):
    def run_http_server(self, driver_port, dev_conn_in):
        global dev_conn, http_port
        dev_conn = dev_conn_in
        web_http = WebHTTPServer(http_port)
        http_thread = Thread(target=web_http.start_http_server, args=())
        http_thread.start()


def main():
    if len(sys.argv) <= 2:
        print("Error number of arguments")
        print("Usage: ${1} <network_port_number> <optional-vlan-id>")
        return -1

    d = SwitchDriver(int(sys.argv[1]), int(sys.argv[2]))


if __name__ == "__main__":
    main()
