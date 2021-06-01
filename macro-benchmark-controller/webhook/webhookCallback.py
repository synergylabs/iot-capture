from flask import Flask
import queue

app = Flask(__name__)
q = queue.Queue()


@app.route('/register/')
def register_device():
    return 'ok'


@app.route('/command/')
def issue_command():
    res = q.qsize()
    if res > 0:
        q.get_nowait()
    return f'{res}'


@app.route('/ifttt/')
def ifttt_webhook():
    q.put(1)
    return 'Hello, World!'
