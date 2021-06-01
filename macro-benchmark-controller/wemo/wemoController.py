import requests


class WemoController(object):
    def __init__(self, bind="0.0.0.0:10085"):
        bind = bind.split(':')
        self.ip = bind[0]
        self.port = bind[1]

    def discoverSwitch(self):
        try:
            req = requests.get(url=f"http://{self.ip}:{self.port}/")
        except requests.ConnectionError:
            return None
        return req

    def setState(self, state):
        return self.setSwitchState(state)

    def setSwitchState(self, state):
        if state != 0 and state != 1:
            return False
        requests.get(url=f"http://{self.ip}:{self.port}/26/"
                         f"{'on' if state == 1 else 'off'}")
        return True

    def turnoffSwitch(self):
        return self.setSwitchState(0)

    def turnonSwitch(self):
        return self.setSwitchState(1)
