#!/usr/bin/env python3
import sys
import socket
from threading import Thread, Lock
from struct import pack, unpack
from http.server import BaseHTTPRequestHandler, HTTPServer

NO_TYPE = 0
GPIO_26_OFF = 1
GPIO_26_ON = 2
GPIO_27_OFF = 3
GPIO_27_ON = 4

HEADER_LENGTH = 4

TESTING = False

device_connection = None
http_port = 8087


class Message:
    def __init__(self, message):
        self.type = None
        self.type = unpack('I', message)[0]
        self.type = socket.ntohl(self.type)

    def construct_message(self):
        message = pack('I', socket.htonl(self.type))
        return message


class WebServerDriver(object):
    def __init__(self, device_port_number, vlan_id):
        self.device_connected = False
        self.device_connection = None
        self.device_socket = None
        self.device_socket = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        self.device_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # self.device_socket.bind(('0.0.0.0', 8188))
        self.device_socket.bind((f'192.168.{vlan_id}.1', device_port_number))
        self.device_socket.listen(1)
        self.device_socket_listener_thread = Thread(target=self.accept_device_connection, args=())
        self.device_socket_listener_thread.start()

    # For derived class to overwrite
    def run_http_server(self, http_port, dev_conn):
        raise NotImplementedError()

    def accept_device_connection(self):
        # while True:
        (conn, address) = self.device_socket.accept()
        self.device_connection = conn
        self.run_http_server(http_port, conn)
