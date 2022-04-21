#include <main.h>
#include <string.h>
#include <print.h>
#include <misc.h>
#include <fpu.h>

#define NUM_NOTES 1

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

#define E 2.71828
#define PI 3.14159265358979323846264338327950

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef u32 size_t;
typedef u32 uintptr_t;
typedef float f32;
typedef double f64;

#define MIXER_IRQ       0x5
#define MIXER_IRQ_DATA  0x2

// SB16 ports
#define DSP_MIXER       0x224
#define DSP_MIXER_DATA  0x225
#define DSP_RESET       0x226
#define DSP_READ        0x22A
#define DSP_WRITE       0x22C
#define DSP_READ_STATUS 0x22E
#define DSP_ACK_8       DSP_READ_STATUS
#define DSP_ACK_16      0x22F

// TODO: ???
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

//#define SAMPLE_RATE     48000
#define SAMPLE_RATE     44100
#define BUFFER_MS       40 //increased latency, but fewer interrupts

#define BUFFER_SIZE ((size_t) (SAMPLE_RATE * (BUFFER_MS / 1000.0)))

static i16 gSoundBuffer[BUFFER_SIZE];
static bool gBufferFlip = false;

static UNUSED u8 gMasterVolume;

static void SbFillBuffer(i16 *buf, size_t len) {
	
	// TODO: Read from a file called /Device/Sb16
	memset_shorts (buf, 0, len);
	
}

static void DspWrite(u8 b) {
    while (ReadPort(DSP_WRITE) & 0x80);
    WritePort(DSP_WRITE, b);
}

static bool SbReset() {
	
    WritePort(DSP_RESET, 1);

    // TODO: maybe not necessary
    // ~3 microseconds?
    for (size_t i = 0; i < 1000000; i++);

    WritePort(DSP_RESET, 0);

    u8 status = ReadPort(DSP_READ_STATUS);
    if (~status & 128) {
        goto fail;
    }

    status = ReadPort(DSP_READ);
    if (status != 0xAA) {
        goto fail;
    }

    WritePort(DSP_WRITE, DSP_VERSION);
    u8 major = ReadPort(DSP_READ),
       minor = ReadPort(DSP_READ);

    if (major < 4) {
        status = (major << 4) | minor;
        goto fail;
    }

    return true;
fail:
	LogMsg("Failed to reset SB16: %d", status);
	return false;
}

static void SbSetSampleRate(u16 hz) {
    DspWrite(DSP_SET_RATE);
    DspWrite((u8) ((hz >> 8) & 0xFF));
    DspWrite((u8) (hz & 0xFF));
}

//note: this takes in a physical address
static void SbSetupDma(uintptr_t buf1, u32 len) {
	
    u8 mode = 0x48;

    // disable DMA channel
    WritePort(DSP_ON_8, 4 + (DMA_CHANNEL_16 % 4));

    // clear byte-poiner flip-flop
    WritePort(DMA_FLIP_FLOP, 1);

    // write DMA mode for transfer
    WritePort(DSP_ON_16, (DMA_CHANNEL_16 % 4) | mode | (1 << 4));

    // write buffer offset (div 2 for 16-bit)
    u16 offset = (((uintptr_t) buf1) / 2) % 65536;
    WritePort(DMA_BASE_ADDR, (u8) ((offset >> 0) & 0xFF));
    WritePort(DMA_BASE_ADDR, (u8) ((offset >> 8) & 0xFF));

    // write transfer length
    WritePort(DMA_COUNT, (u8) (((len - 1) >> 0) & 0xFF));
    WritePort(DMA_COUNT, (u8) (((len - 1) >> 8) & 0xFF));

    // write buffer
    WritePort(0x8B, ((uintptr_t) buf1) >> 16);

    // enable DMA channel
    WritePort(0xD4, DMA_CHANNEL_16 % 4);
}

void SbIrqHandler() {
	
    gBufferFlip = !gBufferFlip;

    SbFillBuffer(
        &gSoundBuffer[gBufferFlip ? 0 : (BUFFER_SIZE / 2)],
        (BUFFER_SIZE / 2)
    );

    ReadPort(DSP_READ_STATUS);
    ReadPort(DSP_ACK_16);
}

static void SbSetupIrq() {
	
    WritePort(DSP_MIXER, DSP_IRQ);
    WritePort(DSP_MIXER_DATA, MIXER_IRQ_DATA);

    u8 v = MIXER_IRQ;
    if (v != MIXER_IRQ) {
		LogMsg("SB16 has incorrect IRQ: %d", v);
		return;
    }
}

void SbInit() {
    //irq_install(MIXER_IRQ, sb16_irq_handler);
    if (!SbReset()) return;
    SbSetupIrq();

    SbSetupDma((uintptr_t)gSoundBuffer - 0xc0000000, BUFFER_SIZE);
    SbSetSampleRate(SAMPLE_RATE);

    u16 sampleCount = (BUFFER_SIZE / 2) - 1;
    DspWrite(DSP_PLAY | DSP_PROG_16 | DSP_AUTO_INIT);
    DspWrite(DSP_SIGNED | DSP_MONO);
    DspWrite((u8) ((sampleCount >> 0) & 0xFF));
    DspWrite((u8) ((sampleCount >> 8) & 0xFF));

    DspWrite(DSP_ON);
    DspWrite(DSP_ON_16);
}

