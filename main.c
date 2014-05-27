#include <public.h>

#include <stdio.h>

#include "hardware.h"
#include "bldc.h"
// #include "debug.h"
#include "comm.h"

volatile uint16_t ticks = 0;

const int MS_INTERVAL = 5;
SIGNAL(TIMER2_OVF_vect) // every 32us
{
	static uint8_t rem = 32 * MS_INTERVAL;
	if (!(--rem))
	{
		ticks += MS_INTERVAL;
		rem = 32 * MS_INTERVAL;
		
		static uint16_t last = 0;
		const int CPS_PER_SEC = 5;
		if (ticks - last >= 1000 / CPS_PER_SEC)
		{
			last = ticks;
			validCPS = cps * CPS_PER_SEC;
			cps = 0;
		}
	}
}

int main()
{
	IO_PUSH_PULL(LED);
	IO_HIGH(LED);
	
	IO_PUSH_PULL(P_UP);
	IO_PUSH_PULL(P_UN);
	IO_PUSH_PULL(P_VP);
	IO_PUSH_PULL(P_VN);
	IO_PUSH_PULL(P_WP);
	IO_PUSH_PULL(P_WN);
	IO_LOW(P_UP);
	IO_LOW(P_UN);
	IO_LOW(P_VP);
	IO_LOW(P_VN);
	IO_LOW(P_WP);
	IO_LOW(P_WN);
	
	IO_INPUT(SCL);
	IO_INPUT(SDA);
	
	DISABLE_ALL();
	
	TCCR1A = TCCR1A_BASE;
	TCCR1B = TCCR1B_BASE;
	TCCR2 = TCCR2_BASE;
	TIMSK = TIMSK_BASE;
	OCR2 = OCR1B = OCR1A = 0;
	
	ADCSRA = 0;
	SFIOR = _BV(ACME);
	
	bldcDisable();
	
	// debugInit();
	bldcInit();
	commInit();
	
	sei();
	
	for (;;)
	{
		bldcProcess();
		commProcess();
	}
}

