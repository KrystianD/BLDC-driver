import sys
sys.path.append("../lib")
import bldc

device = int(sys.argv[1])
num = sys.argv[2]
if num == "all":
  num = None
else:
  num = int(num)

d = bldc.Driver(device, num)
status = d.getStatus()
print(status["state"])
print(status["speed"])
