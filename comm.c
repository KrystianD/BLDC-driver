#include <public.h>
#include "hardware.h"
#include "comm.h"

#include "bldc.h"
#include "crc16.h"
#include "debug.h"

#define TWOPT (_BV(TWEN) | _BV(TWIE))

volatile uint8_t deviceID;

void comm_findID();

void commInit()
{
	comm_findID();

#define TWI_ADDRESS 0x20
	TWAR = ((TWI_ADDRESS + deviceID) << 1) | _BV(TWGCE);
	TWCR = _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
}

void commProcess()
{
	static int l = 0;
	if (ticks - l >= 1000)
	{
		l = ticks;
		// printf("ST 0x%02x %d\r\n", TW_STATUS, validCPS);
	}
}

#define STATE_IDLE  0
#define STATE_RECV  1
#define STATE_SEND  2

#define CMD_DUTY 0x11

static uint8_t buf[5 + 2];
static uint8_t outbuf[3 + 2];
static uint8_t state = STATE_IDLE;
static uint8_t bufIdx = 0;

void comm_processInput()
{
	uint16_t crc;
	if (state == STATE_RECV && bufIdx == sizeof(buf))
	{
		int i;
		crc = crcUpdate(  0, buf[0]);
		crc = crcUpdate(crc, buf[1]);
		crc = crcUpdate(crc, buf[2]);
		crc = crcUpdate(crc, buf[3]);
		crc = crcUpdate(crc, buf[4]);

		uint16_t origCrc = *(uint16_t*)(buf + sizeof(buf) - 2);
		if (crc == origCrc)
		{
			uint8_t cmd = buf[0];
			uint8_t v1 = buf[1];
			uint8_t v2 = buf[2];
			uint8_t v3 = buf[3];
			uint8_t v4 = buf[4];
			if (cmd == 0x11)
			{
				bldcSetDuty(buf[1 + deviceID]);

				state = STATE_IDLE;
				bufIdx = 0;
			}
			else if (cmd == 0x12)
			{
				cli();
				outbuf[0] = 0xaa;
				outbuf[1] = (validCPS >> 0) & 0xff;
				outbuf[2] = (validCPS >> 8) & 0xff;
				sei();
				crc = crcUpdate(  0, outbuf[0]);
				crc = crcUpdate(crc, outbuf[1]);
				crc = crcUpdate(crc, outbuf[2]);
				*(uint16_t*)(outbuf + sizeof(outbuf) - 2) = crc;

				state = STATE_SEND;
				bufIdx = 0;
			}
			else if (cmd == 0xaa && v1 == 0xaa && v2 == 0xbb && v3 == 0xcc && v4 == 0xdd)
			{
				wdt_enable(WDTO_15MS);
				cli();
				for (;;);
			}
			else
			{
				state = STATE_IDLE;
				bufIdx = 0;
			}
		}

		// state = STATE_IDLE;
		// bufIdx = 0;
	}
	else
	{
		state = STATE_IDLE;
		bufIdx = 0;
	}
}

ISR(TWI_vect)
{
	uint8_t c = TWDR;

	switch (TW_STATUS)
	{
		// Slave READ
	case TW_SR_SLA_ACK:
	case TW_SR_GCALL_ACK:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);

		state = STATE_IDLE;
		bufIdx = 0;
		break;
	case TW_SR_DATA_ACK:
	case TW_SR_GCALL_DATA_ACK:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);

		if (state == STATE_IDLE)
		{
			bufIdx = 0;
			buf[bufIdx++] = c;
			state = STATE_RECV;
		}
		else if (state == STATE_RECV)
		{
			if (bufIdx < sizeof(buf))
				buf[bufIdx++] = c;
		}
		else
		{
		}

		// printf("data 0x%02x\r\n", TWDR);
		break;
	case TW_SR_STOP:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);

		comm_processInput();
		break;

		// Slave TRANSMIT
	case TW_ST_SLA_ACK:
		if (state == STATE_SEND)
		{
			TWDR = outbuf[bufIdx++];
			TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
		}
		else
		{
			TWCR = TWOPT | _BV(TWINT);
		}
		break;

	case TW_ST_DATA_ACK:
		if (state == STATE_SEND)
		{
			TWDR = outbuf[bufIdx++];
			if (bufIdx == sizeof(outbuf))
			{
				TWCR = TWOPT | _BV(TWINT);
				state = STATE_IDLE;
			}
			else
			{
				TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
			}
		}
		else
		{
			TWCR = TWOPT | _BV(TWINT);
		}
		break;

	case TW_ST_DATA_NACK:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
		// printf("NA\r\n");
		break;

	case TW_ST_LAST_DATA:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
		// printf("LA\r\n");
		break;

	case TW_BUS_ERROR:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA) | _BV(TWSTO);
		// printf("buserr\r\n");

		break;
	default:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
		// printf("oth 0x%02x\r\n", TWSR);
		break;
	}
}

void comm_findID()
{
	IO_INPUT_PP(ID0);
	IO_PUSH_PULL(ID1);
	IO_LOW(ID1);
	IO_INPUT_PP(ID2);

	if (IO_IS_HIGH(ID0) && IO_IS_HIGH(ID2))
	{
		deviceID = 0;
	}
	else if (IO_IS_HIGH(ID0))
	{
		deviceID = 1;
	}
	else if (IO_IS_HIGH(ID2))
	{
		deviceID = 2;
	}
	else
	{
		deviceID = 3;
	}

	IO_INPUT(ID0);
	IO_INPUT(ID1);
	IO_INPUT(ID2);
	IO_LOW(ID0);
	IO_LOW(ID1);
	IO_LOW(ID2);
}
