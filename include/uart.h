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
bool UartWriteSingleChar(uint8_t com_num, char c, bool block);
bool UartReadSingleChar (uint8_t com_num, char*c, bool block);

#endif//_UART_H