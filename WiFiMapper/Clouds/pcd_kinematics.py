""" Calculates the distanced travelled by the Wi-Fi scanner
    based on the generated pcd file.
	Written by Marc Katzef
"""

import sys
import os
import numpy as np

HEADER_LINE_COUNT = 10
MAX_DELTA_MM = 150
MODULE_COUNT = 5


def get_distance(p1, p2):
    sqr_sum = (p1[0] - p2[0]) ** 2  \
            + (p1[1] - p2[1]) ** 2  \
            + (p1[2] - p2[2]) ** 2             

    return sqr_sum ** (1 / 2)


def main(input, module_count, rate):
    with open(input, 'r') as infile:
        for i in range(HEADER_LINE_COUNT):
            infile.readline()
        
        prev_positions = []
        distances = np.zeros(module_count)
        counts = np.zeros(module_count)
        
        for i in range(module_count):
            tokens = infile.readline().strip().split()
            position_i = [float(j) for j in tokens[:3]]
            prev_positions.append(position_i)
            
        index = 0
        while len(tokens) == 4:
            position_i = [float(i) for i in tokens[:3]]
            
            d_i = get_distance(position_i, prev_positions[index])
            prev_positions[index] = position_i
            
            if d_i < MAX_DELTA_MM:
                distances[index] += d_i
                counts[index] += 1
            
            tokens = infile.readline().strip().split()
            index += 1
            if index >= module_count:
                index = 0
            
    
    for i in range(module_count):
        duration_i = counts[i] / rate
        distance_i_m = distances[i] / 1000
        avg_speed_i_m_s = distance_i_m / duration_i
        
        print("Module:", i + 1)
        print("Movement time:\t", duration_i, "s")
        print("Total distance:\t", distance_i_m, "m")
        print("Average speed:\t", avg_speed_i_m_s, "m/s")

    
if __name__ == "__main__":
    argv = sys.argv
    if argv[0].startswith("py"):
        argv.pop(0)
    
    if len(argv) != 4:
        print("usage:", argv[0], "input_file", "module_count", "sample_rate")
    else:
        f1, module_count, rate = argv[1:]
        if not os.path.exists(f1):
            print("file not found:", f1)
        else:
            valid_rate = False
            try:
                rate = float(rate)
                module_count = int(module_count)
                valid_rate = True
            except:                
                print("invalid module count and/or rate (should be: integer and float)")
        if valid_rate:
            main(f1, module_count, rate)
            