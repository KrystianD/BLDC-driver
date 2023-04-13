#include <public.h>
#include "hardware.h"
#include "bldc.h"

#include "speeds.h"
#include "settings.h"

// settings
volatile uint8_t SET_startupDuty = STARTUP_DUTY;

volatile uint16_t cps = 0, validCPS = 0;

// stabilizing
volatile uint16_t stabilizingRemTime;

// starting
volatile uint8_t speedIdx = 0;
volatile uint8_t delay = 0;

volatile uint8_t enabled = 0;
volatile uint8_t desiredDuty = 80;
volatile uint8_t state = STATE_STOPPED;
volatile uint16_t lastCommutationTime = 0;

register uint8_t phase asm("r4");

TPhase phases[7 + 7] =
// TPhase phasesForward[7] =
{
	// FORWARD
	// UP + VN + W_FALLING
	{ .port = P_UP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_W, .acsr = ACSR_FALLING },
	
	// UP + WN + V_RISING
	{ .port = P_UP_PIN_BV, .tccr2 = TCCR2_BASE | WN_TCCR2, .tccr1a = TCCR1A_BASE | WN_TCCR1A, .admux = ADMUX_V, .acsr = ACSR_RISING },
	
	// VP + WN + U_FALLING
	{ .port = P_VP_PIN_BV, .tccr2 = TCCR2_BASE | WN_TCCR2, .tccr1a = TCCR1A_BASE | WN_TCCR1A, .admux = ADMUX_U, .acsr = ACSR_FALLING },
	
	// VP + UN + W_RISING
	{ .port = P_VP_PIN_BV, .tccr2 = TCCR2_BASE | UN_TCCR2, .tccr1a = TCCR1A_BASE | UN_TCCR1A, .admux = ADMUX_W, .acsr = ACSR_RISING },
	
	// WP + UN + V_FALLING
	{ .port = P_WP_PIN_BV, .tccr2 = TCCR2_BASE | UN_TCCR2, .tccr1a = TCCR1A_BASE | UN_TCCR1A, .admux = ADMUX_V, .acsr = ACSR_FALLING },
	
	// WP + VN + U_RISING
	{ .port = P_WP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_U, .acsr = ACSR_RISING },
	
	// repeated first entry for optimization
	// UP + VN + W_FALLING
	{ .port = P_UP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_W, .acsr = ACSR_FALLING },
	
	/* ------------------------------------------------------------------------------------------------------------------------------ */
	
	// BACKWARD
	// WP + VN + U_FALLING
	{ .port = P_WP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_U, .acsr = ACSR_FALLING },
	
	// WP + UN + V_RISING
	{ .port = P_WP_PIN_BV, .tccr2 = TCCR2_BASE | UN_TCCR2, .tccr1a = TCCR1A_BASE | UN_TCCR1A, .admux = ADMUX_V, .acsr = ACSR_RISING },
	
	// VP + UN + W_FALLING
	{ .port = P_VP_PIN_BV, .tccr2 = TCCR2_BASE | UN_TCCR2, .tccr1a = TCCR1A_BASE | UN_TCCR1A, .admux = ADMUX_W, .acsr = ACSR_FALLING },
	
	// VP + WN + U_RISING
	{ .port = P_VP_PIN_BV, .tccr2 = TCCR2_BASE | WN_TCCR2, .tccr1a = TCCR1A_BASE | WN_TCCR1A, .admux = ADMUX_U, .acsr = ACSR_RISING },
	
	// UP + WN + V_FALLING
	{ .port = P_UP_PIN_BV, .tccr2 = TCCR2_BASE | WN_TCCR2, .tccr1a = TCCR1A_BASE | WN_TCCR1A, .admux = ADMUX_V, .acsr = ACSR_FALLING },
	
	// UP + VN + W_RISING
	{ .port = P_UP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_W, .acsr = ACSR_RISING },
	
	// repeated first entry for optimization
	// WP + VN + U_FALLING
	{ .port = P_WP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_U, .acsr = ACSR_FALLING },
};

// prv
void bldc_setupStoppedState();
void bldc_setupStartingState();

void bldc_setPhase();
void bldc_setPhaseAC();

void bldcInit()
{
	SET_phasesOffset = 0;
}

void bldcProcess()
{
	switch (state)
	{
	case STATE_STOPPED:
		break;
	case STATE_STABILIZING:
		if (stabilizingRemTime == 0)
		{
			delay = 0;
			speedIdx = 0;
			
			TCNT0 = 0;
			TIMSK = TIMSK_BASE | _BV(TOIE0);
			TCCR0 = _BV(CS01) | _BV(CS00); // /64 = 8uS per tick = 2048uS per range
			TCCR0 = _BV(CS02);// | _BV(CS00);;
			
			state = STATE_STARTING;
		}
		else
		{
			_delay_ms(1);
			stabilizingRemTime--;
		}
		break;
	case STATE_STARTING:
		_delay_ms(1);
		delay++;
		
		if (delay >= 10)
		{
			delay = 0;
			if (speedIdx == sizeof(speeds) / sizeof(speeds[0]) - 1)
			{
				state = STATE_CHANGING;
			}
			else
			{
				speedIdx++;
			}
		}
		break;
	case STATE_CHANGING:
		break;
	case STATE_NORMAL:
		_delay_ms(10);
		cli();
		uint16_t lct = lastCommutationTime;
		uint16_t tmpTicks = ticks;
		sei();
		if (tmpTicks - lct > 100)
		{
			bldcDisable();
			bldcEnable();
		}
		break;
	}
}

void bldcEnable()
{
	enabled = 1;
	if (state == STATE_STOPPED)
	{
		bldc_setupStartingState();
	}
}
void bldcDisable()
{
	enabled = 0;
	bldc_setupStoppedState();
}

void bldcSetDuty(uint8_t duty)
{
	OCR1A = duty;
	OCR1B = duty;
	OCR2 = duty;
}
void bldcSetDesiredDuty(uint8_t duty)
{
	desiredDuty = duty;
	if (state == STATE_NORMAL || state == STATE_CHANGING)
		bldcSetDuty(duty);
}

inline void bldc_enableAC()
{
	ACSR |= _BV(ACIE);
	ACSR |= _BV(ACI);
}
inline void bldc_disableAC()
{
	ACSR &= ~_BV(ACIE);
	ACSR |= _BV(ACI);
}

void bldc_setupStoppedState()
{
	DISABLE_ALL();
	bldc_disableAC();
	state = STATE_STOPPED;
}
void bldc_setupStartingState()
{
	bldcSetDuty(SET_startupDuty);
	phase = 0;
	bldc_setPhaseAC();

	state = STATE_STABILIZING;
	stabilizingRemTime = STABILIZATION_TIME_MS;
}

inline void bldc_setPhase()
{
	TCNT1 = TCNT2 = 0;
	
	const volatile TPhase *p = &phases[phase + SET_phasesOffset];
	
	PORTB = p->port;
	TCCR2 = p->tccr2;
	TCCR1A = p->tccr1a;
}
inline void bldc_setPhaseAC()
{
	TCNT1 = TCNT2 = 0;
	
	const volatile TPhase *p = &phases[phase + SET_phasesOffset];
	
	PORTB = p->port;
	TCCR2 = p->tccr2;
	TCCR1A = p->tccr1a;
	ADMUX = p->admux;
	ACSR = p->acsr;
	
	TCNT0 = 255 - 50;
	TCCR0 = _BV(CS01);
	TIMSK = TIMSK_BASE | _BV(TOIE0);
	
	lastCommutationTime = ticks;
}

// interrupts
SIGNAL(TIMER0_OVF_vect)
{
	if (state == STATE_STARTING || state == STATE_CHANGING)
	{
		TCNT0 = speeds[speedIdx];
		phase++;
		if (phase == 6)
		{
			phase = 0;
			
			if (state == STATE_CHANGING)
			{
				bldcSetDuty(desiredDuty);
				TCCR0 = 0;
				state = STATE_NORMAL;
				bldc_setPhaseAC();
				return;
			}
		}
		bldc_setPhase();
	}
	else
	{
		bldc_enableAC();
		TIMSK = TIMSK_BASE; // disable TOIE0
		TCCR0 = 0;
	}
}

SIGNAL(ANA_COMP_vect)
{
	if (state == STATE_NORMAL)
	{
		phase++;
		bldc_setPhaseAC();
		if (phase == 6)
			phase = 0;
		cps++;
	}
	bldc_disableAC();
	IO_TOGGLE(LED);
}
