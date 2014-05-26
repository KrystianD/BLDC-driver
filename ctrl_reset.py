import sys
import bldc

num = int(sys.argv[1])

d = bldc.Driver(6, num)
d.reset()
