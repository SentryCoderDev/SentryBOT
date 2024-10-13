import os
from pubsub import pub

class JetsonTemperature:
    WARNING_TEMP = 80
    THROTTLED_TEMP = 85
    AVG_TEMP = 70

    def __init__(self, sensor_file='/sys/devices/virtual/thermal/thermal_zone0/temp'):
        self.sensor_file = sensor_file
        pub.subscribe(self.monitor, 'loop:10')

    def read(self):
        with open(self.sensor_file, 'r') as f:
            temp_str = f.readline().strip()
            return float(temp_str) / 1000  # Convert millidegrees Celsius to degrees Celsius

    def monitor(self):
        val = self.read()
        pub.sendMessage('log', msg='[TEMP] ' + str(val))
        if val >= JetsonTemperature.THROTTLED_TEMP:
            pub.sendMessage('led:full', color='red') # WARNING
        # else:
            # pub.sendMessage('led', identifiers='top5', color=self.map_range(round(val)))  # right


    def map_range(self, value):
        # Cap range for LED
        if value > JetsonTemperature.WARNING_TEMP:
            value = JetsonTemperature.WARNING_TEMP
        if value < JetsonTemperature.AVG_TEMP:
            value = JetsonTemperature.AVG_TEMP

        # translate range (STARTUP_TEMP to WARNING_TEMP) to (100 to 0) (green is cool, red is hot)
        OldRange = (JetsonTemperature.AVG_TEMP - JetsonTemperature.WARNING_TEMP)
        NewRange = (100 - 0)
        val = (((value - JetsonTemperature.WARNING_TEMP) * NewRange) / OldRange) + 0
        return val

