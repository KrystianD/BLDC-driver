#include <public.h>
#include "hardware.h"
#include "bldc.h"

volatile uint16_t cps = 0, validCPS = 0;

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
