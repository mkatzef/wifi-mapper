""" Maps RSSI values (encoded as the red channel in a pcd file) to
    a colour indicating the signal strength (green being the best,
    red being the worst).
	Written by Marc Katzef
"""

import sys
import os

RSSI_MIN = 30  # best
RSSI_MAX = 90  # worst

HEADER_LINE_COUNT = 10
RSSI_RANGE = RSSI_MAX - RSSI_MIN

minRssi = -1
maxRssi = -1

RELATIVE = False

def get_rgb(pack):
    b = pack & 0xFF
    pack >>= 8
    g = pack & 0xFF
    pack >>= 8
    r = pack & 0xFF
    
    return (r, g, b)


def pack_rgb(r, g, b):
    pack = r
    pack = (pack << 8) + g;
    pack = (pack << 8) + b;
    
    return pack
    
    
def map_color(rssi):
    r = 0
    g = 0
    b = 0
    
    rssi = min(rssi, RSSI_MAX)
    rssi = max(rssi, RSSI_MIN)
    
    ratio = (rssi - RSSI_MIN) / RSSI_RANGE
    r = int(255 * ratio)
    g = 255 - r
    #b = 255 - max(r, g)
    
    return (r, g, b)


def get_mapped_color(rssi_pack):
    rssi, _, _ = get_rgb(rssi_pack)
    
    global minRssi, maxRssi
    if minRssi == -1:
        minRssi = rssi
        maxRssi = rssi
    if rssi < minRssi:
        minRssi = rssi
    if rssi > maxRssi:
        maxRssi = rssi
        
    return pack_rgb(*map_color(rssi))


def main(input, output):
    with open(input, 'r') as infile:
        with open(output, 'w') as outfile:
            for i in range(HEADER_LINE_COUNT):
                outfile.write(infile.readline())
            
            tokens = infile.readline().strip().split()
            while len(tokens) == 4:
                tokens[-1] = str(get_mapped_color(int(tokens[-1])))
                outfile.write(" ".join(tokens) + "\n")
                tokens = infile.readline().strip().split()
    

if __name__ == "__main__":
    argv = sys.argv
    if argv[0].startswith("py"):
        argv.pop(0)
    
    if len(argv) == 4:
        if (argv[-1] == '-r'):
            RELATIVE = True
            argv.pop()
    
    if len(argv) != 3:
        print("usage:", argv[0], "input_file", "output_file", "[-r]")
    else:
        f1, f2 = argv[1:]
        if not os.path.exists(f1):
            print("file not found:", f1)
        elif os.path.exists(f2):
            print("out file already exists:", f2)
        else:
            main(f1, f2)

            print("min RSSI:", minRssi)
            print("max RSSI:", maxRssi)
            
            if RELATIVE:
                RSSI_MIN = minRssi
                RSSI_MAX = maxRssi
                main(f1, f2) # called a second time on purpose (this time with a revised range)
