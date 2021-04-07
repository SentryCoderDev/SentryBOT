from modules.arduinoserial import ArduinoSerial
from time import time, sleep
import subprocess
from pubsub import pub

# battery value logging
import datetime

class Battery:

    BATTERY_THRESHOLD = 670  # max 760 (12.6v), min 670 (10.5v)
    BATTERY_LOW = 690  # max 760 (12.6v), min 670 (10.5v)
    READING_INTERVAL = 60 # seconds

    def __init__(self, pin, serial, **kwargs):
        self.pin = pin
        self.serial = serial
        self.interval = time() # Don't trigger immediately because a false reading will shut the system down before we can stop the script
        pub.subscribe(self.loop, 'loop')

    def loop(self):
        if self.interval < time() - Battery.READING_INTERVAL:
            self.interval = time()
            val = self.check()
            if val == 0:
                pub.sendMessage('led:full', color='red')
                print('Battery read error!')
                return
            if self.low_voltage(val):
                pub.sendMessage('led:full', color='red')
                if not self.safe_voltage(val):
                    print("BATTERY WARNING! SHUTTING DOWN!")
                    pub.sendMessage('exit')
                    sleep(5)
                    subprocess.call(['shutdown', '-h'], shell=False)

    def check(self):
        val =  self.serial.send(ArduinoSerial.DEVICE_PIN_READ, 0, 0)
        print('bat:' + str(val))
        with open('/home/pi/really-useful-robot/battery.csv', 'a') as fd:
            fd.write(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S") + ', ' + str(val) + '\n')
        return val

    def low_voltage(self, val):
        if val < Battery.BATTERY_LOW:
            return True
        return False

    def safe_voltage(self, val):
        if val < Battery.BATTERY_THRESHOLD:
            return False
        return True
