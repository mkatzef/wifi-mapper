""" Fetches the RSSI values measured by the pair of Wi-Fi modules
    on the Wi-Fi scanner, and calculates sampling statistics.
	Written by Marc Katzef
"""

import urllib.request
import time

# ESP8266 IP addresses on the shared network
urls = ["http://192.168.43.251/rssi", "http://192.168.43.87/rssi"]

# Experiment duration
test_duration_s = 5

start_time = time.time()
end_time = start_time + test_duration_s

count = 0
total = 0
diff = 0
print("Starting...")
while time.time() < end_time:
    s1 = int(urllib.request.urlopen(urls[0]).read())
    s2 = int(urllib.request.urlopen(urls[1]).read())
    total += s1 + s2
    diff += abs(s1 - s2)
    count += 2

print("Received", count, "samples")
print("Average sample rate: {:.3f} Hz".format(count / test_duration_s))
print("Average signal strength: {:.3f} dB".format(total / count))
print("Average signal diff: {:.3f} dB".format(diff / count))

# Enter infinite loop showing difference between RSSI readings
maxdiff = 0
while True:
    s1 = int(urllib.request.urlopen(urls[0]).read())
    s2 = int(urllib.request.urlopen(urls[1]).read())
    diff = abs(s2 - s1)
    maxdiff = max(maxdiff, diff)
    print("{}, {}, diff: {}, maxdiff: {}".format(s1, s2, diff, maxdiff), end='\r')
    