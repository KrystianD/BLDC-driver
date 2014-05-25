import os

def crc16(buff, crc = 0, poly = 0xa001):
	l = len(buff)
	i = 0
	while i < l:
		ch = (buff[i])
		uc = 0
		crc ^= ch
		while uc < 8:
			if (crc & 1):
				crc = (crc >> 1) ^ poly
			else:
				crc >>= 1
			uc += 1
		i += 1
	return crc

class Driver:
  CMD_SETTINGS = 0x10
  CMD_DUTY = 0x11

  bus = None
  num = None
  addr = None

  def __init__(self, bus, num = None):
    self.bus = bus
    self.num = num
    if num is None:
      self.addr = 0x00
    else:
      self.addr = 0x20 + num

  def cmd(self, cmd, args):
    buf = [cmd] + args
    crc = crc16(buf)
    buf.append((crc >> 0) & 0xff)
    buf.append((crc >> 8) & 0xff)
    self.send(buf)

  def setDuty(self, duty):
    if self.num is None:
      self.cmd(self.CMD_DUTY, [duty, duty, duty, duty])
    else:
      b = [0, 0, 0, 0]
      b[self.num] = duty
      self.cmd(self.CMD_DUTY, b)

  def setDutyAll(self, d1, d2, d3, d4):
    self.cmd(self.CMD_DUTY, [d1, d2, d3, d4])

  def sendSettings(self, _dir, startupDuty = 50):
    self.cmd(self.CMD_SETTINGS, [_dir, startupDuty])

  def send(self, data):
    s = "i2cset -y {0} 0x{1:x} 0x{2:x} {3} i".format(self.bus, self.addr, data[0], " ".join([str(x) for x in data[1:]]))
    print (s)
    os.system(s)

