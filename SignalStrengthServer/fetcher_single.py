""" Fetches the RSSI value from a single ESP8266 as frequently as possible.
    Written by Marc Katzef
"""

import urllib.request

# The ESP8266's IP address on the shared network
url = "http://192.168.4.1/rssi"
while True:
    print(int(urllib.request.urlopen(url).read()), end='\r')