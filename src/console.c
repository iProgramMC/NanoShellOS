//  ***************************************************************
//  console.c - Creation date: 10/12/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#include <console.h>
#include <string.h>
#include <print.h>
#include <misc.h>

extern Console* g_currentConsole;

bool CoInputBufferEmpty()
{
	return !CoAnythingOnInputQueue (g_currentConsole);
}

extern void KeTaskDone();

char CoGetChar()
{
	while (!CoAnythingOnInputQueue (g_currentConsole))
	{
		// TODO: Maybe suspend this task unless there's something on an input queue.
		WaitMS(1);
	}
	
	return CoReadFromInputQueue (g_currentConsole);
}

void CoKickOff()
{
	CoInitAsE9Hack(&g_debugConsole);
	CoInitAsE9Hack(&g_debugSerialConsole);
}

void CoGetString(char* buffer, int max_size)
{
	int index = 0, max_length = max_size - 1;
	//index represents where the next character we type would go
	while (index < max_length)
	{
		//! has to stall
		char k = CoGetChar();
		if (k == '\n')
		{
			//return:
			LogMsgNoCr("%c", k);
			buffer[index++] = 0;
			return;
		}
		else if (k == '\b')
		{
			if (index > 0)
			{
				LogMsgNoCr("%c", k);
				index--;
				buffer[index] = 0;
			}
		}
		else
		{
			buffer[index++] = k;
			LogMsgNoCr("%c", k);
		}
	}
	LogMsg("");
	buffer[index] = 0;
}


void CLogMsg (Console* pConsole, const char* fmt, ...)
{
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	sprintf (cr + strlen(cr), "\n");
	CoPrintString(pConsole, cr);
	
	va_end(list);
}

void CLogMsgNoCr (Console* pConsole, const char* fmt, ...)
{
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	CoPrintString(pConsole, cr);
	
	va_end(list);
}

void LogMsg (const char* fmt, ...)
{
	KeVerifyInterruptsEnabled;
	
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	sprintf (cr + strlen(cr), "\n");
	CoPrintString(g_currentConsole, cr);
	
	va_end(list);
}

void LogMsgNoCr (const char* fmt, ...)
{
	KeVerifyInterruptsEnabled;
	
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	CoPrintString(g_currentConsole, cr);
	
	va_end(list);
}

void ILogMsg (const char* fmt, ...)
{
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	sprintf (cr + strlen(cr), "\n");
	CoPrintString(&g_debugConsole, cr);
	
	va_end(list);
}

void ILogMsgNoCr (const char* fmt, ...)
{
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	CoPrintString(&g_debugConsole, cr);
	
	va_end(list);
}

void SLogMsg (const char* fmt, ...){
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	sprintf (cr + strlen(cr), "\n");
	CoPrintString(&g_debugSerialConsole, cr);
	
	va_end(list);
}

void SLogMsgNoCr (const char* fmt, ...)
{
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	CoPrintString(&g_debugSerialConsole, cr);
	
	va_end(list);
}

const char* g_uppercaseHex = "0123456789ABCDEF";

void LogHexDumpData (void* pData, int size)
{
	uint8_t* pDataAsNums = (uint8_t*)pData, *pDataAsText = (uint8_t*)pData;
	char c[7], d[4];
	c[5] = 0;   d[2] = ' ';
	c[6] = ' '; d[3] = 0;
	c[4] = ':';
	
	#define BYTES_PER_ROW 16
	for (int i = 0; i < size; i += BYTES_PER_ROW) {
		// print the offset
		c[0] = g_uppercaseHex[(i & 0xF000) >> 12];
		c[1] = g_uppercaseHex[(i & 0x0F00) >>  8];
		c[2] = g_uppercaseHex[(i & 0x00F0) >>  4];
		c[3] = g_uppercaseHex[(i & 0x000F) >>  0];
		LogMsgNoCr("%s", c);
		
		for (int j = 0; j < BYTES_PER_ROW; j++) {
			uint8_t p = *pDataAsNums++;
			d[0] = g_uppercaseHex[(p & 0xF0) >> 4];
			d[1] = g_uppercaseHex[(p & 0x0F) >> 0];
			LogMsgNoCr("%s", d);
		}
		LogMsgNoCr("   ");
		for (int j = 0; j < BYTES_PER_ROW; j++) {
			char c = *pDataAsText++;
			if (c < ' ') c = '.';
			LogMsgNoCr("%c",c);
		}
		LogMsg("");
	}
}


