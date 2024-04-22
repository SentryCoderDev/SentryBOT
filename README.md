<h1>SentryBOT Overview and Frames </h1> &nbsp; <h1><a href="https://github.com/SentryCoderDev/SentryBOT/tree/SentryBOT-V4_EN/images"> -> Photos <- </a></h1>

<div style="display: flex;">
    <img src="https://github.com/SentryCoderDev/SentryBOT/assets/134494889/a15f4172-9089-47ee-9e30-9309376784a2" width="30%">
    <img src="https://github.com/SentryCoderDev/SentryBOT/assets/134494889/58789c03-9025-4d4b-a22c-256050cd794a" width="35%">
    <img src="https://github.com/SentryCoderDev/SentryBOT/assets/134494889/110541b8-3211-41fe-977a-7b9c58584bf4" width="30%">
   
</div>


# Robotics development framework
This platform was built to modularize robotics development and experimentation with Python/C++ using Raspberry Pi/Jetson nano and Arduino.

## Coral USB Accelerator

To use the Googla Coral USB Accelerator, first reprint the Pi SD card with the image included in the AIY Maker Kit.

(I tried to install the required software included in the Coral getting started guide, but was unsuccessful due to an error that "GLIBC_2.29" was not found.)

Alternatively, you can opt for the original (slower) facial recognition process by setting Config.VISION_TECH to opencv. I'm no longer updating this section, so you may encounter some integration issues.

## Setup
```
chmod 777 install.sh
./install.sh
```

To use neopixels on Raspberry Pi, you need to disable the sound (see neopixel section).

## Operation
```
./startup.sh
```

For manual control via keyboard
```
./manual_startup.sh
```

Contains a preview of the video feed to get started (not available via SSH)
```
./preview_startup.sh
```

### Testing
```
python3 -m pytest --cov=modules --cov-report term-missing
```

## Automatically run at startup

Run `sudo vim /etc/rc/local` and add the following lines before `exit 0`:
```
python3 /home/Desktop/companion-robot/shutdown_pi.py
/home/Desktop/companion-robot/startup.sh
```

### Automatic Shutdown in case of fall
GPIO 26 is wired to enable shutdown when brought to ground via a switch.

The shutdown_pi.py script manages this.

Guide:
https://howchoo.com/g/mwnlytk3zmm/how-to-add-a-power-button-to-your-raspberry-pi

## Features

### Face detection and tracking
Using Raspberry Pi camera or any webcam

### Servo control
8 servo control with Arduino serial connection (6 servo feet, 1 servo pan, 1 servo tilt)

### Battery Monitor
It is integrated externally and via Arduino serial connection via software.

### Buzzer
A buzzer is connected to GPIO 27 so that tones can be played when no sound is output (see Neopixel section).

https://github.com/gumslone/raspi_buzzer_player.git

### Motion sensor
An RCWL-0516 microwave radar sensor is attached to GPIO 13. This sensor, with its piservo module, will automatically scan 360 degrees like a radar when it enters a night loop 

note: it contains two different piservos, one for nltk and one for motion sensor

### NLTK
NLTK analyzes a text and evaluates the degree to which the text is positive or negative. The antenna again uses the piservo control to perform an animation of this evaluation.

### Stereo MEMS Microphones
GPIO 18, 19 and 20 are used to use stereo MEMS microphones as audio input.
```
Mic 3V - Connects to Pi 3.3V.
Mic GND - Connects to Pi GND.
Mic SEL - Pi connects to GND (this is used for channel selection, can be connected to 3.3V or GND).
Mic BCLK - Connects to BCM 18 (pin 12).
Mic DOUT - Connects to BCM 20 (pin 38).
Mic LRCL - connects to BCM 19 (pin 35).
```
https://learn.adafruit.com/adafruit-i2s-mems-microphone-breakout/raspberry-pi-wiring-test


```
cd ~
sudo pip3 install --upgrade adafruit-python-shell
wget https://raw.githubusercontent.com/adafruit/Raspberry-Pi-Installer-Scripts/master/i2smic.py
sudo python3 i2smic.py
```

#### Testing
`arecord -l`
`arecord -D plughw:0 -c2 -r 48000 -f S32_LE -t wav -V stereo -v file_stereo.wav`

_Note:_ See additional configuration below to support voice recognition.

### Hotword
The original hot word detection used the now deprecated Snowboy. The files are still available in this repository:

https://github.com/dmt-labs/modular-biped/blob/feature/PR29_review/modules/hotword.py contains the module from this framework.

https://github.com/dmt-labs/modular-biped/tree/feature/PR29_review/modules/snowboy includes original snowboy functionality.

https://github.com/dmt-labs/modular-biped/tree/feature/PR29_review/modules/snowboy/resources/models contains trained models that can be used as keywords. It may be possible to find more on the internet.

A guide to training new models is available here, but there are resources for training new models on a Linux device.

### Speech Recognition
Trigger word for voice recognition (not currently used):
https://snowboy.kitt.ai/

Speech recognition is activated when a face is seen.
Make sure that the `device_index` value specified in the `modules/speechinput.py` file matches your microphone.

See `scripts/speech.py` to list and test input devices. See below for MEMS microphone configuration.
### MEMS Microphone configuration for Speech Recognition

By default, the Adafruit I2S MEMS Microphone Breakout does not work with voice recognition.

The following configuration changes must be made to support voice recognition on MEMS microphone(s).

`sudo apt-get install ladspa-sdk`
Create `/etc/asound.conf` file with the following content:

```
pcm.pluglp {
     type ladspa
     slave.pcm "plughw:0"
     path "/usr/lib/ladspa"
     capture_plugins [
    {
       label hpf
       id 1042
    }
         {
                 label amp_mono
                 id 1048
                 input {
                     controls [ 30 ]
                 }
         }
     ]
}

pcm.lp {
     type plug
     slave.pcm pluglp
}
```

This ensures that the 'lp' device can be referenced in voice recognition. In the example below it is shown at index `18`.

For example, the sample rate should be set to `16000`.

`mic = sr.Microphone(device_index=18, sample_rate=16000)`

References:

* [MEMS Microphone Installation Guide](https://learn.adafruit.com/adafruit-i2s-mems-microphone-breakout/raspberry-pi-wiring-test)

* [Adafruit Support discussing issue](https://forums.adafruit.com/viewtopic.php?f=50&t=181675&p=883853&hilit=MEMS#p883853)

* [Referenced documentation of fix](https://github.com/mpromonet/v4l2rtspserver/issues/94)

### Serial communication with Arduino

To use the Raspberry Pi's serial port, getty (the program that displays the login screen) must be disabled.

`sudo raspi-config -> Interfacing Options -> Serial -> "Would you like a login shell to be accessible over serial" = 'No'. Restart`

#### Connection via serial pins
Connect GPIO pins 14 and 15 (tx and rx) of the Raspberry Pi to the Arduino tx and rx pins (tx -> rx in both direction and direction reversal!) via a logic level shifter, since Raspberry Pi 3v3 and Arduino (probably) It is 5v.

#### You can follow the steps below to upload to Arduino via serial pins:

To upload via serial pins, press the reset button on the Arduino at the point where the IDE starts 'uploading' (after compiling); otherwise a synchronization error will be displayed.

### Neopixel

WS1820B support is provided via Pi GPIO pin 12. Unfortunately, audio on the Pi must be disabled to support this.

```
sudo vim /boot/config.txt
#dtparam=audio=on
```

Therefore, the application must be run with `sudo`.

If you want to use neopixels without using the driver (and no audio output), set pin to 12 in the configuration and set i2c to False. The application must also be run with sudo.

https://learn.adafruit.com/neopixels-on-raspberry-pi/python-usage

### Instant Translation

The live translation module <code>translation.py</code> allows live translation of logs and TTS output via Google Translate.

Call the <code>translate.request(msg)</code> function and optionally specify the source and target languages.

<code>config/translation.yml</code> specifies the default languages.


## PCBs
The prefabricated PCBs of this project are available in the `circuits` folder. This provides connectivity between core components as described above.


![Top](https://github.com/SentryCoderDev/SentryBOT/assets/134494889/91994cd7-4dd4-43bc-9664-7bb79c908d23)


![body_pcb](https://github.com/SentryCoderDev/SentryBOT/assets/134494889/857f3284-5361-4b6f-b097-b052fe510902)
