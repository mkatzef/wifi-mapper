import sys
import os

HEADER_LINE_COUNT = 10
MAX_DELTA_MM = 150

def get_distance(p1, p2):
    sqr_sum = (p1[0] - p2[0]) ** 2  \
            + (p1[1] - p2[1]) ** 2  \
            + (p1[2] - p2[2]) ** 2             

    return sqr_sum ** (1 / 2)


def main(input, rate):
    with open(input, 'r') as infile:
        for i in range(HEADER_LINE_COUNT):
            infile.readline()
        
        distance_a = 0
        count = 0
        distance_b = 0
        
        tokens_a = infile.readline().strip().split()
        p1_a = [float(i) for i in tokens_a[:3]]
        tokens_b = infile.readline().strip().split()
        p1_b = [float(i) for i in tokens_b[:3]]
        
        tokens_a = infile.readline().strip().split()
        tokens_b = infile.readline().strip().split()
        while len(tokens_b) == 4:
            p2_a = [float(i) for i in tokens_a[:3]]
            p2_b = [float(i) for i in tokens_b[:3]]
            
            da = get_distance(p1_a, p2_a)
            db = get_distance(p1_b, p2_b)
            
            if da < MAX_DELTA_MM and db < MAX_DELTA_MM:
                distance_a += da
                distance_b += db
                p1_a = p2_a
                p1_b = p2_b
                
                count += 1
            
            tokens_a = infile.readline().strip().split()
            if len(tokens_a) != 4:
                break    
            tokens_b = infile.readline().strip().split()
    
    duration = count / rate
    distance_a_m = distance_a / 1000
    avg_speed_a_m_s = distance_a_m / duration
    distance_b_m = distance_b / 1000
    avg_speed_b_m_s = distance_b_m / duration
    
    print("Time:\t", duration, "s")
    print("Total distance A:\t", distance_a_m, "m")
    print("Average speed A:\t", avg_speed_a_m_s, "m/s")
    print("Total distance B:\t", distance_b_m, "m")
    print("Average speed B:\t", avg_speed_b_m_s, "m/s")

    
if __name__ == "__main__":
    argv = sys.argv
    if argv[0].startswith("py"):
        argv.pop(0)
    
    if len(argv) != 3:
        print("usage:", argv[0], "input_file", "sample_rate")
    else:
        f1, rate = argv[1:]
        if not os.path.exists(f1):
            print("file not found:", f1)
        else:
            valid_rate = False
            try:
                rate = float(rate)
                valid_rate = True
            except:                
                print("out file already exists:", f2)
        if valid_rate:
            main(f1, rate)