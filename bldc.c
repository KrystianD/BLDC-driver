#include <public.h>
#include "hardware.h"
#include "bldc.h"

#include "speeds.h"

volatile uint16_t cps = 0, validCPS = 0;

volatile uint8_t enabled = 0;
volatile uint8_t desiredDuty = 80;
volatile uint8_t state = STATE_STOPPED;

volatile uint8_t phase = 0;
volatile uint8_t speedIdx = 0;
volatile uint8_t delay = 0;

volatile uint16_t lastCommutationTime = 0;

TPhase phases[7] =
{
	// UP + VN + W_FAILING
	{ .port = P_UP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_W, .acsr = ACSR_FAILING },
	
	// UP + WN + V_RISING
	{ .port = P_UP_PIN_BV, .tccr2 = TCCR2_BASE | WN_TCCR2, .tccr1a = TCCR1A_BASE | WN_TCCR1A, .admux = ADMUX_V, .acsr = ACSR_RISING },
	
	// VP + WN + U_FAILING
	{ .port = P_VP_PIN_BV, .tccr2 = TCCR2_BASE | WN_TCCR2, .tccr1a = TCCR1A_BASE | WN_TCCR1A, .admux = ADMUX_U, .acsr = ACSR_FAILING },
	
	// VP + UN + W_RISING
	{ .port = P_VP_PIN_BV, .tccr2 = TCCR2_BASE | UN_TCCR2, .tccr1a = TCCR1A_BASE | UN_TCCR1A, .admux = ADMUX_W, .acsr = ACSR_RISING },
	
	// WP + UN + V_FAILING
	{ .port = P_WP_PIN_BV, .tccr2 = TCCR2_BASE | UN_TCCR2, .tccr1a = TCCR1A_BASE | UN_TCCR1A, .admux = ADMUX_V, .acsr = ACSR_FAILING },
	
	// WP + VN + U_RISING
	{ .port = P_WP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_U, .acsr = ACSR_RISING },
	
	// repeated first entry for optimization
	// UP + VN + W_FAILING
	{ .port = P_UP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_W, .acsr = ACSR_FAILING },
};

TPhase phasesRev[7] =
{
	// WP + VN + U_FAILING
	{ .port = P_WP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_U, .acsr = ACSR_FAILING },
	
	// WP + UN + V_RISING
	{ .port = P_WP_PIN_BV, .tccr2 = TCCR2_BASE | UN_TCCR2, .tccr1a = TCCR1A_BASE | UN_TCCR1A, .admux = ADMUX_V, .acsr = ACSR_RISING },
	
	// VP + UN + W_FAILING
	{ .port = P_VP_PIN_BV, .tccr2 = TCCR2_BASE | UN_TCCR2, .tccr1a = TCCR1A_BASE | UN_TCCR1A, .admux = ADMUX_W, .acsr = ACSR_FAILING },
	
	// VP + WN + U_RISING
	{ .port = P_VP_PIN_BV, .tccr2 = TCCR2_BASE | WN_TCCR2, .tccr1a = TCCR1A_BASE | WN_TCCR1A, .admux = ADMUX_U, .acsr = ACSR_RISING },
	
	// UP + WN + V_FAILING
	{ .port = P_UP_PIN_BV, .tccr2 = TCCR2_BASE | WN_TCCR2, .tccr1a = TCCR1A_BASE | WN_TCCR1A, .admux = ADMUX_V, .acsr = ACSR_FAILING },
	
	// UP + VN + W_RISING
	{ .port = P_UP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_W, .acsr = ACSR_RISING },
	
	// repeated first entry for optimization
	// WP + VN + U_FAILING
	{ .port = P_WP_PIN_BV, .tccr2 = TCCR2_BASE | VN_TCCR2, .tccr1a = TCCR1A_BASE | VN_TCCR1A, .admux = ADMUX_U, .acsr = ACSR_FAILING },
};

// prv
void bldc_setupStoppedState();
void bldc_setupStartingState();

void bldc_setPhase();
void bldc_setPhaseAC();

void bldcInit()
{
}

void bldcProcess()
{
	switch (state)
	{
	case STATE_STOPPED:
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
		uint32_t lct = lastCommutationTime;
		sei();
		if (ticks - lct > 100)
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

static inline void bldc_enableAC()
{
	ACSR |= _BV(ACIE);
	ACSR |= _BV(ACI);
}
static inline void bldc_disableAC()
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
	delay = 0;
	speedIdx = 0;
	
	bldcSetDuty(130);
	bldcSetDuty(50);
	DISABLE_ALL();
	phase = 0;
	bldc_setPhaseAC();
	_delay_ms(100);
	
	bldc_enableAC();
	
	TCNT0 = 0;
	TIMSK = TIMSK_BASE | _BV(TOIE0);
	TCCR0 = _BV(CS01) | _BV(CS00); // /64 = 8uS per tick = 2048uS per range
	TCCR0 = _BV(CS02);// | _BV(CS00);;
	
	state = STATE_STARTING;
}

void bldc_setPhase()
{
	TCNT1 = TCNT2 = 0;
	
	const TPhase *p = &phases[phase];
	
	PORTB = p->port;
	TCCR2 = p->tccr2;
	TCCR1A = p->tccr1a;
}
void bldc_setPhaseAC()
{
	TCNT1 = TCNT2 = 0;
	
	const TPhase *p = &phases[phase];
	
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
