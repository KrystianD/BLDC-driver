import math, sys

step = 0.020
startTime = 1
minSpeed = 0
maxSpeed = 170

sys.stdout.write("static uint8_t speeds[] = { ")

t = 0
while t < startTime:
  t += step

  x = t
  val = (math.exp(x/(1/math.log(2)))-1)**4

  val = (maxSpeed - minSpeed) * val + minSpeed

  sys.stdout.write("{0}, ".format(int(val)))

sys.stdout.write("};\n")
