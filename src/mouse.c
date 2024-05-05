/*****************************************
		NanoShell Operating System
	      (C) 2021 iProgramInCpp

            Mouse Driver module
******************************************/
#include <mouse.h>
#include <config.h>
#include <idt.h>

enum
{
	COMMAND_NONE = 0,
	COMMAND_RESET,
	COMMAND_SETDEFAULT,
	COMMAND_SETREPORT,
	COMMAND_GETDEVID,
	COMMAND_READDATA,
	COMMAND_SETSAMPRATE, COMMAND_SETSAMPRATE1,
	COMMAND_SETRESOLUTION, COMMAND_SETRESOLUTION1
};

uint8_t g_commandRunning, g_resetStages, g_mouseDeviceID = 0, g_mouseCycle;
bool g_mouseAvailable = false, g_mouseInitted = false, g_ps2MouseAvail = false, g_ps2DisableMovement = false;

MousePacket g_currentPacket;
bool g_discardPacket;

bool IsMouseAvailable()
{
	return g_mouseAvailable && g_mouseInitted;
}

//mouse comms//
#if 1
void MouseWaitN (uint8_t type, const char* waiter, int waiterL)
{
	if (!g_mouseAvailable) return;
	uint32_t _timeout = 1000000;
	
	if (type == 0)
	{
		while (_timeout--)
		{
			if (ReadPort (0x64) & 1) return;
			asm ("pause");
		}
	}
	else
	{
		while (_timeout--)
		{
			if (!(ReadPort (0x64) & 2)) return;
			asm ("pause");
		}
	}
	g_mouseAvailable = false;
	ILogMsg("PS/2 mouse took too long to respond, guessed it's not available. (%s:%d)",waiter,waiterL);
}
#define MouseWait(x) MouseWaitN(x, __FILE__, __LINE__)
void MouseWrite (uint8_t write)
{
	if (!g_mouseAvailable) return;
	MouseWait (1);
	WritePort (0x64, 0xD4);
	MouseWait (1);
	WritePort (0x60, write);
}
uint8_t MouseRead()
{
	if (!g_mouseAvailable)
		return 0xFF;
	MouseWait (0);
	return ReadPort (0x60);
}
#endif

//mouse commds//
#if 1
static const int g_sampleRateValues[] = {10,20,40,60,80,100,200};
int g_mouseSpeedMultiplier = 2, g_mouseSampRate = 6;
int GetMouseSpeedMultiplier()
{
	return g_mouseSpeedMultiplier;
}
int GetMouseSampRateMax()
{
	return ARRAY_COUNT(g_sampleRateValues);
}
void SetMouseSpeedMultiplier(int spd)
{
	spd &= 0b11;
	g_mouseSpeedMultiplier = spd;
	
	if (g_ps2MouseAvail)
	{
		g_commandRunning = COMMAND_SETRESOLUTION;
		MouseWrite(0xE8);
	}
}
void SetMouseSampleRate(int spd)
{
	if (spd < 0) spd = 0;
	if (spd >= GetMouseSampRateMax()) spd = ARRAY_COUNT(g_sampleRateValues);
	g_mouseSampRate = spd;
	
	//send Set Sample Rate command
	if (g_ps2MouseAvail)
	{
		g_commandRunning = COMMAND_SETSAMPRATE;
		MouseWrite(0xF3);
	}
}
#endif

//mouse irq//
#if 1
void MouseInterruptHandler()
{
	uint8_t b = MouseRead();
	if (g_commandRunning)
	{
		switch (g_commandRunning)
		{
			case COMMAND_RESET: {
				if (b == 0xFA)
				{
					//we are on good terms, return.
					if (g_resetStages <= 3)
					{
						g_resetStages++;
						return;
					}
				}
				else if (b == 0xAA && g_resetStages <= 1)
				{
					g_resetStages = 2;
					return;
				}
				else if (g_resetStages == 2)
				{
					g_resetStages = 3;
					
					//this is the mouse ID
					g_mouseDeviceID = b;
					return;
				}
				else if (g_resetStages == 3)
				{
					//extra data that we do not care about
					g_resetStages = 4;
					g_commandRunning = COMMAND_NONE;
				}
				break;
			}
			case COMMAND_SETDEFAULT: {
				//NanoShell V2 ignores this. Why?
				g_commandRunning = COMMAND_NONE;
				break;
			}
			case COMMAND_SETREPORT: {
				//NanoShell V2 ignores this. Why?
				g_commandRunning = COMMAND_NONE;
				break;
			}
			case COMMAND_SETSAMPRATE: {
				if (b == 0xFA)
				{
					//acknowledged.
					g_commandRunning = COMMAND_SETSAMPRATE1;
					MouseWrite (g_sampleRateValues[g_mouseSampRate]);
				}
				else
					g_commandRunning = COMMAND_NONE;
				break;
			}
			case COMMAND_SETRESOLUTION: {
				if (b == 0xFA)
				{
					//acknowledged.
					g_commandRunning = COMMAND_SETRESOLUTION1;
					MouseWrite (g_mouseSpeedMultiplier);
				}
				else
					g_commandRunning = COMMAND_NONE;
				break;
			}
			case COMMAND_SETSAMPRATE1:
			case COMMAND_SETRESOLUTION1:
			{
				g_commandRunning = COMMAND_NONE;
				break;
			}
			case COMMAND_GETDEVID: {
				//! What if the mouse ID was indeed 0xfa? Probably never the case
				if (b != 0xFA)
					g_mouseDeviceID = b; 
				
				g_commandRunning = COMMAND_NONE;
				break;
			}
		}
	}
	else
	{
		switch (g_mouseCycle)
		{
			case 0:
				g_currentPacket.flags = b;
				g_mouseCycle++;
				g_discardPacket = false;
				
				if (g_currentPacket.flags & (1 << 6) || g_currentPacket.flags & (1 << 7))
					g_discardPacket = 1;
				
				if (!(g_currentPacket.flags & (1 << 3)))
				{
					// WAIT UNTIL WE GET A 0x8, THEN PROCEED!!!!!!
					// This is a hack, and should not be kept.
					g_mouseCycle = 0;
				}
				break;
			case 1:
				g_currentPacket.xMov = b;
				g_mouseCycle++;
				break;
			case 2:
				g_currentPacket.yMov = b;
				if (g_mouseDeviceID == 0)
				{
					//some mice do not send scroll data too
					g_mouseCycle = 0;
					g_commandRunning = COMMAND_NONE;
					if (g_discardPacket)
					{
						g_discardPacket = false;
						return;
					}
					if (g_ps2DisableMovement)
					{
						g_currentPacket.xMov = 0;
						g_currentPacket.yMov = 0;
					}
					OnUpdateMouse (g_currentPacket.flags, g_currentPacket.xMov, g_currentPacket.yMov, 0);
				}
				else g_mouseCycle++;
				break;
			case 3:
				g_currentPacket.zMov = b;
				g_mouseCycle = 0;
				g_commandRunning = COMMAND_NONE;
				if (g_discardPacket)
				{
					g_discardPacket = false;
					return;
				}
				if (g_ps2DisableMovement)
				{
					g_currentPacket.xMov = 0;
					g_currentPacket.yMov = 0;
				}
				OnUpdateMouse (g_currentPacket.flags, g_currentPacket.xMov, g_currentPacket.yMov, g_currentPacket.zMov);
				break;
		}
	}
}
#endif

//mouse init//
#if 1

bool MouseShouldInit()
{
	ConfigEntry * pEntry = CfgGetEntry("Driver::PS2");
	
	// default: true
	if (!pEntry) return true;
	
	return strcmp(pEntry->value, "off") != 0;
}

void MouseInit()
{
	if (!MouseShouldInit())
	{
		return;
	}
	
	g_mouseAvailable = true;
	
	ILogMsg("Initializing PS/2 mouse driver... (If on real hardware, the OS may stop at this point)");
	//return;//don't have it for now
	
	KeRegisterIrqHandler(IRQ_MOUSE, MouseInterruptHandler, false);
	
	uint8_t _status;
	
	// Enable the auxiliary mouse device
	MouseWait (1);
	if (!g_mouseAvailable) return;
	WritePort (0x64, 0xA8);
	
	// Enable the interrupts
	MouseWait (1);
	if (!g_mouseAvailable) return;
	WritePort (0x64, 0x20);
	
	uint8_t b = ReadPort (0x60);
	
	//HACK!!! Some PCs (my HP laptop for instance) will actually return a
	//bitflipped config, so bits 7 and 3 are set.  Just flip the whole byte
	//so everything is in order.
	if ((b & 0x80) && (b & 0x08)) b = ~b;
	//if (b & 0x88) b = ~b;
	_status = (b | 2);
	
	MouseWait (1);
	if (!g_mouseAvailable) return;
	WritePort (0x64, 0x60);
	MouseWait (1);
	if (!g_mouseAvailable) return;
	WritePort (0x60, _status);
	
	//reset mouse
	g_commandRunning = COMMAND_RESET;
	MouseWrite (255);
	
	for (int i = 0; i < 20; i++)
	{
		WritePort(0x80, 0x00); //Wait a few seconds to make sure all the interrupts went through.
	}
	while (g_resetStages < 2) hlt;
	g_commandRunning = COMMAND_NONE;
	
	//halt for 3 bytes, because we're supposed to get them
	g_resetStages = 4;
	
	//tell the mouse to use default settings
	g_commandRunning = COMMAND_SETDEFAULT;
	MouseWrite (0xF6);
	while (g_commandRunning) hlt;
	
	g_commandRunning = COMMAND_SETREPORT;
	MouseWrite (0xF4);
	while (g_commandRunning) hlt;
	
	g_commandRunning = COMMAND_GETDEVID;
	MouseWrite (0xF2);
	while (g_commandRunning) hlt;
	
	SetMouseSampleRate(6);
	while (g_commandRunning) hlt;
	
	while (ReadPort(0x64) & 2)
		ReadPort (0x60);
	
	if (g_mouseAvailable)
	{
		g_mouseInitted = true;
		SetDefaultCursor();
		SetMouseVisible (true);
		SetMousePos(GetScreenSizeX() / 2, GetScreenSizeY() / 2);
		g_ps2MouseAvail = true;
	}
	else
	{
		ILogMsg("PS/2 Mouse failed to initialize!");
	}
}
#endif
