import Jetson.GPIO as GPIO
from time import sleep
from pubsub import pub

class JetsonServo:
    def __init__(self, left_pin, right_pin, **kwargs):
        self.left_pin = left_pin
        self.right_pin = right_pin
        self.start = kwargs.get('start_pos', 0)
        self.setup_gpio()
        self.left_servo = GPIO.PWM(left_pin, 50)  # Left servo pin, 50Hz
        self.right_servo = GPIO.PWM(right_pin, 50)  # Right servo pin, 50Hz
        self.left_servo.start(self.start)
        self.right_servo.start(self.start)
        
        pub.subscribe(self.move, 'jetservo:move') # All jetservo movement
        pub.subscribe(self.invert_move, 'jetservo:inversermove')  # Inverse jetservo movement
        pub.subscribe(self.move_left, 'jetservo:left_move')  # Left jetservo
        pub.subscribe(self.move_right, 'jetservo:right_move')  # Right jetservo

    def setup_gpio(self):
        GPIO.setmode(GPIO.BCM)  # Set GPIO numbering mode to BCM
        GPIO.setup(self.left_pin, GPIO.OUT)  # Set left pin as output
        GPIO.setup(self.right_pin, GPIO.OUT)  # Set right pin as output

    def move(self, angle):
        left_duty = self.angle_to_duty(angle)
        right_duty = self.angle_to_duty(angle)
        self.left_servo.ChangeDutyCycle(left_duty)
        self.right_servo.ChangeDutyCycle(right_duty)
        sleep(1)  # @TODO: Remove this sleep

    def invert_move(self, angle):
        left_duty = self.angle_to_duty(angle)
        right_duty = self.angle_to_duty(180 - angle)  # Inverse movement for right servo
        self.left_servo.ChangeDutyCycle(left_duty)
        self.right_servo.ChangeDutyCycle(right_duty)
        sleep(1)  # @TODO: Remove this sleep

    def move_left(self, angle):
        duty = self.angle_to_duty(angle)
        GPIO.output(self.left_pin, GPIO.HIGH)  # Set left servo pin to HIGH
        self.left_servo.ChangeDutyCycle(duty)
        sleep(1)  # Wait for the servo to complete movement
        GPIO.output(self.left_pin, GPIO.LOW)  # Set left servo pin to LOW

    def move_right(self, angle):
        duty = self.angle_to_duty(angle)
        GPIO.output(self.right_pin, GPIO.HIGH)  # Set right servo pin to HIGH
        self.right_servo.ChangeDutyCycle(duty)
        sleep(1)  # Wait for the servo to complete movement
        GPIO.output(self.right_pin, GPIO.LOW)  # Set right servo pin to LOW

    def angle_to_duty(self, angle):
        duty = angle / 18 + 2  # Map angle (0 to 180) to duty cycle (2 to 12)
        return duty

    def cleanup(self):
        self.left_servo.stop()
        self.right_servo.stop()
        GPIO.cleanup()  # Clean up GPIO pins
