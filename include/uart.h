/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

            Serial port module
******************************************/
#ifndef _UART_H
#define _UART_H

#include <main.h>

#define COM1 0
#define COM2 1

void UartInit(uint8_t com_num);
void UartWriteSingleChar(uint8_t com_num, char c);
char UartReadSingleChar (uint8_t com_num);

#endif//_UART_H