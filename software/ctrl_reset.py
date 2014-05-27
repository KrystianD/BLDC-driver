import sys
sys.path.append("../lib")
import bldc

device = int(sys.argv[1])
num = int(sys.argv[2])

d = bldc.Driver(device, num)
d.reset()
