""" Fetches the RSSI values measured by the Wi-Fi modules
    on the Wi-Fi scanner, and calculates sampling statistics.
	Written by Marc Katzef
"""

import urllib.request
import time

# ESP8266 IP addresses on the shared network
urls = ["http://192.168.1.77/rssi", "http://192.168.1.76/rssi", "http://192.168.1.71/rssi", "http://192.168.1.72/rssi", "http://192.168.1.74/rssi"]

# Experiment duration
test_duration_s = 5

start_time = time.time()
end_time = start_time + test_duration_s

count = 0
total = 0
diff = 0
print("Starting...")
while time.time() < end_time:
    for url in urls:
        rssi = int(urllib.request.urlopen(url).read())
        total += rssi
        count += 1

print("Received", count, "samples")
print("Average sample rate: {:.3f} Hz".format(count / test_duration_s))
print("Average signal strength: {:.3f} dB".format(total / count))

# Enter infinite loop showing difference between RSSI readings
maxdiff = 0
while True:
    for url in urls:
        rssi = int(urllib.request.urlopen(urls[0]).read())
        print(rssi, end='\t')
    print(" ", end='\r')
    