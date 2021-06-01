#!/usr/bin/env python3
import sys
import socket
from threading import Thread, Lock
from struct import pack, unpack
from http.server import BaseHTTPRequestHandler,HTTPServer

NO_TYPE = 0
GET_TEMP = 1
TEMP_VAL = 2
GET_HUM = 3
HUM_VAL = 4

HEADER_LENGTH = 8

TESTING = False

temperature = 0
humidity = 0
HTML_PAGE = """<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP32 DHT Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i>
    <span class="dht-labels">Temperature</span>
    <span id="temperature">%s</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%s</span>
    <sup class="units">%%</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>"""

device_connection = None
http_port = 8088

class Message:
    def __init__(self, message):
        self.type = None
        self.value = None
        (self.type, self.value) = unpack('If', message)
        self.type = socket.ntohl(self.type)

    def construct_message(self):
        message = pack('If', socket.htonl(self.type), self.value)
        return message

class WebHTTPServer:
    def __init__(self, http_port):
        self.http_port = http_port
    
    def start_http_server(self):
        server = HTTPServer(('', self.http_port), THServerRequestHandler)
        server.serve_forever()

class THServerRequestHandler(BaseHTTPRequestHandler):

    def read_val(self):
        global device_connection
        msg = b''
        msg = device_connection.recv(HEADER_LENGTH)
        message = Message(msg)
        print(f"Recv message: type: {message.type}, value: {message.value}")
        return message

    def read_temp(self):
        global temperature
        msg = self.read_val()
        status = ""
        if msg.type != TEMP_VAL:
            status = f'Invalid value {msg.type} read instead of {TEMP_VAL} from driver'
        else:
            temperature = msg.value
            status = f'Successfully read temperature: {temperature}'
        return status

    def read_hum(self):
        global humidity
        msg = self.read_val()
        status = ""
        if msg.type != HUM_VAL:
            status = f'Invalid value {msg.type} read instead of {HUM_VAL} from driver'
        else:
            humidity = msg.value
            status = f'Successfully read humidity: {humidity}'
        return status
    
    def request_value(self, type):
        global device_connection
        message = Message(bytes(HEADER_LENGTH))
        message.type = type
        message.value = 0
        msg = message.construct_message()
        device_connection.sendall(msg)
        print("Sent request to device")

    def send_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
    
    def send_webpage(self, page_type):
        global temperature, humidity
        self.send_headers()
        webpage = ""
        if page_type == "complete":
            webpage = HTML_PAGE % (temperature, humidity)
        elif page_type == "temperature":
            webpage = str(temperature)
        elif page_type == "humidity":
            webpage = str(humidity)
        self.wfile.write(bytes(webpage, encoding='utf-8'))

    def do_GET(self):
        if self.path == '/':
            print("Processing request for /")
            self.request_value(GET_TEMP)
            self.read_temp()
            self.request_value(GET_HUM)
            self.read_hum()
            self.send_webpage("complete")
        elif self.path == '/temperature':
            print("Processing request for /temperature")
            self.request_value(GET_TEMP)
            status = self.read_temp()
            print(status)
            self.send_webpage("temperature")
        elif self.path == '/humidity':
            print("Processing request for /temperature")
            self.request_value(GET_HUM)
            status = self.read_hum()
            print(status)
            self.send_webpage("humidity")
        else:
            print("Processing request for undefined")
            pass
    
    def finish(self):
        print("Handled Request")
            
class THServer_Driver:
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

    def run_http_server(self, http_port, dev_conn):
        global device_connection
        device_connection = dev_conn
        
        web_http = WebHTTPServer(http_port)
        http_thread = Thread(target=web_http.start_http_server, args=())
        http_thread.start()
    
    def accept_device_connection(self):
        # while True:
        (conn, address) = self.device_socket.accept()
        self.device_connection = conn
        self.run_http_server(http_port, conn)


def main():
    if len(sys.argv) <= 2:
        print("Error number of arguments")
        print("Usage: ${1} <network_port_number> <optional-vlan-id>")
        return -1
    
    thserver_driver = THServer_Driver(int(sys.argv[1]), int(sys.argv[2]))
    

if __name__ == "__main__":
    main()