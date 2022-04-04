/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

 VMware Tools guest protocol device driver
******************************************/
#include <main.h>
#include <memory.h>
#include <video.h>
#include <string.h>
#include <idt.h>

#ifdef EXPERIMENTAL_VMWARE

#define VMWARE_MAGIC  (0x564D5868)
#define VMWARE_PORT   (0x5658)
#define VMWARE_PORTHB (0x5659)
#define CMD_GETVERSION  (0xA)
#define CMD_ABSPTR_DATA (0x27)
#define CMD_ABSPTR_STAT (0x28)
#define CMD_ABSPTR_CMND (0x29)
#define ABSPTR_ENABLE   (0x45414552)
#define ABSPTR_RELATIVE (0xF5)
#define ABSPTR_ABSOLUTE (0x53424152)

//vmware.asm
void VmwASend();
void VmwASendHb();
void VmwAGetHb();

typedef struct
{
	uint32_t size;
	uint32_t command;
	uint32_t source;
	uint32_t destination;
}
VmwCommand;
typedef struct
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
}
VmwCommandResult;

extern VmwCommand       VmwCommandInput;
extern VmwCommandResult VmwCommandOutput;

VmwCommandResult* VmwSend(VmwCommand input)
{
	VmwCommandInput = input;
	VmwASend();
	return &VmwCommandOutput;
}
VmwCommandResult* VmwSendHb(VmwCommand input)
{
	VmwCommandInput = input;
	VmwASendHb();
	return &VmwCommandOutput;
}
VmwCommandResult* VmwGetHb(VmwCommand input)
{
	VmwCommandInput = input;
	VmwAGetHb();
	return &VmwCommandOutput;
}

bool gVmwAttemptedDetect, gVmwDetected;
bool VmwDetect()
{
	if (gVmwAttemptedDetect)
		return gVmwDetected;
	
	VmwCommand command;
	memset (&command, 0, sizeof command);
	
	command.command = CMD_GETVERSION;
	
	VmwCommandResult *pResult = VmwSend (command);
	
	gVmwDetected = (pResult->ebx == VMWARE_MAGIC);
	return gVmwDetected;
}

uint8_t gVmwCounter = 0;

volatile uint32_t gVmwCounter2 = 0;

void OnUpdateMouse(uint8_t flags, uint8_t Dx, uint8_t Dy, __attribute__((unused)) uint8_t Dz);
void VmwAbsCursorIrqA();
void VmwAbsCursorIrq()
{
	gVmwCounter++;
	gVmwCounter2++;
	
	//EOI
	WritePort (0x20,0x20);
	WritePort (0xA0,0x20);
	
	// drop byte from PS/2 buffer
	ReadPort (0x60);
	
	if (gVmwCounter == 3)
	{
		gVmwCounter = 0;
		
		// read status
		VmwCommand command;
		memset (&command, 0, sizeof command);
		command.command = CMD_ABSPTR_STAT;
		command.size    = 0;
		
		VmwCommandResult sOut = *VmwSend (command);
		
		if (sOut.eax == 0xffff0000) // Error
		{
			LogMsg("VMware mouse device error");
			KeStopSystem();
		}
		
		short nPackets = (short)(sOut.eax) / 4;
		
		for (int i = 0; i < nPackets; i++)
		{
			VmwCommand mousePkt;
			memset (&mousePkt, 0, sizeof mousePkt);
			mousePkt.command = CMD_ABSPTR_DATA;
			mousePkt.size    = 4;
			
			VmwCommandResult sPkt = *VmwSend (mousePkt);
			
			uint16_t buttons = (uint16_t)(sPkt.eax);
			bool lmb, rmb, mmb;
			lmb = (buttons & 0x20) != 0;
			rmb = (buttons & 0x10) != 0;
			mmb = (buttons & 0x08) != 0;
			
			uint16_t scaled_x = (uint16_t)(sPkt.ebx);
			uint16_t scaled_y = (uint16_t)(sPkt.ecx);
			uint16_t n_scroll = (uint16_t)(sPkt.edx);
			
			// LogMsg
			//SLogMsg("Buttons: %d-%d-%d, %d %d  <->%d", lmb, mmb, rmb, scaled_x, scaled_y, n_scroll);
			OnUpdateMouse (
				(lmb ? MOUSE_FLAG_L_BUTTON : 0) |
				(rmb ? MOUSE_FLAG_R_BUTTON : 0) |
				(mmb ? MOUSE_FLAG_M_BUTTON : 0),
				0,
				0,
				0
			);
			
			// the mouse coordinates are scaled to the range (0x0000, 0xffff) independently
			// in each dimension, so scale them back
			uint32_t
			x = (scaled_x * GetScreenWidth ()) / 0xFFFF,
			y = (scaled_y * GetScreenHeight()) / 0xFFFF;
			
			SetMousePos (x, y);
		}
	}
}

bool VmwInit()
{
	if (!VmwDetect())
	{
		LogMsg("VMware device not detected");
		return false;
	}
	
	LogMsg("VMware device detected, initializing");
	
	SLogMsg("Enabling abscursor ...");
	
	VmwCommand command;
	memset (&command, 0, sizeof command);
	
	command.command = CMD_ABSPTR_CMND;
	command.size    = ABSPTR_ENABLE;
	VmwSend (command);
	SLogMsg("AbsPtr Cmnd...");
	
	command.command = CMD_ABSPTR_STAT;
	command.size    = 0;
	VmwSend (command);
	SLogMsg("AbsPtr Stat...");
	
	SLogMsg("AbsPtr Interrupt hooked...");
	SetupPicInterrupt (0x0C, VmwAbsCursorIrqA);
	
	command.command = CMD_ABSPTR_DATA;
	command.size    = 1;
	VmwSend (command);
	SLogMsg("AbsPtr Data...");
	
	
	command.command = CMD_ABSPTR_CMND;
	command.size    = ABSPTR_ABSOLUTE;
	VmwSend (command);
	SLogMsg("AbsPtr Cmnd2...");
	
	
	
	
	LogMsg("Done!");
	return true;
}


#endif

