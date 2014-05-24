#include <public.h>
#include "hardware.h"
#include "bldc.h"

volatile uint16_t cps = 0, validCPS = 0;

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

void bldcInit()
{
}

void bldcProcess()
{
}

void bldcSetDuty(uint8_t duty)
{
	OCR1A = duty;
	OCR1B = duty;
	OCR2 = duty;
}
