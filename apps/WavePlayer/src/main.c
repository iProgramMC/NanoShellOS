/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
            Notepad application

             Main source file
******************************************/
#include <nanoshell/nanoshell.h>

#define WAVE_WIDTH  (320)
#define WAVE_HEIGHT (96)

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

typedef struct
{
	uint16_t nFormat;
	uint16_t nChannels;
	uint32_t nSampleRate;
	uint32_t nBytesPerSec;
	uint16_t nSomething;
	uint16_t nBitsPerSample;
}
__attribute__((packed))
WaveFormatData;

WaveFormatData g_wave_data = {
	.nFormat = 1,
	.nChannels = 1,
	.nSampleRate = SAMPLE_RATE,
	.nBytesPerSec = SAMPLE_RATE * 2,
	.nSomething = 0,
	.nBitsPerSample = 16,
};

Window * g_pWindow;

const char* g_LoadSoundFile;
bool g_LoadAndPlay;

bool g_bPlaying;

int g_fileFD = -1, g_fileCurrentPlace;
int g_soundFD = -1;
int g_tickLastTicked = 0;

int g_offsetFromStart = SAMPLE_RATE * 60, g_FileSize; // a full minute for testing

uint8_t * g_SampleBuffer = NULL;
uint32_t  g_SampleBufferSize = 0;

bool g_bDontUpdateScrollBar = false; // in bytes

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

void CloseFile();

int GetFileSize()
{
	return g_FileSize;
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
	
	uint32_t inread = read(g_fileFD, g_SampleBuffer, g_SampleBufferSize);
	bool bAtEnd = (inread < g_SampleBufferSize);
	uint32_t readSamples = g_SampleBufferSize / g_wave_data.nChannels / ((g_wave_data.nBitsPerSample + 7) / 8);
	
	int16_t samples[SAMPLES_PER_TICK];
	
	if (g_wave_data.nChannels == 1 && g_wave_data.nBitsPerSample == 16)
	{
		// A memcpy will do.
		memcpy(samples, g_SampleBuffer, g_SampleBufferSize);
	}
	else
	{
		// We have to copy each sample manually.
		// TODO: allow resampling
		if (g_wave_data.nBitsPerSample == 16)
		{
			int16_t* sampleData = (int16_t*)g_SampleBuffer;
			for (int i = 0; i < SAMPLES_PER_TICK; i++)
			{
				samples[i] = sampleData[i * g_wave_data.nChannels];
			}
		}
		else
		{
			CloseFile();
			g_bPlaying = false;
			char buffer[1024];
			sprintf(buffer, "A bit-per-sample rate of %d is not supported, only 16-bit PCM data is supported right now.", g_wave_data.nBitsPerSample);
			MessageBox(g_pWindow, buffer, "Wave Player", MB_OK | ICON_ERROR << 16);
			return;
		}
	}
	
	uint32_t outwrote = write(g_soundFD, samples, readSamples * sizeof(int16_t));
	
	g_fileCurrentPlace = tellf(g_fileFD) - g_offsetFromStart;
	
	// update the scroll bar
	if (!g_bDontUpdateScrollBar)
	{
		SetScrollBarPos(g_pWindow, C_SEEK_BAR, bAtEnd ? GetFileSize() : g_fileCurrentPlace);
		CallControlCallback(g_pWindow, C_SEEK_BAR, EVENT_PAINT, 0, 0);
	}
	
	char buffer[32];
	int posSec = g_fileCurrentPlace / ((g_wave_data.nBitsPerSample + 7) / 8) / g_wave_data.nChannels / g_wave_data.nSampleRate;
	sprintf(buffer, "%02d:%02d", posSec / 60, posSec % 60);
	SetLabelText(g_pWindow, C_START_LABEL, buffer);
	CallControlCallback(g_pWindow, C_START_LABEL, EVENT_PAINT, 0, 0);
	
	g_tickLastTicked = GetTickCount();
	
	if (bAtEnd)
		g_bPlaying = false;
}

void WaveFileProcessing()
{
	LogMsg("Loading wave file.");
	
	lseek(g_fileFD, 4, SEEK_END);
	
	uint32_t dataSize = 0, waveMarker = 0;
	read(g_fileFD, &dataSize, sizeof dataSize);
	LogMsg("Data size: %d bytes.", dataSize);
	
	read(g_fileFD, &waveMarker, sizeof waveMarker);
	if (waveMarker != *(uint32_t*)"WAVE") goto fail;
	
	// okay, now read each section until we've reached the 'data' section.
	
	bool bFoundDataSection = false;
	
	while (!bFoundDataSection)
	{
		int where = tellf(g_fileFD);
		uint32_t sectionData[2] = { 0, 0 };
		read(g_fileFD, &sectionData, sizeof sectionData);
		switch (sectionData[0])
		{
			case 0x20746D66: // 'fmt '
			{
				// This is formatting data.
				WaveFormatData fdata;
				
				read(g_fileFD, &fdata, sizeof fdata);
				
				g_wave_data = fdata;
				
				break;
			}
			case 0x61746164: // 'data'
			{
				// Found the data section! We should break.
				bFoundDataSection = true;
				g_offsetFromStart = where + sizeof sectionData;
				g_FileSize = sectionData[1];
				break;
			}
			default:
			{
				char s[5];
				s[4] = 0;
				*((uint32_t*)s) = sectionData[0];
				LogMsg("Unrecognised section name: %s", s);
				break;
			}
		}
		lseek(g_fileFD, where + sectionData[1] + sizeof sectionData, SEEK_SET);
	}
	
	lseek(g_fileFD, g_offsetFromStart, SEEK_SET);
	
	return;
fail:
	lseek(g_fileFD, 0, SEEK_SET);
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
	
	// Check if this is a wave file
	uint32_t firstInt = 0;
	read(g_fileFD, &firstInt, sizeof firstInt);
	
	if (firstInt == *(uint32_t*)"RIFF")
	{
		WaveFileProcessing();
	}
	else
	{
		// Get the size of the raw file
		int size;
		lseek(g_fileFD, 0, SEEK_END);
		size = tellf(g_fileFD);
		size -= g_offsetFromStart;
		g_FileSize = size;
		
		WaveFormatData fd = {
			.nFormat = 1,
			.nChannels = 1,
			.nSampleRate = SAMPLE_RATE,
			.nBytesPerSec = SAMPLE_RATE * 2,
			.nSomething = 0,
			.nBitsPerSample = 16,
		};
		g_wave_data = fd;
	}
	
	g_SampleBufferSize = SAMPLES_PER_TICK * g_wave_data.nChannels * ((g_wave_data.nBitsPerSample + 7) / 8);
	g_SampleBuffer = malloc(g_SampleBufferSize);
	memset(g_SampleBuffer, 0, g_SampleBufferSize);
	
	// Calculate its length in seconds.
	int lenSec = g_FileSize / ((g_wave_data.nBitsPerSample + 7) / 8) / g_wave_data.nChannels / g_wave_data.nSampleRate;
	
	lseek(g_fileFD, g_offsetFromStart, SEEK_SET);
	
	char buffer[32];
	sprintf(buffer, "%02d:%02d", lenSec / 60, lenSec % 60);
	
	SetLabelText(g_pWindow, C_END_LABEL, buffer);
	CallControlCallback(g_pWindow, C_END_LABEL, EVENT_PAINT, 0, 0);
	
	SetScrollBarMax(g_pWindow, C_SEEK_BAR, g_FileSize < 1 ? 1 : g_FileSize);
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
	
	free(g_SampleBuffer);
	g_SampleBuffer = NULL;
	g_SampleBufferSize = 0;
}

void SetPos(int pos)
{
	int filePos = pos;
	int filePosRem = filePos % (g_wave_data.nChannels * (g_wave_data.nBitsPerSample / 8));
	filePos -= filePosRem;
	
	lseek(g_fileFD, g_offsetFromStart + filePos, SEEK_SET);
}

void CALLBACK WndProc (Window* pWindow, int msg, long parm1, long parm2)
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
			
			RECT (r, 12, 40, WAVE_WIDTH - 25, 20);
			
			AddControl(pWindow, CONTROL_HSCROLLBAR, r, NULL, C_SEEK_BAR, (0 << 16) | (1000), 0);
			
			RECT (r, 12, 8, WAVE_WIDTH - 24, 15);
			
			AddControl(pWindow, CONTROL_TEXT, r, "No file loaded", C_FILE_LABEL, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			RECT (r, 12, 24, WAVE_WIDTH / 2, 15);
			
			AddControl(pWindow, CONTROL_TEXT, r, "00:00", C_START_LABEL, WINDOW_TEXT_COLOR, WINDOW_BACKGD_COLOR);
			
			int right = WAVE_WIDTH - 12;
			
			RECT (r, right - WAVE_WIDTH / 2, 24, WAVE_WIDTH / 2, 15);
			
			AddControl(pWindow, CONTROL_TEXTCENTER, r, "00:00", C_END_LABEL, WINDOW_TEXT_COLOR, TEXTSTYLE_RJUSTIFY | TEXTSTYLE_FORCEBGCOL);
			
			int buttonWidth = 68, buttonWidthGap = buttonWidth + 8;
			RECT (r, 12 + 0 * buttonWidthGap, 64, buttonWidth, 30);
			AddControl(pWindow, CONTROL_BUTTON, r, "Play", C_PLAY_BUTTON, 0, 0);
			RECT (r, 12 + 1 * buttonWidthGap, 64, buttonWidth, 30);
			AddControl(pWindow, CONTROL_BUTTON, r, "Stop", C_STOP_BUTTON, 0, 0);
			RECT (r, 12 + 2 * buttonWidthGap, 64, buttonWidth, 30);
			AddControl(pWindow, CONTROL_BUTTON, r, "Open...", C_OPEN_BUTTON, 0, 0);
			RECT (r, 12 + 3 * buttonWidthGap, 64, buttonWidth, 30);
			AddControl(pWindow, CONTROL_BUTTON, r, "Help", C_HELP_BUTTON, 0, 0);
			
			
			if (g_LoadSoundFile)
			{
				LoadFile(g_LoadSoundFile);
			}
			
			if (g_LoadAndPlay)
			{
				Play();
			}
			
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

int main(int argc, char** argv)
{
	g_LoadSoundFile = NULL;
	g_LoadAndPlay   = false;
	
	if (argc >= 2)
	{
		g_LoadSoundFile = argv[1];
	}
	
	for (int i = 2; i < argc; i++)
	{
		if (strcmp(argv[i], "/?") == 0 ||
		    strcmp(argv[i], "--help") == 0)
		{
			LogMsg("Wave Player help");
			LogMsg("Usage: %s [optional file name] [/?] [/p]", argv[0]);
			LogMsg("\t/p\t - Instantly loads and plays the sound file if available.");
			LogMsg("\t/?\t - Shows this help menu.");
		}
		
		if (strcmp(argv[i], "/p") == 0)
		{
			LogMsg("Playing instantly");
			g_LoadAndPlay = true;
		}
	}
	
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
