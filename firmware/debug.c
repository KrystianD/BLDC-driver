#ifdef DEBUG_ENABLED

#include <public.h>
#include <stdio.h>
#include "debug.h"

#define UART_BAUD     UART_BAUD_SELECT(38400,F_CPU)
#define UART_DATABITS 8
#include <UART.h>

int put(char c, FILE* stream)
{
	uartTX(c);
	return 0;
}
FILE mystdout = FDEV_SETUP_STREAM(&put, 0, _FDEV_SETUP_WRITE);

void debugInit()
{
	uartInit();
	stdout = &mystdout;
	
	DEBUG("OK");
}

#endif
