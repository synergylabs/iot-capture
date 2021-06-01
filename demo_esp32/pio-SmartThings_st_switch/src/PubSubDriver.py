#!/usr/bin/env python3

import pdb
import sys
import socket
from threading import Thread, Lock
from struct import pack, unpack
from http.server import BaseHTTPRequestHandler, HTTPServer
from time import sleep
import select

import paho.mqtt.client as mqtt


SUBSCRIBE = 1
EVENT_FOR_SUBSCRIBER = 2
PUBLISH_EVENT = 3

HEADER_LENGTH = 12

DEVICE_CAP = 10

device_socket = None
mqtt_driver = None
device_connection = None


def on_message(client, userdata, message):
    print("received.")
    print(f"message received {message.payload}")
    print("message topic=", message.topic)
    print("message qos=", message.qos)
    print("message retain flag=", message.retain)

    device_msg = Message(bytes(HEADER_LENGTH))
    device_msg.type = EVENT_FOR_SUBSCRIBER
    device_msg.topic = message.topic
    device_msg.topic_length = len(device_msg.topic)
    device_msg.message = message.payload.decode("utf-8")
    device_msg.message_length = len(device_msg.message)
    global device_connection
    print(f"messge: {device_msg.construct_message()}")
    device_connection.sendall(device_msg.construct_message())


class Message:
    def __init__(self, message):
        self.type = None
        self.topic_length = 0
        self.message_length = 0
        self.topic = b''
        self.message = b''
        if message != None and message != '':
            (self.type, self.topic_length,
             self.message_length) = unpack('III', message)
            self.type = socket.ntohl(self.type)
            self.topic_length = socket.ntohl(self.topic_length)
            self.message_length = socket.ntohl(self.message_length)

    def construct_message(self):
        payload = pack('III', socket.htonl(self.type), socket.htonl(
            self.topic_length), socket.htonl(self.message_length))
        if self.topic_length:
            payload = payload + self.topic.encode("utf-8")
        if self.message_length:
            payload = payload + self.message.encode("utf-8")
        return payload


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    # client.subscribe("test/abc")

class MQTT_Driver:
    def __init__(self, device_port_number, vlan_id):
        self.device_connected = False
        global device_connection
        device_connection = None
        global device_socket
        device_socket = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        device_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # self.device_socket.bind(('0.0.0.0', 8188))
        device_socket.bind((f'192.168.{vlan_id}.1', device_port_number))
        device_socket.listen(1)

        # Set up MQTT broker connection
        self.paho_client = mqtt.Client("capture-pi-mqtt")
        self.paho_client.on_message = on_message
        self.paho_client.on_connect = on_connect
        self.paho_client.connect("192.168.11.1", keepalive=60)

        self.paho_client.subscribe("test/drivertest")

        self.paho_client.loop_start()

    def handle_message_from_device(self, header):
        msg = Message(header)

        print(
            f"Receive header {msg.type}, with length: {msg.topic_length} / {msg.message_length}")

        global device_connection
        if msg.topic_length > 0:
            print(f"Waiting for topic...")
            msg.topic = device_connection.recv(msg.topic_length)
        if msg.message_length > 0:
            print("waiting for message payload...")
            msg.message = device_connection.recv(msg.message_length)

        print("all received.")
        msg.topic = msg.topic.decode("utf-8")
        msg.message = msg.message.decode("utf-8")

        if msg.type == SUBSCRIBE:
            self.paho_client.subscribe(msg.topic)
            pass
        elif msg.type == EVENT_FOR_SUBSCRIBER:
            print("Probably shouldn't receive this type of message from device.")
            pass
        elif msg.type == PUBLISH_EVENT:
            self.paho_client.publish(msg.topic, msg.message)
            pass
        else:
            # Unknown type
            print("unknown message type")

    def accept_device_connection(self):
        # while True:
        (conn, _) = device_socket.accept()
        global device_connection
        device_connection = conn

        while True:
            header = device_connection.recv(HEADER_LENGTH)
            # import pdb; pdb.set_trace()
            self.handle_message_from_device(header)


def main():
    if len(sys.argv) <= 2:
        print("Error number of arguments")
        print("Usage: ${1} <network_port_number> <optional-vlan-id>")
        return -1

    global mqtt_driver
    mqtt_driver = MQTT_Driver(int(sys.argv[1]), int(sys.argv[2]))
    mqtt_driver.device_socket_listener_thread = Thread(
        target=mqtt_driver.accept_device_connection, args=())
    mqtt_driver.device_socket_listener_thread.start()


if __name__ == "__main__":
    main()
