import os, sys
import bldc

num = int(sys.argv[1])
_dir = int(sys.argv[2])
duty = int(sys.argv[3])

d = bldc.Driver(6, num)
d.sendSettings(_dir, duty)
