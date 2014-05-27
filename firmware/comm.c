#include <public.h>
#include "hardware.h"
#include "comm.h"

#include "bldc.h"
#include "crc16.h"
// #include "debug.h"
#include "settings.h"

#define TWOPT (_BV(TWEN) | _BV(TWIE))

#define STATE_IDLE  0
#define STATE_RECV  1
#define STATE_SEND  2

#include "bldc_driver.h"

// public
volatile uint8_t deviceID, deviceIDMask;

// prv
static uint8_t buf[5 + 2];
static uint8_t outbuf[3 + 2];
volatile uint8_t bufIdx = 0;
uint16_t crc;

void comm_findID();

// impl
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

void comm_processInput()
{
	if (bufIdx >= 1 + 2)
	{
		int i;
		uint16_t origCrc = *(uint16_t*)(buf + bufIdx - 2);
		if (crc == origCrc)
		{
			uint8_t cmd = buf[0];
			uint8_t len = bufIdx - sizeof(origCrc) - sizeof(cmd);
			uint8_t* data = buf + 1;
			
			if (cmd == CMD_SETTINGS && len == CMD_SETTINGS_LEN)
			{
				if (state == STATE_STOPPED)
				{
					uint8_t dir = data[0];
					uint8_t startupDuty = data[1];
					if (startupDuty > STARTUP_DUTY_MAX)
						startupDuty = STARTUP_DUTY_MAX;
						
					SET_startupDuty = startupDuty;
					if (dir == 0)
						SET_phasesOffset = 0;
					else
						SET_phasesOffset = 6;
				}
			}
			else if (cmd == CMD_DUTY && len == CMD_DUTY_LEN)
			{
				uint8_t duty = data[deviceID];
				if (duty > 0)
					bldcEnable();
				else
					bldcDisable();
					
				bldcSetDesiredDuty(duty);
				
				uint16_t crc2;
				outbuf[0] = state;
				outbuf[1] = (validCPS >> 0) & 0xff;
				outbuf[2] = (validCPS >> 8) & 0xff;
				crc2 = crcUpdate(0, outbuf[0]);
				crc2 = crcUpdate(crc2, outbuf[1]);
				crc2 = crcUpdate(crc2, outbuf[2]);
				*(uint16_t*)(outbuf + sizeof(outbuf) - 2) = crc2;
				
				bufIdx = 0;
			}
			else if (cmd == CMD_STATUS && len == CMD_STATUS_LEN)
			{
				uint16_t crc2;
				outbuf[0] = state;
				outbuf[1] = (validCPS >> 0) & 0xff;
				outbuf[2] = (validCPS >> 8) & 0xff;
				crc2 = crcUpdate(0, outbuf[0]);
				crc2 = crcUpdate(crc2, outbuf[1]);
				crc2 = crcUpdate(crc2, outbuf[2]);
				*(uint16_t*)(outbuf + sizeof(outbuf) - 2) = crc2;
				
				bufIdx = 0;
			}
			else if (cmd == CMD_RESET && len == CMD_RESET_LEN)
			{
				if (data[0] == 0xaa)
				{
					wdt_enable(WDTO_15MS);
					cli();
					for (;;);
				}
			}
			else
			{
				bufIdx = 0;
			}
		}
		else
		{
			bufIdx = 0;
		}
	}
	else
	{
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
		
		bufIdx = 0;
		crc = 0;
		break;
	case TW_SR_DATA_ACK:
	case TW_SR_GCALL_DATA_ACK:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
		
		if (bufIdx < sizeof(buf))
		{
			if (bufIdx >= 2)
				crc = crcUpdate(crc, buf[bufIdx - 2]);
			buf[bufIdx++] = c;
		}
		break;
	case TW_SR_STOP:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
		
		comm_processInput();
		break;
		
	// Slave TRANSMIT
	case TW_ST_SLA_ACK:
		bufIdx = 0;
		TWDR = outbuf[bufIdx++];
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
		break;
		
	case TW_ST_DATA_ACK:
		TWDR = outbuf[bufIdx++];
		if (bufIdx == sizeof(outbuf))
		{
			TWCR = TWOPT | _BV(TWINT);
			bufIdx = 0;
		}
		else
		{
			TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
		}
		break;
		
	case TW_ST_DATA_NACK:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
		break;
		
	case TW_ST_LAST_DATA:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
		break;
		
	case TW_BUS_ERROR:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA) | _BV(TWSTO);
		break;
	default:
		TWCR = TWOPT | _BV(TWINT) | _BV(TWEA);
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
	deviceIDMask = 1 << deviceID;
	
	IO_INPUT(ID0);
	IO_INPUT(ID1);
	IO_INPUT(ID2);
	IO_LOW(ID0);
	IO_LOW(ID1);
	IO_LOW(ID2);
}
