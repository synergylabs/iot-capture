import socket
import datetime
from threading import Thread, Lock, Condition

notification_port = 54321
notification_error_timeout = 60


# Create a feedback listener at notification_port
class WemoNotificationController(object):
    def __init__(self, listen_port=notification_port):
        self.cv = Condition()
        self.ready = False
        self.start_time = None
        self.notification_time = None
        self.device_socket = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        self.device_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.device_socket.bind(('0.0.0.0', listen_port))
        self.device_socket.listen(1)
        self.device_socket_listener_thread = Thread(target=self.accept_device_connection, args=())
        self.device_socket_listener_thread.start()

    def accept_device_connection(self):
        while True:
            (conn, address) = self.device_socket.accept()
            payload = conn.recv(1).decode('utf-8')
            print(f'receive data payload {payload}')
            self.cv.acquire()
            self.notification_time = datetime.datetime.now()
            if not self.ready:
                self.ready = True
            self.cv.notify()
            self.cv.release()

    def register_start_time(self, start_time):
        self.cv.acquire()
        self.start_time = start_time
        self.cv.release()

    def wait_for_notification(self):
        self.cv.acquire()
        if not self.ready:
            self.cv.wait(timeout=notification_error_timeout)
        if not self.ready:
            print('error')
        self.ready = False
        self.cv.release()

    def get_notification_time(self):
        self.cv.acquire()
        res = self.notification_time
        self.notification_time = None
        self.cv.release()
        return res


if __name__ == '__main__':
    test = WemoNotificationController()
