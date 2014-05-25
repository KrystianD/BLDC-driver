import sys
import bldc

num = int(sys.argv[1])
duty = int(sys.argv[2])

d = bldc.Driver(6, num)
d.setDuty(duty)
