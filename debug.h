#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>

#define UART_BAUD     UART_BAUD_SELECT (9600,F_CPU)
#define UART_DATABITS 8
#include "UART.h"

void debugInit();

#define DEBUG(x,...) printf(x "\r\n", ##__VA_ARGS__)

#endif
