/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

            Serial port module
******************************************/
#include <uart.h>

const short g_uart_port_bases[] = { 0x3F8, 0x2F8 };//, 0x3E8, 0x2E8 };

void UartOnInterrupt(uint8_t com_num)
{
	// PIC EOI
	WritePort (0x20, 0x20);
	WritePort (0xA0, 0x20);
	
	SLogMsg("Got uart interrupt on COM%d!", com_num + 1);
}

short UartGetPortBase(uint8_t com_num)
{
	return g_uart_port_bases[com_num];
}

void UartInit(uint8_t com_num)
{
	//COM1 and COM2
	if (com_num >= 2)
	{
		LogMsg("Cannot initialize COM%d, we don't know what it is", com_num + 1);
		return;
	}
}

void UartWriteSingleChar(uint8_t com_num, char c)
{
	
}

char UartReadSingleChar(uint8_t com_num)
{
	
}
