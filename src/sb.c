/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

      Sound Blaster 16 driver module
******************************************/
#include <main.h>
#include <string.h>
#include <print.h>
#include <misc.h>
#include <memory.h>
#include <fpu.h>
#include <vfs.h>

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
#define BUFFER_MS       100

#define BUFFER_SIZE ((size_t) (SAMPLE_RATE * BUFFER_MS / 1000))

static i16 gSoundBuffer[BUFFER_SIZE];
static bool gBufferFlip = false;

static UNUSED u8 gMasterVolume;

// Buffered data that the file write function writes to.
static u8  gSoundQueuedData[BUFFER_SIZE * 2];
static int gSoundQueueTail, gSoundQueueHead;

bool SbWriteSingleByte (uint8_t data, bool bBlock)
{
	// About to overflow?
	while (((gSoundQueueHead + 1) % sizeof(gSoundQueuedData)) == gSoundQueueTail)
	{
		// Wait.  We can wait here, since we're not handling an IRQ.
		if (!bBlock) return false;
		KeTaskDone();
	}
	
	gSoundQueuedData[gSoundQueueHead++] = data;
	
	gSoundQueueHead = gSoundQueueHead % sizeof(gSoundQueuedData);
	return true;
}

SAI bool SbReadSingleByte(uint8_t* out)
{
	if (gSoundQueueTail == gSoundQueueHead)
	{
		*out = 0;
		return false;
	}
	
	*out = gSoundQueuedData[gSoundQueueTail++];
	
	gSoundQueueTail = gSoundQueueTail % sizeof(gSoundQueuedData);
	
	return true;
}

void* SbTestGenerateSound(size_t* size)
{
	// construct some size
	size_t sz = 88200; // this many bytes to fill 1 second of audio
	
	uint16_t* data = MmAllocate(sz);
	
	//generate a 689.0625 Hz audio output.
	for (size_t i = 0; i < sz/2; i++)
	{
		if (i%128 < 64)
		{
			data[i] = 10000;
		}
		else
		{
			data[i] = 0;
		}
	}
	
	*size = sz;
	return data;
}

int SbWriteData (const void *pData, size_t sizeBytes, bool bBlock)
{
	const uint8_t* pBuffer = (const uint8_t*)pData;
	// Is the tail ahead of the head?
	if (gSoundQueueTail > gSoundQueueHead)
	{
		// Yes. Check for overflow up until the tail.
		if (gSoundQueueHead + (int)sizeBytes < gSoundQueueTail)
		{
			// No overflow. Great, now we can just memcpy the data.
			memcpy(&gSoundQueuedData[gSoundQueueHead], pBuffer, sizeBytes);
			gSoundQueueHead += sizeBytes;
			
			return (int) sizeBytes;
		}
		// No. Write each byte one at a time, if we're about to overflow,
		// wait until the tail advances.
		for (size_t i = 0; i < sizeBytes; i++)
		{
			//TODO We could optimize this by memcpying up until the tail
			if (!SbWriteSingleByte(pBuffer[i], bBlock))
				return (int) i;
		}
		
		return (int) sizeBytes;
	}
	else
	{
		// No. Check if we're about to overflow the whole array
		if (gSoundQueueHead + sizeBytes < sizeof(gSoundQueuedData))
		{
			// No overflow. Great, now we can just memcpy the data.
			memcpy(&gSoundQueuedData[gSoundQueueHead], pBuffer, sizeBytes);
			gSoundQueueHead += sizeBytes;
			
			return (int) sizeBytes;
		}
		// No. Write each byte one at a time, if we're about to overflow,
		// wait until the tail advances.
		for (size_t i = 0; i < sizeBytes; i++)
		{
			//TODO We could optimize this by memcpying up until the tail
			if (!SbWriteSingleByte(pBuffer[i], bBlock))
				return (int) i;
		}
		
		return (int) sizeBytes;
	}
}

static void SbFillBuffer(i16 *buf, size_t len)
{
	//uint64_t a = ReadTSC();
	for (int i = 0; i < (int)len; i++)
	{
		uint8_t lo = 0, hi = 0;
		
		SbReadSingleByte (&lo),
		SbReadSingleByte (&hi);
		
		buf[i] = lo | hi << 8;
	}
	//uint64_t b = ReadTSC();
	//SLogMsg("b-a: %l",b-a);
}

static void DspWrite(u8 b)
{
    while (ReadPort(DSP_WRITE) & 0x80);
    WritePort(DSP_WRITE, b);
}

static bool SbReset()
{
    WritePort(DSP_RESET, 1);

    // TODO: maybe not necessary
    // ~3 microseconds?
    for (volatile size_t i = 0; i < 1000000; i++);

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
	ILogMsg("Failed to reset SB16: %d", status);
	return false;
}

static void SbSetSampleRate(u16 hz)
{
    DspWrite(DSP_SET_RATE);
    DspWrite((u8) ((hz >> 8) & 0xFF));
    DspWrite((u8) (hz & 0xFF));
}

//note: this takes in a physical address
static void SbSetupDma(uintptr_t buf1, u32 len)
{
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

void SbIrqHandler()
{
    gBufferFlip = !gBufferFlip;

    SbFillBuffer(
        &gSoundBuffer[gBufferFlip ? 0 : (BUFFER_SIZE / 2)],
        (BUFFER_SIZE / 2)
    );

	WritePort (0x20, 0x20);
	WritePort (0xA0, 0x20);
	
    ReadPort(DSP_READ_STATUS);
    ReadPort(DSP_ACK_16);
}

static void SbSetupIrq() {
	
    WritePort(DSP_MIXER, DSP_IRQ);
    WritePort(DSP_MIXER_DATA, MIXER_IRQ_DATA);

    u8 v = MIXER_IRQ;
    if (v != MIXER_IRQ) {
		ILogMsg("SB16 has incorrect IRQ: %d", v);
		return;
    }
}

void SbSetUpFile();

void SbInit()
{
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
	
	SbSetUpFile();
}

// File system interface
static int FsSoundBlasterRead (UNUSED FileNode *pNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED void *pBuffer, UNUSED bool bBlock)
{
	return 0;//This is a write only file
}

static int FsSoundBlasterWrite(UNUSED FileNode *pNode, UNUSED uint32_t offset, UNUSED uint32_t size, UNUSED const void *pBuffer, UNUSED bool bBlock)
{
	//Offset will be ignored.
	return SbWriteData (pBuffer, size, bBlock);
}

int FsSoundBlasterIoControl(UNUSED FileNode* pNode, unsigned long request, void * argp)
{
	if (request != IOCTL_SOUNDDEV_SET_SAMPLE_RATE)  return -ENOTTY;
	
	// Set the sample rate as passed in to argp.
	int * argp_int = (int *) argp;
	
	SLogMsg("Set sample rate to %d", *argp_int);
	SbSetSampleRate(*argp_int);
	
	return -ENOTHING;
}

void FsUtilAddArbitraryFileNode(const char* pDirPath, const char* pFileName, FileNode* pSrcNode);

void SbSetUpFile()
{
	FileNode node, *pNode = &node;
	memset(pNode, 0, sizeof node);
	
	ASSERT(pNode && "Couldn't add that file to the root?");
	
	pNode->m_refCount = NODE_IS_PERMANENT;
	pNode->m_inode    = 0;
	
	pNode->m_perms   = PERM_READ | PERM_WRITE;
	pNode->m_type    = FILE_TYPE_CHAR_DEVICE;
	pNode->m_inode   = 0;
	pNode->m_length  = 0;
	pNode->m_bHasDirCallbacks = false;
	pNode->Read      = FsSoundBlasterRead;
	pNode->Write     = FsSoundBlasterWrite;
	pNode->IoControl = FsSoundBlasterIoControl;
	
	FsUtilAddArbitraryFileNode("/Device", "Sb16", pNode);
}
