# Signal Strength Server
A program for an ESP8266 microcontroller to act as a Wi-Fi signal strength scanner, with a simple web interface.

## Screenshots
<table>
<td>
<img src="../Images/epic_webpage.png" width="256" title="ESP8266 signal strength server interface">
</td>
</table>

## Files
This project consists of the following files: 
* `SignalStrengthServer.ino` - the Arduino sketch for the ESP8266 microcontrollers.
* `WiFiCredentialsTemplate.h` - the C header file to be copied and populated with default Wi-Fi network parameters.
* `fetcher_single.py` - a Python3 script to test the connection with an ESP8266 on the same network
* `fetcher.py` - a Python3 script which performs a similar function to `fetcher.py` but with multiple ESP8266 microcontrollers at the same time.

## Pinout
The Arduino sketch requires no external components for the ESP8266 to perform the Wi-Fi scanning role in its entirety.

## Installation
To program an ESP8266, the following software packages must be installed:
* Arduino IDE (available [here](https://www.arduino.cc/en/Main/Software))
* Arduino core for ESP8266 WiFi Chip (available on [github](https://github.com/esp8266/Arduino))

With the above softwares installed, an ESP8266 microcontroller may be programmed with the included sketch through the Arduino IDE.

## Use
With an ESP8266 module programmed with the included sketch, the device will create a soft access point on boot. A Wi-Fi-enabled device may connect to the new network in the usual way. Once connected, navigate to  `http://192.168.4.1` in a web browser to load the ESP8266's configuration page. This page allows one to:
* View the IP address of the device on each of the networks it is available on
* Change the network the ESP8266 is connected to (only when broadcasting its own network)
* Toggle the state of the soft access point (only when connected to an alternative network)
* Get the current value for RSSI read by the ESP8266

## Authors
**Marc Katzef** - mka122@uclive.ac.nz