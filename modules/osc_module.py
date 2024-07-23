import json
import os
from pubsub import pub
from oscpy.server import OSCThreadServer
from time import sleep

ANIMATION_DIR = 'modules/animations'

def load_animation(animation_name):
    """Load animation data from a JSON file."""
    file_path = os.path.join(ANIMATION_DIR, f'{animation_name}.json')
    if not os.path.isfile(file_path):
        print(f"Animation file '{file_path}' does not exist.")
        return None
    with open(file_path, 'r') as file:
        return json.load(file)

def handle_prediction(left, right):
    """Determine which animation to execute based on prediction values."""
    if left > 0.6:
        animation_name = 'Llft'  # Load 'Llft' animation for left
    elif right > 0.6:
        animation_name = 'Rlift'  # Load 'Rlift' animation for right
    else:
        animation_name = 'lower'  # Load 'lower' animation

    # Send the animation action to the Animate class via PubSub
    pub.sendMessage('animate', action=animation_name)

def start_osc_server():
    """Initialize and start the OSC server to handle messages."""
    def callback(left, right):
        """Handle OSC messages and execute animations based on predictions."""
        print("Left prediction : ", round(left, 2))
        print("Right prediction : ", round(right, 2))
        handle_prediction(left, right)

    osc = OSCThreadServer()
    osc.listen(address='127.0.0.1', port=9002, default=True)
    osc.bind(b'/neuropype', callback)

    print("OSC server started.")
    while True:
        sleep(1)

if __name__ == "__main__":
    start_osc_server()
