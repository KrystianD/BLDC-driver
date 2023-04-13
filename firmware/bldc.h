#ifndef __BLDC_H__
#define __BLDC_H__

#include <stdint.h>

#define STATE_STOPPED     0
#define STATE_STABILIZING 1
#define STATE_STARTING    2
#define STATE_CHANGING    3
#define STATE_NORMAL      4

#define TIMSK_BASE (_BV(TOIE2))

// inverted
#define ACSR_RISING  (_BV(ACIS1))
#define ACSR_FALLING (_BV(ACIS1) | _BV(ACIS0))

#define TCCR1A_BASE (_BV(WGM10))
#define TCCR1B_BASE (_BV(WGM12) | _BV(CS10))
#define TCCR2_BASE (_BV(WGM21) | _BV(WGM20) | _BV(CS20))

typedef struct
{
	uint8_t port;
	uint8_t tccr2, tccr1a;
	uint8_t admux, acsr;
} TPhase;

// settings
register uint8_t SET_phasesOffset asm("r3");
extern volatile uint8_t SET_startupDuty;

// cps - commutations per second
extern volatile uint16_t cps, validCPS;

// stabilizing
extern volatile uint16_t stabilizingRemTime;

// starting
extern volatile uint8_t speedIdx;
extern volatile uint8_t delay;

extern volatile uint8_t enabled;
extern volatile uint8_t desiredDuty;
extern volatile uint8_t state;
extern volatile uint16_t lastCommutationTime;

extern TPhase phases[7 + 7];

void bldcInit();
void bldcProcess();

void bldcEnable();
void bldcDisable();

void bldcSetDuty(uint8_t duty);
void bldcSetDesiredDuty(uint8_t duty);


#endif
