#ifndef __CRC16_H__
#define __CRC16_H__

#include <stdint.h>

extern const uint16_t CRCtbl[256];

static inline uint16_t crcUpdate(uint16_t crc, uint8_t b)
{
	return (crc >> 8) ^ CRCtbl[(crc & 0xff) ^ b];
}

#endif
