#ifndef __BLDC_H__
#define __BLDC_H__

#include <stdint.h>

// cps - commutations per second
extern volatile uint16_t cps, validCPS;

// inverted
#define ACSR_RISING  (_BV(ACIS1))
#define ACSR_FAILING (_BV(ACIS1) | _BV(ACIS0))

#define TCCR1A_BASE (_BV(WGM10))
#define TCCR1B_BASE (_BV(WGM12) | _BV(CS10))
#define TCCR2_BASE (_BV(WGM21) | _BV(WGM20) | _BV(CS20))

typedef struct
{
	uint8_t port;
	uint8_t tccr2, tccr1a;
	uint8_t admux, acsr;
} TPhase;

TPhase phases[6];

void bldcInit();
void bldcProcess();

void bldcSetDuty(uint8_t duty);

#endif
