import sys
import bldc

num = sys.argv[1]
if num == "all":
  num = None
else:
  num = int(num)
duty = int(sys.argv[2])

d = bldc.Driver(6, num)
d.setDuty(duty)
