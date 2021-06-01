#!/usr/bin/env python3

import sys
import socket
from threading import Thread, Lock
from struct import pack, unpack
import time
import pdb

GET_CLIENT = 1
INGRESS = 2
EGRESS = 3
CHECK_STATUS = 4
CLOSE_CLIENT = 5

HEADER_LENGTH = 8

DEVICE_CAP = 10
CLIENT_CAP = 10

DEVICE_BUFFER_SIZE = 4096


TESTING = False

class Message_Header:
    def __init__(self, message):
        # Unpack the contents of the header and store it
        self.type = None
        self.client_id = None
        self.piggyback_opt = None
        self.explicit_close = None
        self.length = None
        if len(message) != 8:
            print(f"Buffer len: {len(message)}")
        (self.type, self.client_id, self.piggyback_opt, self.explicit_close, self.length ) = unpack('BBBBI', message)
        self.length = socket.ntohl(self.length)
        self.payload = b''

    def construct_message(self):
        # Pack the contents of the message so that it can be sent over
        if self.length:
            message = pack('BBBBI', self.type, self.client_id, self.piggyback_opt, self.explicit_close, socket.htonl(self.length)) + self.payload
        else:
            message = pack('BBBBI', self.type, self.client_id, self.piggyback_opt, self.explicit_close, self.length)
        return message

class Client_Meta:
    def __init__(self, conn):
        self.connection = conn
        self.is_serviced = False

class Capture_Driver:
    def __init__(self, listening_port, ip_subnet, web_server_port):
        self.device_counter = 0
        self.device_connections = [None] * (DEVICE_CAP + 1)

        self.client_counter = 1
        self.client_list = [None] * (CLIENT_CAP + 1)

        # self.client_to_be_served = 1

        self.lock = Lock()

        self.idle_device_socket = None

        self.web_server_port = web_server_port

        self.device_socket = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        self.device_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.device_socket.bind(('0.0.0.0', listening_port))
        self.device_socket.listen(1)

        self.device_socket_listener_thread = Thread(target=self.accept_device_connection, args=())
        self.device_socket_listener_thread.start()
        pass

    def handle_message_from_device(self, device_socket, header, msg):
        message = Message_Header(bytes(HEADER_LENGTH))
        self.lock.acquire()
        if header.type == GET_CLIENT:
            # Return the ID of the next client that is waiting to be serviced.
            # The return message will have type = GET_CLIENT, client_id = ID,
            # length = 0, payload = 0. The return message is written onto the
            # device socket.
            message.type = GET_CLIENT
            message.client_id = 0
            count = 0
            while count < CLIENT_CAP:
                count += 1
                if (self.client_list[count] and
                    not self.client_list[count].is_serviced):
                    message.client_id = count

                    # print(f"Dispatch device to serve client {message.client_id}")

                    self.client_list[count].is_serviced = True

                    client_meta = self.client_list[message.client_id]
                    client_socket = client_meta.connection
                    client_data = client_socket.recv(DEVICE_BUFFER_SIZE)
                    message.length = len(client_data)
                    if message.length > 0:
                        message.piggyback_opt = 1
                        message.payload = client_data
                    break

            # Read the payload bytes of data so that the buffer is cleared
            if header.length > 0:
                device_socket.recv(header.length)

            if message.client_id == 0:
                # No available client. Put device connection to wait.
                self.idle_device_socket = device_socket
            else:
                bytes_to_write = message.construct_message()
                self.write_data(device_socket, bytes_to_write)

        elif header.type == INGRESS:
            # Read the specified number of bytes from the particular client.
            # The return message will have type = INGRESS, client_id = ID,
            # length = length read from client, payload = data read from client.
            # The return message is written onto the device socket.
            message.type = INGRESS
            message.client_id = header.client_id
            client_meta = self.client_list[header.client_id]
            if client_meta:
                client_socket = client_meta.connection
                client_data = client_socket.recv(header.length)
                message.payload = client_data
                message.length = len(client_data)
            # Read the payload bytes of data so that the buffer is cleared
            # device_socket.recv(header.length)
            bytes_to_write = message.construct_message()
            self.write_data(device_socket, bytes_to_write)
        elif header.type == EGRESS:
            # Write the specified number of bytes onto the particular client.
            # The return message will have type = EGRESS, client_id = ID,
            # length = length read from device, payload = data read from device.
            # The return message is written onto the client socket.
            client_meta = self.client_list[header.client_id]
            if client_meta:
                client_socket = client_meta.connection
                # print("Payload size: ", header.length)
                device_data = device_socket.recv(header.length)
                # print("Forwarding payload: ", device_data)
                # print("Actual payload size: ", len(device_data))
                self.write_data(client_socket, device_data)
        elif header.type == CHECK_STATUS:
            # Return the status of the specified client.
            # The message will have type = CHECK_STATUS, client_id = ID,
            # length = 1, payload = 1 byte representing True (1) or False (0)
            message.type = CHECK_STATUS
            message.client_id = header.client_id
            client_meta = self.client_list[header.client_id]
            client_connected = int((client_meta != None))
            message.length = 1
            message.payload = bytes([client_connected])
            # Read the payload bytes of data so that the buffer is cleared
            device_socket.recv(header.length)
            bytes_to_write = message.construct_message()
            self.write_data(device_socket, bytes_to_write)
        elif header.type == CLOSE_CLIENT:
            self.close_client(header.client_id)

        if header.explicit_close > 0:
            self.close_client(header.client_id)

        self.lock.release()

    def close_client(self, client_id):
        # print("Closing... ", client_id)
        # Close client connection with client_id
        if self.client_list[client_id]:
            self.client_list[client_id].connection.shutdown(socket.SHUT_RDWR)
            self.client_list[client_id].connection.close()
            self.client_list[client_id] = None

    def read_from_device(self, device_socket, message):
        pass

    def write_data(self, write_socket, message):
        write_socket.sendall(message)

    def accept_device_connection(self):
        while True:
            (conn, address) = self.device_socket.accept()
            self.lock.acquire()
            try:
                if self.device_connections[self.device_counter] != None:
                    # TODO: Remove existing connections to local device
                    pass
                self.device_connections[self.device_counter] = conn
                self.device_counter += 1
                self.device_counter %= DEVICE_CAP
            finally:
                self.lock.release()
            print("Beep")

            print('Connected to device', address)
            while True:
                # Communicate with the Device
                device_socket = self.device_connections[self.device_counter - 1]
                msg = device_socket.recv(HEADER_LENGTH)
                # print("Recv: ", msg)
                message_header = Message_Header(msg[:HEADER_LENGTH])
                self.handle_message_from_device(device_socket, message_header, msg)

        pass

    def get_driver_IP(self):
        pass

    def start_web_server(self):
        #create an INET, STREAMing socket
        serversocket = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        #bind the socket to a public host,
        # and a well-known port
        serversocket.bind(('0.0.0.0', self.web_server_port))
        #become a server socket
        serversocket.listen(5)

        print("Server started...")

        while True:
             #accept connections from outside
            (clientsocket, address) = serversocket.accept()
            #now do something with the clientsocket
            #in this case, we'll pretend this is a threaded server
            if clientsocket:
                # print('Connected by', address)
                # TODO: notify the local device that there is a new client connection via availiable()
                self.lock.acquire()
                try:
                    if self.client_list[self.client_counter] != None:
                        # TODO: Remove existing connections to local device
                        pass
                    client_id = self.client_counter
                    self.client_list[client_id] = Client_Meta(clientsocket)
                    self.client_counter = (self.client_counter % CLIENT_CAP) + 1

                    if self.idle_device_socket != None:
                        message = Message_Header(bytes(HEADER_LENGTH))
                        message.type = GET_CLIENT
                        message.client_id = client_id
                        self.client_list[message.client_id].is_serviced = True
                        client_meta = self.client_list[message.client_id]
                        client_socket = client_meta.connection
                        client_data = client_socket.recv(DEVICE_BUFFER_SIZE)
                        message.length = len(client_data)
                        if message.length > 0:
                            message.piggyback_opt = 1
                            message.payload = client_data
                        bytes_to_write = message.construct_message()
                        self.write_data(self.idle_device_socket, bytes_to_write)
                        # self.idle_device_socket = None
                finally:
                    self.lock.release()
                pass


def main():
    if len(sys.argv) != 4 and len(sys.argv) != 3:
        print("Error number of arguments")
        print("Usage: ${1} <network_port_number> <optional-vlan-id> <web-server-port-DEFAULT-8088>")
        return -1
    capture = Capture_Driver(int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]) if len(sys.argv) > 3 else 8088)
    capture.start_web_server()
    pass

if __name__ == "__main__":
    main()
