import Jetson.GPIO as GPIO
from time import sleep
from pubsub import pub

class JetsonMotionSensor:
  def __init__(self, pin, **kwargs):
    self.pin = pin
    self.value = None
    GPIO.setup(self.pin, GPIO.IN)  # Set pin as input

  def loop(self):
    if self.read():
      pub.sendMessage('motion')
    sleep(0.1)  # Adjust delay as needed

  def read(self):
    self.value = GPIO.input(self.pin)
    return self.value

