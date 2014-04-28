#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>

void debugInit();

#define DEBUG(x,...) printf(x "\r\n", ##__VA_ARGS__)

#endif
