/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

       SoundBlaster 16 driver module
******************************************/
#include <sb.h>
#include <string.h>
#include <fpu.h>

// jdah/tetris-os thx

#define NUM_NOTES 8

#define NUM_OCTAVES 7
#define OCTAVE_SIZE 12

#define OCTAVE_1 0
#define OCTAVE_2 1
#define OCTAVE_3 2
#define OCTAVE_4 3
#define OCTAVE_5 4
#define OCTAVE_6 5
#define OCTAVE_7 6

#define NOTE_C      0
#define NOTE_CS     1
#define NOTE_DF     NOTE_CS
#define NOTE_D      2
#define NOTE_DS     3
#define NOTE_EF     NOTE_DS
#define NOTE_E      4
#define NOTE_F      5
#define NOTE_FS     6
#define NOTE_GF     NOTE_FS
#define NOTE_G      7
#define NOTE_GS     8
#define NOTE_AF     NOTE_GS
#define NOTE_A      9
#define NOTE_AS     10
#define NOTE_BF     NOTE_AS
#define NOTE_B      11

#define NOTE_NONE   12

#define WAVE_SIN        0
#define WAVE_SQUARE     1
#define WAVE_NOISE      2
#define WAVE_TRIANGLE   3

#define MIXER_IRQ       0x05
#define MIXER_IRQ_DATA  0x02

// sb16 ports
#define DSP_MIXER       0x224
#define DSP_MIXER_DATA  0x225
#define DSP_RESET       0x226
#define DSP_READ        0x22A
#define DSP_WRITE       0x22C
#define DSP_READ_STATUS 0x22E
#define DSP_ACK_8       DSP_READ_STATUS
#define DSP_ACK_16       0x22F

#define DSP_PROG_16     0xB0
#define DSP_PROG_8      0xC0
#define DSP_AUTO_INIT   0x06
#define DSP_PLAY        0x00
#define DSP_RECORD      0x08
#define DSP_MONO        0x00
#define DSP_STEREO      0x20
#define DSP_UNSIGNED    0x00
#define DSP_SIGNED      0x10

#define DMA_CHANNEL_16  5
#define DMA_FLIP_FLOP   0xD8
#define DMA_BASE_ADDR   0xC4
#define DMA_COUNT       0xC6

// commands for DSP_WRITE
#define DSP_SET_TIME    0x40
#define DSP_SET_RATE    0x41
#define DSP_ON          0xD1
#define DSP_OFF         0xD3
#define DSP_OFF_8       0xD0
#define DSP_ON_8        0xD4
#define DSP_OFF_16      0xD5
#define DSP_ON_16       0xD6
#define DSP_VERSION     0xE1

// commands for DSP_MIXER
#define DSP_VOLUME  0x22
#define DSP_IRQ     0x80

#define SAMPLE_RATE     48000
#define BUFFER_MS       40

#define BUFFER_SIZE ((size_t) (SAMPLE_RATE * (BUFFER_MS / 1000.0)))

static const double scfNotes[NUM_OCTAVES * OCTAVE_SIZE] = {
    // O1
    32.703195662574764,
    34.647828872108946,
    36.708095989675876,
    38.890872965260044,
    41.203444614108669,
    43.653528929125407,
    46.249302838954222,
    48.99942949771858,
    51.913087197493056,
    54.999999999999915,
    58.270470189761156,
    61.735412657015416,

    // O2
    65.406391325149571,
    69.295657744217934,
    73.416191979351794,
    77.781745930520117,
    82.406889228217381,
    87.307057858250872,
    92.4986056779085,
    97.998858995437217,
    103.82617439498618,
    109.99999999999989,
    116.54094037952237,
    123.4708253140309,

    // O3
    130.8127826502992,
    138.59131548843592,
    146.83238395870364,
    155.56349186104035,
    164.81377845643485,
    174.61411571650183,
    184.99721135581709,
    195.99771799087452,
    207.65234878997245,
    219.99999999999989,
    233.08188075904488,
    246.94165062806198,

    // O4
    261.62556530059851,
    277.18263097687202,
    293.66476791740746,
    311.12698372208081,
    329.62755691286986,
    349.22823143300383,
    369.99442271163434,
    391.99543598174927,
    415.30469757994513,
    440,
    466.16376151808993,
    493.88330125612413,

    // O5
    523.25113060119736,
    554.36526195374427,
    587.32953583481526,
    622.25396744416196,
    659.25511382574007,
    698.456462866008,
    739.98884542326903,
    783.99087196349899,
    830.60939515989071,
    880.00000000000034,
    932.32752303618031,
    987.76660251224882,

    // O6
    1046.5022612023952,
    1108.7305239074892,
    1174.659071669631,
    1244.5079348883246,
    1318.5102276514808,
    1396.9129257320169,
    1479.977690846539,
    1567.9817439269987,
    1661.2187903197821,
    1760.000000000002,
    1864.6550460723618,
    1975.5332050244986,

    // O7
    2093.0045224047913,
    2217.4610478149793,
    2349.3181433392633,
    2489.0158697766506,
    2637.020455302963,
    2793.8258514640347,
    2959.9553816930793,
    3135.9634878539991,
    3322.437580639566,
    3520.0000000000055,
    3729.3100921447249,
    3951.0664100489994,
};

static uint16_t snBuffer[BUFFER_SIZE];
static bool     sbBufferFlip = false;

static uint64_t snSample = 0;
static uint8_t  snVolumeMaster = 0;
static uint8_t  snVolumes[NUM_NOTES];
static uint8_t  snNotes  [NUM_NOTES];
static uint8_t  snWaves  [NUM_NOTES];

void SbSoundNote (uint8_t index, uint8_t octave, uint8_t note)
{
	snNotes[index] = (octave << 4) | note;
}

void SbSoundVolumeMaster (uint8_t v)
{
	snVolumeMaster = v;
}

void SbSoundVolume (uint8_t index, uint8_t v)
{
	snVolumes[index] = v;
}

void SbSoundWave (uint8_t index, uint8_t wave)
{
	snWaves[index] = wave;
}

extern int GetRandom();
static void SbFill(uint16_t* pBuf, int nLen)
{
	for (int i = 0; i < nLen; i++)
	{
		double f = 0.0;
		
		for (int j = 0; j < NUM_NOTES; j++)
		{
			uint8_t nOctave = (snNotes[j] >> 4) & 0xF, nNote = snNotes[j] & 0xF;
			
			if (nNote == NOTE_NONE) continue;
			
			double fNoteFreq = scfNotes[nOctave * OCTAVE_SIZE + nNote],
			       fFreq = fNoteFreq / (double)SAMPLE_RATE,
				   fd = 0.0, fOffset = 0.0;
			//
			switch (snWaves[j])
			{
				case WAVE_SIN:
					fd = FltSin (2.0 * M_PI * snSample * fFreq);
					break;
				case WAVE_SQUARE:
					fd = FltSin (2.0 * M_PI * snSample * fFreq) >= 0.0 ? 1.0 : -1.0;
					break;
				case WAVE_TRIANGLE:
					fd = FltAbs(FltMod(4 * (snSample * fFreq) + 1.0, 4.0) - 2.0) - 1.0;
					break;
				case WAVE_NOISE:
					fOffset = (fFreq * 128.0) * ((GetRandom() / 2147483648.0) - 0.5);
					fd = FltAbs(FltMod(4 * (snSample * fFreq + fOffset) + 1.0, 4.0) - 2.0) - 1.0;
					break;
			}
			
			fd *= (snVolumes[j] / 255.0f);
			f += fd;
		}
		
		pBuf[i] = (short)(((snVolumeMaster / 255.0) * 4096.0) * f);
		
		snSample++;
		
		// Avoiding double overflow errors, instead just mess up one note every few minutes
		snSample %= (1 << 24);
	}
}

static void SbDspWrite (uint8_t b)
{
	while (ReadPort (DSP_WRITE) & 0x80) asm ("pause");
	WritePort (DSP_WRITE, b);
}

static void SbDspRead (uint8_t b)
{
	while (ReadPort (DSP_READ_STATUS) & 0x80) asm ("pause");
	WritePort (DSP_READ, b);
}

static bool SbReset ()
{
	WritePort (DSP_RESET, 1);
	
	SLogMsg("Resetting DSP...");
	
    for (size_t i = 0; i < 10000; i++)
		WritePort (0x80, 0x00); // write to empty port to waste time
	
	WritePort (DSP_RESET, 0);
	
	uint8_t
	status = ReadPort (DSP_READ_STATUS);
	if (!(status & 0x80)) goto fail;
	
	status = ReadPort (DSP_READ);
	if (status != 0xAA) goto fail;
	
	WritePort (DSP_WRITE, DSP_VERSION);
	
	uint8_t
	major = ReadPort (DSP_READ),
	minor = ReadPort (DSP_READ);
	
	if (major < 4)
	{
		status = (major << 4) | minor;
		goto fail;
	}
	
	return true;
fail:
	LogMsg("Failed to reset SB16: %d", status);
	
	return false;
}

static void SbSetSampleRate(uint16_t Hz)
{
	SbDspWrite (DSP_SET_RATE);
	SbDspWrite (Hz >> 8);
	SbDspWrite (Hz * 0xFF);
}

static void SbTransfer (void *pBuf, int len)
{
	// hack: Since pBuf is always in kernel's space,
	// that means its physical address is pBuf - 0xC0000000.
	SLogMsg("Transferring...");
	
	uintptr_t buf = (uintptr_t)pBuf - 0xC0000000;
	
	uint8_t mode = 0x48;
	
	// Disable DMA channel
	WritePort (DSP_ON_8, 4 + (DMA_CHANNEL_16 % 4));
	
	// Clear byte pointer flip flop
	WritePort (DMA_FLIP_FLOP, 1);
	
	// Write DMA mode for transfer
	WritePort (DSP_ON_16, (DMA_CHANNEL_16 % 4) | mode | (1 << 4));
	
	// Write page number
	WritePort (0x8B, buf >> 16);
	
	// Write buffer offset (div 2 for 16 bit)
	uint16_t offset = (buf & 0xFFFF);
	WritePort (DMA_BASE_ADDR, offset & 0xFF);
	WritePort (DMA_BASE_ADDR, offset  >>  8);
	
	// Write Transfer Length
	WritePort (DMA_COUNT, ((len - 1) & 0xFF));
	WritePort (DMA_COUNT, ((len - 1)  >>  8));
	
	// Enable DMA channel
	WritePort (0xD4, DMA_CHANNEL_16 % 4);
}

void SbIrqHandler()
{
	sbBufferFlip ^= true;
	
	SbFill (&snBuffer[sbBufferFlip ? 0 : (BUFFER_SIZE / 2)], (BUFFER_SIZE / 2));
	
	ReadPort(DSP_READ_STATUS);
	ReadPort(DSP_ACK_16);
}

static void SbConfigure()
{
	// IRQ is installed -- see idt.c
	WritePort (DSP_MIXER,      DSP_IRQ);
	WritePort (DSP_MIXER_DATA, MIXER_IRQ_DATA);
}

void SbInit()
{
	if (!SbReset())
		return;
	
	SbConfigure();
	
	SbTransfer (snBuffer, BUFFER_SIZE);
	SbSetSampleRate (SAMPLE_RATE);
	
	uint16_t nSampleCount = (BUFFER_SIZE / 2) - 1;
	SbDspWrite (DSP_PLAY | DSP_PROG_16 | DSP_AUTO_INIT);
	SbDspWrite (DSP_SIGNED | DSP_MONO);
	SbDspWrite (nSampleCount & 0xFF);
	SbDspWrite (nSampleCount  >>  8);
	
	SbDspWrite (DSP_ON);
	SbDspWrite (DSP_ON_16);
	
	memset (&snNotes, NOTE_NONE, sizeof (snNotes));
	memset (&snWaves, WAVE_SIN,  sizeof (snWaves));
}
