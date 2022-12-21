/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
            Notepad application

             Main source file
******************************************/
#include <nanoshell/nanoshell.h>

#define WAVE_WIDTH  (320)
#define WAVE_HEIGHT (96 + TITLE_BAR_HEIGHT + 6)

#define SAMPLES_PER_TICK (735)
#define SAMPLE_RATE      (44100)
#define MIN_MS_PER_TICK  (10)
#define BYTES_PER_SAMPLE (2) // must be a power of two, otherwise things will break!!

enum
{
	NO_CONTROL,
	
	C_SEEK_BAR,
	C_FILE_LABEL,
	C_START_LABEL,
	C_END_LABEL,
	C_PLAY_BUTTON,
	C_STOP_BUTTON,
	C_OPEN_BUTTON,
	C_HELP_BUTTON,
};

Window * g_pWindow;

bool g_bPlaying;

int g_fileFD = -1, g_fileCurrentPlace;
int g_soundFD = -1;
int g_tickLastTicked = 0;

bool g_bDontUpdateScrollBar = false;

const char * GetPreferredSoundDevice()
{
	return "/Device/Sb16"; // TODO
}

void FileOpenDialog(char buffer[], size_t sizeBuf)
{
	//strcpy(buffer, "/Ext0/lma.raw");
	
	char * out = InputBox(g_pWindow, "Type in the file path of an audio file to play.", "Wave Player", "");
	
	strncpy(buffer, out, sizeBuf);
	
	buffer[sizeBuf - 1] = 0;
	
	MmKernelFree(out);
}

void Help()
{
	MessageBox(g_pWindow,
		"NanoShell Wave Player\n"
		"\n"
		"This program (currently) allows you to play raw signed 16-bit mono PCM sample data as sound.\n"
		"Before running this program, ensure that the Sound Blaster 16 (the only sound device currently supported by NanoShell) "
		"is available by checking the path '/Device/Sb16'.\n\n"
		"Play/Pause - Starts/stops playback of the current sound.\n"
		"Open... - Requests to open a file.\n"
		"Stop - Stops playback and closes the file.",
		"Wave Player", MB_OK);
}

void InitSound()
{
	g_soundFD = open(GetPreferredSoundDevice(), O_WRONLY);
	
	if (g_soundFD < 0)
	{
		char buffer[4096];
		snprintf(buffer, sizeof buffer, "Could not open the sound device %s for writing: %s. The application will now exit.", GetPreferredSoundDevice(), ErrNoStr(g_soundFD));
		MessageBox(NULL, buffer, "Wave Player", MB_OK | (ICON_ERROR << 16));
		
		g_soundFD = -1;
		return;
	}
	
	int newSampleRate = SAMPLE_RATE / 2;
	
	ioctl(g_soundFD, IOCTL_SOUNDDEV_SET_SAMPLE_RATE, &newSampleRate);
}

void TeardownSound()
{
	if (g_soundFD < 0) return;
	
	close(g_soundFD);
	g_soundFD = -1;
}

void Play()
{
	g_bPlaying = true;
	SetLabelText(g_pWindow, C_PLAY_BUTTON, "Pause");
	CallControlCallback(g_pWindow, C_PLAY_BUTTON, EVENT_PAINT, 0, 0);
}

void Pause()
{
	g_bPlaying = false;
	SetLabelText(g_pWindow, C_PLAY_BUTTON, "Play");
	CallControlCallback(g_pWindow, C_PLAY_BUTTON, EVENT_PAINT, 0, 0);
}

void TickOnce()
{
	if (!g_bPlaying) return;
	
	if (GetTickCount() < g_tickLastTicked + MIN_MS_PER_TICK)
	{
		if (GetTickCount() < g_tickLastTicked + 2)
			sleep(MIN_MS_PER_TICK - 2);
		
		return; // don't actually tick now
	}
	
	// push 8192 samples to the sound device
	int16_t samples[SAMPLES_PER_TICK];
	
	uint32_t inread = read(g_fileFD, samples, sizeof samples);
	bool bAtEnd = (inread < sizeof samples);
	
	uint32_t outwrote = write(g_soundFD, samples, inread);
	
	g_fileCurrentPlace = tellf(g_fileFD);
	
	// update the scroll bar
	if (!g_bDontUpdateScrollBar)
	{
		SetScrollBarPos(g_pWindow, C_SEEK_BAR, bAtEnd ? GetFileSize() : g_fileCurrentPlace);
		CallControlCallback(g_pWindow, C_SEEK_BAR, EVENT_PAINT, 0, 0);
	}
	
	char buffer[32];
	int posSec = g_fileCurrentPlace / BYTES_PER_SAMPLE / SAMPLE_RATE;
	sprintf(buffer, "%02d:%02d", posSec / 60, posSec % 60);
	SetLabelText(g_pWindow, C_START_LABEL, buffer);
	CallControlCallback(g_pWindow, C_START_LABEL, EVENT_PAINT, 0, 0);
	
	g_tickLastTicked = GetTickCount();
	
	if (bAtEnd)
		g_bPlaying = false;
}

int GetFileSize()
{
	g_fileCurrentPlace = tellf(g_fileFD);
	
	lseek(g_fileFD, 0, SEEK_END);
	
	int size = tellf(g_fileFD);
	
	lseek(g_fileFD, g_fileCurrentPlace, SEEK_SET);
	
	return size;
}

void LoadFile(const char * filename)
{
	if (g_fileFD >= 0)
	{
		MessageBox(g_pWindow, "The currently playing file must be unloaded before a new file can be loaded.", "Wave Player", MB_OK);
		return;
	}
	
	g_fileFD = open(filename, O_RDONLY);
	
	if (g_fileFD < 0)
	{
		char buffer[2048];
		
		sprintf(buffer, "Could not open file '%s' for playback.\n\n%s", filename, ErrNoStr(g_fileFD));
		
		g_fileFD = -1;
		
		MessageBox(g_pWindow, buffer, "Wave Player", MB_OK | (ICON_ERROR << 16));
	}
	
	SetLabelText(g_pWindow, C_FILE_LABEL, filename);
	CallControlCallback(g_pWindow, C_FILE_LABEL, EVENT_PAINT, 0, 0);
	
	// Get the size of the raw file
	int size = GetFileSize();
	
	// Calculate its length in seconds.
	int lenSec = size / BYTES_PER_SAMPLE / SAMPLE_RATE;
	
	char buffer[32];
	sprintf(buffer, "%02d:%02d", lenSec / 60, lenSec % 60);
	
	SetLabelText(g_pWindow, C_END_LABEL, buffer);
	CallControlCallback(g_pWindow, C_END_LABEL, EVENT_PAINT, 0, 0);
	
	SetScrollBarMax(g_pWindow, C_SEEK_BAR, size < 1 ? 1 : size);
	CallControlCallback(g_pWindow, C_SEEK_BAR, EVENT_PAINT, 0, 0);
}

void CloseFile()
{
	if (g_fileFD < 0) return;
	
	close(g_fileFD);
	g_fileFD = -1;
	
	SetLabelText(g_pWindow, C_FILE_LABEL, "No file loaded");
	CallControlCallback(g_pWindow, C_FILE_LABEL, EVENT_PAINT, 0, 0);
	
	SetLabelText(g_pWindow, C_END_LABEL, "00:00");
	CallControlCallback(g_pWindow, C_END_LABEL, EVENT_PAINT, 0, 0);
	
	SetScrollBarMax(g_pWindow, C_SEEK_BAR, 1);
	CallControlCallback(g_pWindow, C_SEEK_BAR, EVENT_PAINT, 0, 0);
}

void SetPos(int pos)
{
	lseek(g_fileFD, pos & ~(BYTES_PER_SAMPLE - 1), SEEK_SET);
}

void CALLBACK WndProc (Window* pWindow, int msg, int parm1, int parm2)
{
	switch (msg)
	{
		case EVENT_UPDATE:
		{
			TickOnce();
			break;
		}
		case EVENT_CREATE:
		{
			Rectangle r;
			
			RECT (r, 12, 40 + TITLE_BAR_HEIGHT, WAVE_WIDTH - 25, 20);
			
			AddControl(pWindow, CONTROL_HSCROLLBAR, r, NULL, C_SEEK_BAR, (0 << 16) | (1000), 0);
			
			RECT (r, 12, 8 + TITLE_BAR_HEIGHT, WAVE_WIDTH - 24, 15);
			
			AddControl(pWindow, CONTROL_TEXT, r, "No file loaded", C_FILE_LABEL, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			RECT (r, 12, 24 + TITLE_BAR_HEIGHT, WAVE_WIDTH / 2, 15);
			
			AddControl(pWindow, CONTROL_TEXT, r, "00:00", C_START_LABEL, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			int right = WAVE_WIDTH - 12;
			
			RECT (r, right - WAVE_WIDTH / 2, 24 + TITLE_BAR_HEIGHT, WAVE_WIDTH / 2, 15);
			
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "00:00", C_END_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_RJUSTIFY | TEXTSTYLE_FORCEBGCOL);
			
			int buttonWidth = 68, buttonWidthGap = buttonWidth + 8;
			RECT (r, 12 + 0 * buttonWidthGap, 64 + TITLE_BAR_HEIGHT, buttonWidth, 30);
			AddControl(pWindow, CONTROL_BUTTON, r, "Play", C_PLAY_BUTTON, 0, 0);
			RECT (r, 12 + 1 * buttonWidthGap, 64 + TITLE_BAR_HEIGHT, buttonWidth, 30);
			AddControl(pWindow, CONTROL_BUTTON, r, "Stop", C_STOP_BUTTON, 0, 0);
			RECT (r, 12 + 2 * buttonWidthGap, 64 + TITLE_BAR_HEIGHT, buttonWidth, 30);
			AddControl(pWindow, CONTROL_BUTTON, r, "Open...", C_OPEN_BUTTON, 0, 0);
			RECT (r, 12 + 3 * buttonWidthGap, 64 + TITLE_BAR_HEIGHT, buttonWidth, 30);
			AddControl(pWindow, CONTROL_BUTTON, r, "Help", C_HELP_BUTTON, 0, 0);
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			g_bDontUpdateScrollBar = true;
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			g_bDontUpdateScrollBar = false;
			break;
		}
		case EVENT_COMMAND:
		{
			switch (parm1)
			{
				case C_OPEN_BUTTON:
				{
					char buffer[PATH_MAX+2];
					FileOpenDialog(buffer, sizeof buffer);
					LoadFile(buffer);
					break;
				}
				case C_PLAY_BUTTON:
				{
					if (g_bPlaying)
						Pause();
					else
						Play();
					break;
				}
				case C_HELP_BUTTON:
				{
					Help();
					break;
				}
				case C_STOP_BUTTON:
				{
					CloseFile();
					break;
				}
			}
			break;
		}
		case EVENT_SCROLLDONE:
		{
			g_bDontUpdateScrollBar = false;
			SetPos(GetScrollBarPos(g_pWindow, C_SEEK_BAR));
			break;
		}
		case EVENT_DESTROY:
		{
			CloseFile();
			TeardownSound();
			break;
		}
	}
	DefaultWindowProc(pWindow, msg, parm1, parm2);
}

int NsMain (UNUSED int argc, UNUSED char** argv)
{
	InitSound();
	
	if (g_soundFD < 0)
	{
		return 1;
	}
	
	Window *pWindow = g_pWindow = CreateWindow ("Wave Player", CW_AUTOPOSITION, CW_AUTOPOSITION, WAVE_WIDTH, WAVE_HEIGHT, WndProc, 0);
	
	if (!pWindow)
	{
		LogMsg("Could not create window.");
		return 1;
	}
	
	while (HandleMessages (pWindow))
	{
		if (g_bPlaying)
		{
			RegisterEvent(g_pWindow, EVENT_UPDATE, 0, 0);
		}
	}
	
	return 0;
}
