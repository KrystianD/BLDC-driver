import sys
sys.path.append("../lib")
import bldc

device = int(sys.argv[1])
num = int(sys.argv[2])
_dir = int(sys.argv[3])
duty = int(sys.argv[4])

d = bldc.Driver(device, num)
d.sendSettings(_dir, duty)
