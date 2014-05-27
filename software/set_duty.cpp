#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <i2c.h>
#include "../crc16.h"

int main(int argc, char** argv)
{
	if (argc != 4)
	{
		printf("Usage: %s device addr|all duty\n", argv[0]);
		return 1;
	}

	const char* device = argv[1];
	const char* addr = argv[2];
	int duty = strtol(argv[3], 0, 0);
	
	i2cOpen(device);
	if (strcmp(addr, "all") == 0)
		i2cSetAddress(0x00);
	else
		i2cSetAddress(strtol(addr, 0, 0));

	
	uint8_t data[5 + 2];
	data[0] = 0x11;
	data[1] = duty;
	data[2] = duty;
	data[3] = duty;
	data[4] = duty;
	
	uint16_t crc = 0;
	for (int i = 0; i < 5; i++)
		crc = crcUpdate(crc, data[i]);
	*(uint16_t*)(data + 5) = crc;

	i2cWrite(data, sizeof(data));

	int r = i2cRead(data, 5);
	printf("r: %d\n", r);

	for(int i=0;i<5;i++)
	{
		printf("0x%02x\n", data[i]);
	}


	i2cClose();
	
	return 0;
}
