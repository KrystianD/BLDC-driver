import os, fcntl

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
	CMD_STATUS = 0x12
	CMD_RESET = 0xaa

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

	def cmd(self, cmd, args, readLen = 0):
		buf = [cmd] + args
		return self.send(buf, readLen)

	def setDuty(self, duty):
		if self.num is None:
			self.cmd(self.CMD_DUTY, [duty, duty, duty, duty])
		else:
			b = [0, 0, 0, 0]
			b[self.num] = duty
			self.cmd(self.CMD_DUTY, b, 0)

	def setDutyAll(self, d1, d2, d3, d4):
		self.cmd(self.CMD_DUTY, [d1, d2, d3, d4])

	def reset(self):
		self.cmd(self.CMD_RESET, [0xaa])

	def sendSettings(self, _dir, startupDuty = 50):
		self.cmd(self.CMD_SETTINGS, [_dir, startupDuty])

	def getStatus(self):
		d = self.cmd(self.CMD_STATUS, [], 3)
		return {
				"state": d[0],
				"speed": (d[2] << 8) | d[1] }
 
	def send(self, data, readLen):
		I2C_SLAVE = 0x0703

		crc = crc16(data)
		data.append((crc >> 0) & 0xff)
		data.append((crc >> 8) & 0xff)

		fd = os.open("/dev/i2c-" + str(self.bus), os.O_RDWR)
		fcntl.ioctl(fd, I2C_SLAVE, self.addr)
		b = "".join([chr(x) for x in data]).encode("latin-1")
		t = os.write(fd, b)

		if readLen > 0:
			readLen += 2
			r = os.read(fd, readLen).decode("latin-1")
			if len(r) == readLen:
				r = [ord(x) for x in r]
				crc = crc16(r[:-2])
				crcOrig = (r[-1] << 8) | r[-2]
				if crc == crcOrig:
					retData = r[:-2]
					os.close(fd)
					return retData
				else:
					return False
			else:
				return False
		else:
			os.close(fd)
			return True
