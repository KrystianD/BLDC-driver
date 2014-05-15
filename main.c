#include <public.h>

#define UART_BAUD     UART_BAUD_SELECT (9600,F_CPU)
#define UART_DATABITS 8
#include "UART.h"
#include <stdio.h>

#include "hardware.h"
#include "bldc.h"
#include "debug.h"

volatile unsigned int ticks = 0;

#define ADMUX_U 0
#define ADMUX_V _BV(MUX0)
#define ADMUX_W _BV(MUX1)

// inverted
#define ACSR_RISING  (_BV(ACIS1))
#define ACSR_FAILING (_BV(ACIS1) | _BV(ACIS0))

uint16_t phases[] = {
	UP | VN | ACW | ACFAILING,
	UP | WN | ACV | ACRISING,
	VP | WN | ACU | ACFAILING,
	VP | UN | ACW | ACRISING,
	WP | UN | ACV | ACFAILING,
	WP | VN | ACU | ACRISING,
};
uint8_t phasesADMUX[] = {
	ADMUX_W,
	ADMUX_V,
	ADMUX_U,
	ADMUX_W,
	ADMUX_V,
	ADMUX_U,
};
uint8_t phasesACSR[] = {
	ACSR_FAILING,
	ACSR_RISING,
	ACSR_FAILING,
	ACSR_RISING,
	ACSR_FAILING,
	ACSR_RISING,
};

// uint8_t phasesorig[] = {
// UP | VN,
// UP | WN,
// VP | WN,
// VP | UN,
// WP | UN,
// WP | VN,
// };
// uint8_t phases[] = {
// WP | VN,
// WP | UN,
// VP | UN,
// VP | WN,
// UP | WN,
// UP | VN,

// WP | VN, // optim
// };



#define STATE_STOPPED  0
#define STATE_STARTING 1
// #define STATE_STARTING2 5
#define STATE_CHANGING 2
#define STATE_NORMAL   3
#define STATE_STABILIZING   4

volatile uint8_t id = 0;

volatile uint8_t state;

volatile uint8_t phase = 0;
volatile uint8_t idx = 0;
volatile uint8_t delay = 0;
volatile uint8_t speedTime = 100;
volatile uint16_t val = 0;
volatile uint8_t brake = 0;
volatile uint8_t type = 0;

volatile uint8_t timer0 = 0;
volatile uint16_t lastCommutationTime = 0;

// volatile uint16_t changesValid = 0;


static inline void enableAC ()
{
	ACSR |= _BV(ACIE);	
	ACSR |= _BV(ACI);	
}
static inline void disableAC ()
{
	ACSR &= ~_BV(ACIE);
	ACSR |= _BV(ACI);	
}

void setPhaseAC ()
{
	u16 val = phases[phase];
	TCNT1 = TCNT2 = 0;
	if (val & UP) { ENABLE_UP; } else { DISABLE_UP; }
	if (val & UN) { ENABLE_UN; } else { DISABLE_UN; }
	if (val & VP) { ENABLE_VP; } else { DISABLE_VP; }
	if (val & VN) { ENABLE_VN; } else { DISABLE_VN; }
	if (val & WP) { ENABLE_WP; } else { DISABLE_WP; }
	if (val & WN) { ENABLE_WN; } else { DISABLE_WN; }

	u8 admux = phasesADMUX[phase];
	u8 acsr = phasesACSR[phase];
	ADMUX = admux;
	ACSR = acsr;

	if (state == STATE_NORMAL || state == STATE_STABILIZING)
	{
		TCNT0 = 255 - 50;
		timer0 = 0;
		TCCR0 = _BV(CS01);
		TIMSK |= _BV(TOIE0);
	}

	lastCommutationTime = ticks;
}

SIGNAL(TIMER0_OVF_vect)
{
	if (state == STATE_STARTING || state == STATE_CHANGING)
	{
		// if (timer0 == 0)
		// {
			// enableAC ();
			// // TIMSK &= ~_BV(TOIE0);
			// // TCCR0 = 0;
			// timer0 = 1;

			// TCNT0 = speedTime;
			// TCNT0 = 1;
			// TCCR0 = _BV(CS01) | _BV(CS00);
			// TIMSK |= _BV(TOIE0);
		// }
		// else
		{
			TCNT0 = speedTime;
			phase++;
			if (phase == 6)
			{
				phase = 0;

				if (state == STATE_CHANGING)
				{
					bldcSetDuty (110);
					TCCR0 = 0;
					state = STATE_NORMAL;
					setPhaseAC ();
					return;
				}
			}
			setPhaseAC ();
		}
	}
	else
	{
		enableAC ();
		TIMSK &= ~_BV(TOIE0);
		TCCR0 = 0;
	}
}

SIGNAL(ANA_COMP_vect)
{
	disableAC ();
	IO_TOGGLE(LED);
	if (state == STATE_NORMAL || state == STATE_STABILIZING)
	{
		// lastPhaseTimeValid = lastPhaseTime;
		// lastPhaseTime = 0;
		// IO_TOGGLE(EXT1);

		phase++;
		if (phase == 6)
		{
			// IO_TOGGLE(SDA);
			phase = 0;
		}
		setPhaseAC ();
		cps++;
	}
}

#include "comm.h"

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
			cli();
			validCPS = cps * CPS_PER_SEC;
			sei();
			cps = 0;
		}
	}
}

void setupStoppedState ()
{
	DISABLE_ALL();
	disableAC ();
	state = STATE_STOPPED;
}
void setupStartingState ()
{
	delay = 0;
	speedTime = 1;

	DISABLE_ALL();
	_delay_ms (100);
	phase = 0;
	setPhaseAC ();
	_delay_ms (100);

	enableAC ();

	TCNT0 = speedTime;
	TIMSK |= _BV(TOIE0);
	TCCR0 = _BV(CS01) | _BV(CS00);

	state = STATE_STARTING;
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

	TCCR1A = _BV(WGM10);
	TCCR1B = _BV(WGM12) | _BV(CS10);
	TCCR2 = _BV(WGM21) | _BV(WGM20) | _BV(CS20);
	TIMSK |= _BV(TOIE2);

	OCR2 = 20;
	OCR1B = 20;
	OCR1A = 20;

	ADCSRA = 0;
	SFIOR = _BV(ACME);

	setupStartingState();

	// debugInit();
	commInit();

	sei();

	for (;;)
	{
		switch (state)
		{
		case STATE_STOPPED:
			break;
		case STATE_STARTING:
			_delay_ms (10);
			delay++;

			const uint8_t maxSpeed = 100;
			if (delay >= 5 && speedTime < maxSpeed)
			{
				speedTime += 2;
				delay = 0;
				if (speedTime >= maxSpeed)
				{
					/*if (state == STATE_STARTING)
						{
						TCCR0 = _BV(CS01) | _BV(CS00);
						speedTime = 0;
						state = STATE_STARTING2;
						}
						else if (state == STATE_STARTING2)
						{
						}*/
					// bldcSetDuty (110);
					state = STATE_CHANGING;
				}
			}
			break;
		case STATE_CHANGING:
			break;
		case STATE_STABILIZING:
			break;
		case STATE_NORMAL:
			_delay_ms(10);
			cli();
			uint16_t lct = lastCommutationTime;
			sei();
			if (ticks - lct > 10)
			{
				setupStoppedState();
				setupStartingState();
			}
			break;
		}
		commProcess();
	}
}

