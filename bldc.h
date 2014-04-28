#ifndef __BLDC_H__
#define __BLDC_H__

#include <stdint.h>

#define UP (1 << 0)
#define UN (1 << 1)
#define VP (1 << 2)
#define VN (1 << 3)
#define WP (1 << 4)
#define WN (1 << 5)

#define ACU (1l << 6)
#define ACV (1l << 7)
#define ACW (1l << 8)

#define ACFAILING (0l << 9)
#define ACRISING (1l << 9)
// cps - commutations per second
extern volatile uint16_t cps, validCPS;

void bldcInit();
void bldcProcess();

void bldcSetDuty(uint8_t duty);

#endif
