#include <nsstandard.h>

/* NES specific */
#include <nes.h>
#include <ppu.h>
#include <cpu.h>
#include <cartridge.h>
#include <controller.h>

Window* g_pWindow = NULL;

void RequestRepaint(Window* pWindow);
void die(const char* fmt, ...)
{
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	LogMsg("NESEMU ERROR!");
	LogMsg(cr);
	
	va_end(list);
	
	if (g_pWindow)
	{
		LogMsg("A window was created, closing...");
		DestroyWindow(g_pWindow);
		while (HandleMessages(g_pWindow))
		{
			LogMsg("Still waiting for an eventual window destruction...");
		}
		g_pWindow = NULL;
	}
	
	exit (1);
}

/*
	BITS:
	
	0-ON/OFF
	1-A
	2-B
	3-Select
	4-Start
	5-Up
	6-Down
	7-Left
	8-Right
*/

uint16_t cont_state = 0;
uint8_t nes_key_state(uint8_t b)
{
	return (cont_state & (1 << b)) ? 1 : 0;
}

uint8_t nes_key_state_ctrl2(uint8_t b)
{
    switch (b)
    {
        case 0: // On / Off
            return 1;
        case 1: // A
            return 0;
        case 2: // B
            return 0;
        case 3: // SELECT
            return 0;
        case 4: // START
            return 0;
        case 5: // UP
            return 0;
        case 6: // DOWN
            return 0;
        case 7: // LEFT
            return 0;
        case 8: // RIGHT
            return 0;
        default:
            return 1;
    }
}
#define SIZE_MULT 2//more than 1 = slower.
const uint32_t* g_screenBitmap = NULL;
void CALLBACK WndProc (Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		/*case EVENT_CLOSE:
		case EVENT_DESTROY:
			break;*/
		case EVENT_PAINT:
		{
			Image image;
			image.width  = 256;
			image.height = 240;
			image.framebuffer = g_screenBitmap;
			VidBlitImageResize(&image, 0, 0, 256*SIZE_MULT, 240*SIZE_MULT);
			break;
		}
		case EVENT_KEYRAW:
		{
			cont_state |= 1;
			bool released = parm1 & 0x80;
			unsigned char at = parm1 & 0x7F;
			switch (at)
			{
				case KEY_ARROW_UP:    if (released) cont_state &= ~(1<<5); else cont_state |= (1<<5); break;
				case KEY_ARROW_DOWN:  if (released) cont_state &= ~(1<<6); else cont_state |= (1<<6); break;
				case KEY_ARROW_LEFT:  if (released) cont_state &= ~(1<<7); else cont_state |= (1<<7); break;
				case KEY_ARROW_RIGHT: if (released) cont_state &= ~(1<<8); else cont_state |= (1<<8); break;
				case KEY_Z:           if (released) cont_state &= ~(1<<1); else cont_state |= (1<<1); break;
				case KEY_X:           if (released) cont_state &= ~(1<<2); else cont_state |= (1<<2); break;
				case KEY_SPACE:       if (released) cont_state &= ~(1<<3); else cont_state |= (1<<3); break;
				case KEY_ENTER:       if (released) cont_state &= ~(1<<4); else cont_state |= (1<<4); break;
			}
			cont_state |= 1;
			break;
		}
		default:
			DefaultWindowProc (pWindow, messageType, parm1, parm2);
			break;
	}
}

int main(int argc, char** argv)
{
	if ((*argv)[0] == 'e')
		argv++, argc--;
	
	LogMsg("Basic NES emulator");
	LogMsg("Copyright (C) 2018, 2019, franzflasch (original: https://github.com/franzflasch/nes_emu)");
	LogMsg("Copyright (C) 2022, iProgramInCpp");
	LogMsg("This program is licensed under the GNU General Public License V3.0.");
	
	//char pRomFile[] = "/Fat1/mario.nes";//hardcoded for now.
	// load last parm instead
	char *pRomFile = NULL;
	
	if (argc > 1)
	{
		pRomFile = strdup (argv[1]);
	}
	else
	{
		pRomFile = InputBox(NULL, "Type in the name of a ROM file to open", "NES Emulator", "/Fat1/mario.nes");
		if (!pRomFile)
		{
			die("No file");
			return 1;
		}
	}
	
	
	// Initialize globals
    static nes_ppu_t nes_ppu;
    static nes_cpu_t nes_cpu;
    static nes_cartridge_t nes_cart;
    static nes_mem_td nes_memory;// = { { 0 } };
	memset (&nes_memory, 0, sizeof nes_memory);

    uint32_t cpu_clocks = 0;
    uint32_t ppu_clocks = 0;
    uint32_t ppu_rest_clocks = 0;
    uint32_t ppu_clock_index = 0;
    uint8_t ppu_status = 0;

	// Setup things
	nes_cart_init (&nes_cart, &nes_memory);
	
	// Load ROM
	if (nes_cart_load_rom (&nes_cart, pRomFile) != 0)
	{
		die ("ROM not found?");
	}
	
	MmKernelFree (pRomFile);
	
	// Init CPU
	nes_cpu_init (&nes_cpu, &nes_memory);
	nes_cpu_reset(&nes_cpu);
	
	// Init PPU
	nes_ppu_init (&nes_ppu, &nes_memory);
	
	// Initialize GUI
	Window* pWindow = CreateWindow ("nes_emu", 200,200, 256*SIZE_MULT, 240*SIZE_MULT, WndProc, 0);
	if (!pWindow)
	{
		die ("Could not create window");
	}
	
	g_pWindow = pWindow;
	int nextTickIn;
	bool fix = false;
	while (1)
	{
		nextTickIn = GetTickCount() + 16 + fix;
		fix ^= 1;
		// NES core loop:
		for (;;)
		{
			cpu_clocks = 0;
            if(!ppu_rest_clocks)
            {
                if(ppu_status & PPU_STATUS_NMI)
                    cpu_clocks += nes_cpu_nmi(&nes_cpu);
                cpu_clocks += nes_cpu_run(&nes_cpu);
            }

            /* the ppu runs at a 3 times higher clock rate than the cpu
            so we need to give the ppu some clocks here to catchup */
            ppu_clocks = (cpu_clocks*3) + ppu_rest_clocks;
            ppu_status = 0;
            for(ppu_clock_index=0;ppu_clock_index<ppu_clocks;ppu_clock_index++)
            {
                ppu_status |= nes_ppu_run(&nes_ppu, nes_cpu.num_cycles);
                if (ppu_status & PPU_STATUS_FRAME_READY) break;
                else ppu_rest_clocks = 0;
            }

            ppu_rest_clocks = (ppu_clocks - ppu_clock_index);

            nes_ppu_dump_regs(&nes_ppu);

            if (ppu_status & PPU_STATUS_FRAME_READY)
				break;//hey babe, new frame ready
		}
		
		g_screenBitmap = nes_ppu.screen_bitmap;
		RequestRepaint (pWindow);
		
		// Handle window messages.
		bool result = HandleMessages (pWindow);
		if (!result)
		{
			g_pWindow = NULL;
			break;
		}
		
		while (nextTickIn > GetTickCount());
	}
	
	die("Exited");
	
	return 0;
}
