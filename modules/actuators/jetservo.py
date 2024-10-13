import Jetson.GPIO as GPIO
from time import sleep
from pubsub import pub

class JetsonServo:
    def __init__(self, pin, range, **kwargs):
        self.pin = pin
        self.range = range
        self.start = kwargs.get('start_pos', 0)
        self.setup_gpio()
        self.servo = GPIO.PWM(pin, 50)  # GPIO pin 18, 50Hz
        self.servo.start(self.start)
        pub.subscribe(self.move, 'jetservo:move')

    def setup_gpio(self):
        GPIO.setmode(GPIO.BCM)  # Set GPIO numbering mode to BCM
        GPIO.setup(self.pin, GPIO.OUT)  # Set pin as output

    def move(self, angle):
        duty = self.angle_to_duty(angle)
        self.servo.ChangeDutyCycle(duty)
        sleep(1)  # @TODO: Remove this sleep

    def angle_to_duty(self, angle):
        duty = angle / 18 + 2  # Map angle (0 to 180) to duty cycle (2 to 12)
        return duty

