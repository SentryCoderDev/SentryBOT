# Stop any running instances of main.py
sudo pkill -f /home/archie/modular-biped/main.py
# Enable camera module
sudo modprobe bcm2835-v4l2
# Start the GPIO daemon
sudo pigpiod
# Start the main script with 'bci' as the argument
sudo python3 /home/archie/modular-biped/main.py bci
