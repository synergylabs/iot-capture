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
    # print("received.")
    # print(f"message received {message.payload}")
    # print("message topic=", message.topic)
    # print("message qos=", message.qos)
    # print("message retain flag=", message.retain)

    mqtt_driver.paho_client_private.publish(message.topic, message.payload)

    # device_msg = Message(bytes(HEADER_LENGTH))
    # device_msg.type = EVENT_FOR_SUBSCRIBER
    # device_msg.topic = message.topic
    # device_msg.topic_length = len(device_msg.topic)
    # device_msg.message = message.payload.decode("utf-8")
    # device_msg.message_length = len(device_msg.message)
    # global device_connection
    # print(f"messge: {device_msg.construct_message()}")
    # device_connection.sendall(device_msg.construct_message())


def on_message_private(client, userdata, message):
    mqtt_driver.paho_client_public.publish(message.topic, message.payload)


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))


class MQTT_Driver:
    def __init__(self, device_port_number, vlan_id):
        # self.device_connected = False
        # global device_connection
        # device_connection = None
        # global device_socket
        # device_socket = socket.socket(
        #     socket.AF_INET, socket.SOCK_STREAM)
        # device_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # # self.device_socket.bind(('0.0.0.0', 8188))
        # device_socket.bind((f'192.168.{vlan_id}.1', device_port_number))
        # device_socket.listen(1)

        # Set up MQTT broker connection
        self.paho_client_public = mqtt.Client("capture-pi-mqtt")
        self.paho_client_public.on_message = on_message
        self.paho_client_public.on_connect = on_connect
        self.paho_client_public.connect("192.168.11.1", keepalive=60)

        self.paho_client_public.subscribe("test/drivertest")
        self.paho_client_public.subscribe("switch")

        self.paho_client_public.loop_start()

        # Set up MQTT broker connection
        self.paho_client_private = mqtt.Client("capture-pi-mqtt-internal")
        self.paho_client_private.on_message = on_message_private
        self.paho_client_private.on_connect = on_connect
        self.paho_client_private.connect("192.168.11.1", port=51883, keepalive=60)
        self.paho_client_private.subscribe("feedback")

        self.paho_client_private.loop_start()

        pass

def main():
    if len(sys.argv) <= 2:
        print("Error number of arguments")
        print("Usage: ${1} <network_port_number> <optional-vlan-id>")
        return -1

    global mqtt_driver
    mqtt_driver = MQTT_Driver(int(sys.argv[1]), int(sys.argv[2]))
    while True:
        sleep(1000)
    pass

if __name__ == "__main__":
    main()
