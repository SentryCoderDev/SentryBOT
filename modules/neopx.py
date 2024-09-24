from pubsub import pub
from time import sleep
from colour import Color
try:
    from modules.config import Config
except Exception as e:
    print("Importing config directly")
    from config import Config
import board

# Import necessary libraries based on the chosen method
if Config.get('neopixel', 'i2c'):
    print("Using I2C configuration")
    import busio
    from rainbowio import colorwheel
    from adafruit_seesaw import seesaw, neopixel
    connection_type = 'i2c'  # Set the connection type to I2C
elif Config.get('neopixel', 'usb'):
    print("Using USB configuration")
    import neopixel
    connection_type = 'usb'  # Set the connection type to USB
else:
    print("Using GPIO configuration")
    import neopixel
    connection_type = 'gpio'  # Set the connection type to GPIO

class NeoPx:
    COLOR_OFF = (0, 0, 0)
    COLOR_RED = (100, 0, 0)
    COLOR_GREEN = (0, 100, 0)
    COLOR_BLUE = (0, 0, 100)
    COLOR_PURPLE = (100, 0, 100)
    COLOR_WHITE = (100, 100, 100)
    COLOR_WHITE_FULL = (255, 255, 255)
    COLOR_WHITE_DIM = (50, 50, 50)
    COLOR_RED_TO_GREEN_100 = list(Color("red").range_to(Color("green"), 100))
    COLOR_BLUE_TO_RED_100 = list(Color("blue").range_to(Color("red"), 100))
    COLOR_BLUE_TO_GREEN_100 = list(Color("blue").range_to(Color("green"), 100))

    COLOR_MAP = {
        'red': COLOR_RED,
        'green': COLOR_GREEN,
        'blue': COLOR_BLUE,
        'purple': COLOR_PURPLE,
        'white': COLOR_WHITE,
        'white_full': COLOR_WHITE_FULL,
        'off': COLOR_OFF,
        'white_dim': COLOR_WHITE_DIM
    }

    def __init__(self, count, **kwargs):
        self.count = count
        self.connection_type = connection_type  # Correctly assign connection type
        self.positions = Config.get('neopixel', 'positions')
        self.brightness = Config.get('neopixel', 'brightness')
        self.all = range(self.count)
        self.all_eye = ['right', 'top_right', 'top_left', 'left', 'bottom_left', 'bottom_right', 'middle']
        self.ring_eye = ['right', 'top_right', 'top_left', 'left', 'bottom_left', 'bottom_right']
        self.animation = False
        self.thread = None
        self.overridden = False

        # Initialize connection based on connection type
        if self.connection_type == 'i2c':
            self.i2c = busio.I2C(board.SCL, board.SDA)
            self.ss = seesaw.Seesaw(self.i2c, addr=0x60)  # Use self.ss to store the seesaw instance
            neo_pin = 15
            self.pixels = neopixel.NeoPixel(self.ss, neo_pin, count, brightness=0.1)
        elif self.connection_type in ['usb', 'gpio']:
            self.pixels = neopixel.NeoPixel(board.D12, count)

        # Default states
        self.set(self.all, NeoPx.COLOR_OFF)
        sleep(0.1)
        self.set(self.positions['middle'], NeoPx.COLOR_BLUE)

        # Set subscribers
        pub.subscribe(self.set, 'led')
        pub.subscribe(self.full, 'led:full')
        pub.subscribe(self.eye, 'led:eye')
        pub.subscribe(self.ring, 'led:ring')
        pub.subscribe(self.off, 'led:off')
        pub.subscribe(self.eye, 'led:flashlight')
        pub.subscribe(self.party, 'led:party')
        pub.subscribe(self.exit, 'exit')
        pub.subscribe(self.speech, 'speech')

    def exit(self):
        if self.animation:
            self.animation = False
            self.thread.join()
        self.set(self.all, NeoPx.COLOR_OFF)
        if hasattr(self, 'i2c'):
            self.i2c.deinit()  # De-initialize only if it was initialized
        sleep(1)

    def speech(self, msg):
        if 'light on' in msg:
            self.flashlight(True)
        if 'light off' in msg:
            self.flashlight(False)

    def set(self, identifiers, color, gradient=False):
        if self.overridden:
            return
        if isinstance(identifiers, int):
            identifiers = [identifiers]
        elif isinstance(identifiers, str):
            identifiers = [self.positions[identifiers]]
        if isinstance(color, float):
            color = int(color)
        if isinstance(color, int):
            if color >= 100:
                color = 99
            if gradient == 'br':
                color = NeoPx.COLOR_BLUE_TO_RED_100[color].rgb
            elif gradient == 'bg':
                color = NeoPx.COLOR_BLUE_TO_GREEN_100[color].rgb
            else:
                color = NeoPx.COLOR_RED_TO_GREEN_100[color].rgb
            color = (round(color[0] * 100), round(color[1] * 100), round(color[2] * 100))
        elif isinstance(color, str):
            color = NeoPx.COLOR_MAP[color]
        for i in identifiers:
            if isinstance(i, str):
                i = self.positions[i]
            try:
                if i >= self.count:
                    pub.sendMessage('log', msg='[LED] Error in set pixels: index out of range')
                    print('Error in set pixels: index out of range')
                    i = self.count - 1
                self.pixels[i] = self.apply_brightness_modifier(i, color)
            except Exception as e:
                print(e)
                pub.sendMessage('log', msg='[LED] Error in set pixels: ' + str(e))
                pass
        
        self.pixels.show()
        sleep(0.1)

    def apply_brightness_modifier(self, identifier, color):
        return (round(color[0] * self.brightness[identifier]), 
                round(color[1] * self.brightness[identifier]), 
                round(color[2] * self.brightness[identifier]))

    def ring(self, color):
        self.set(self.ring_eye, color)

    def flashlight(self, on):
        if on:
            self.set(self.all_eye, NeoPx.COLOR_WHITE_FULL)
            self.overridden = True
        else:
            self.overridden = False
            self.set(self.all_eye, NeoPx.COLOR_OFF)

    def off(self):
        if self.thread:
            pub.sendMessage('log', msg='[LED] Animation stopping')
            self.animation = False
            self.thread.animation = False
            self.thread.join()
        self.set(self.all, NeoPx.COLOR_OFF)
        sleep(0.5)

    def full(self, color):
        if color in NeoPx.COLOR_MAP.keys():
            self.set(self.all, NeoPx.COLOR_MAP[color])

    def eye(self, color):
        if 'middle' not in self.positions:
            raise ValueError('Middle position not found')
        if color not in NeoPx.COLOR_MAP.keys():
            raise ValueError('Color not found')
        index = self.positions['middle']
        self.set(index, NeoPx.COLOR_MAP[color])


    def party(self):
        # self.animate(self.all, 'off', 'rainbow_cycle')

        for j in range(256 * 1):
            for i in range(self.count):
                self.set(i, NeoPx._wheel((int(i * 256 / self.count) + j) & 255))
            return
        print('done')

        # threading.Thread(target=self.rainbow_cycle(self.all, 'off'))
        # self.thread.start()

    def animate(self, identifiers, color, animation):
        """
        Trigger one of the LED animations
        :param identifiers: single index or array or indexes
        :param color: string map of COLOR_MAP or tuple (R, G, B)
        :param animation: string name of animation listed in map below
        :return:
        """
        if self.animation:
            pub.sendMessage('log', msg='[LED] Animation already started. Command ignored')
            return

        pub.sendMessage('log', msg='[LED] Animation starting: ' + animation)

        animations = {
            'spinner': self.spinner,
            'breathe': self.breathe,
            'rainbow': self.rainbow,
            'rainbow_cycle': self.rainbow_cycle
        }

        self.animation = True
        if animation in animations:
            self.thread = threading.Thread(target=animations[animation], args=(identifiers, color,))
        self.thread.start()


    def spinner(self, identifiers, color, index=1):
        """
        Create a spinner effect around outer LEDs of 7 LED ring.
        :param color: string map of COLOR_MAP or tuple (R, G, B)
        :param index: current LED to change
        :return:
        """
        sleep(.3)

        self.set(range(1, 7), NeoPx.COLOR_OFF)
        self.set(index, color)

        index = (index + 1) % self.count
        # don't set the center led
        if index == 0:
            index = 1

        t = threading.currentThread()
        if getattr(t, "animation", True):
            self.spinner(identifiers, color, index)

    def breathe(self, identifiers, color):
        """
        Begin a breathing animation for whatever color has been passed in.

        Assume the max is the brightest value from the tuple.
        All values > 0 will become aligned to that max value.
        E.g. (0, 100, 255) will brighten to (0, 255, 255) and then darken to (0, 0, 0) repeatedly

        :param identifiers: pins to apply animation
        :param color: string map of COLOR_MAP or tuple (R, G, B)
        """
        if type(color) is str:
            color = NeoPx.COLOR_MAP[color]
        t = threading.currentThread()
        if getattr(t, "animation", True):
            for dc in range(0, max(color), 1):  # Increase brightness to max of color
                self.set(identifiers, (dc if color[0] > 0 else 0, dc if color[0] > 0 else 0, dc if color[0] > 0 else 0))
                sleep(0.10)
            sleep(2)
            for dc in range(max(color), 0, -1):  # Decrease brightness to 0
                self.set(identifiers, (dc if color[0] > 0 else 0, dc if color[0] > 0 else 0, dc if color[0] > 0 else 0))
                sleep(0.10)
            sleep(2)

    @staticmethod
    def _wheel(p):
        """Generate rainbow colors across 0-255 positions."""
        # https://github.com/jgarff/rpi_ws281x/blob/master/python/examples/strandtest.py
        if p < 85:
            return (p * 3, 255 - p * 3, 0)
        elif p < 170:
            p -= 85
            return (255 - p * 3, 0, p * 3)
        else:
            p -= 170
            return (0, p * 3, 255 - p * 3)

    def rainbow(self, identifiers, color, wait_ms=20, iterations=1):
        """Draw rainbow that fades across all pixels at once."""
        for j in range(256 * iterations):
            for i in range(self.count):
                self.set(i, NeoPx._wheel((i + j) & 255))
            t = threading.currentThread()
            if not getattr(t, "animation", True):
                return
            sleep(wait_ms / 1000)

    def rainbow_cycle(self, identifiers, color, wait_ms=20, iterations=5):
        """Draw rainbow that uniformly distributes itself across all pixels."""
        for j in range(256 * iterations):
            for i in range(self.count):
                self.set(i, NeoPx._wheel((int(i * 256 / self.count) + j) & 255))
            t = threading.currentThread()
            if not getattr(t, "animation", True):
                return
            sleep(wait_ms / 1000)



if __name__ == '__main__':
    inst = NeoPx(12)
    inst.set(inst.all, 1)
    sleep(2)
    inst.set(inst.all, 50)
    sleep(2)
    inst.set(inst.all, 100)
    sleep(2)
    inst.set(inst.all, NeoPx.COLOR_RED)
    sleep(2)
    inst.set(inst.all, NeoPx.COLOR_GREEN)
    sleep(2)
    inst.set(inst.all, NeoPx.COLOR_BLUE)
    sleep(2)
    inst.set(inst.all, NeoPx.COLOR_OFF)
    sleep(2)
    inst.flashlight(True)
    sleep(2)
    inst.flashlight(False)
