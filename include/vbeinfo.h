/*****************************************
		NanoShell Operating System
		  (C) 2026 iProgramInCpp

        VBE Information header file
******************************************/
#ifndef _VBEINFO_H
#define _VBEINFO_H

typedef struct
{
	uint8_t  VbeSignature[4];
	uint16_t VbeVersion;
	uint16_t OemStringPtr[2];
	uint32_t Capabilities;
	uint16_t VideoModePtr[2];
	uint16_t TotalMemoryIn64KBBlocks;
	uint8_t  Reserved[492];
}
__attribute__((packed))
VBE_INFO_BLOCK, *PVBE_INFO_BLOCK;

_Static_assert(sizeof(VBE_INFO_BLOCK) == 512, "this should be 512 bytes");

typedef struct
{
	uint16_t Attributes;
	uint8_t  WindowA;
	uint8_t  WindowB;
	uint16_t Granularity;
	uint16_t WindowSize;
	uint16_t SegmentA;
	uint16_t SegmentB;
	uint32_t WindowFunctionPointer;
	uint16_t Pitch;
	uint16_t Width;
	uint16_t Height;
	uint8_t  WChar;
	uint8_t  YChar;
	uint8_t  Planes;
	uint8_t  Bpp;
	uint8_t  Banks;
	uint8_t  MemoryModel;
	uint8_t  BankSize;
	uint8_t  ImagePages;
	uint8_t  Reserved0;
	uint8_t  RedMask;
	uint8_t  RedPosition;
	uint8_t  GreenMask;
	uint8_t  GreenPosition;
	uint8_t  BlueMask;
	uint8_t  BluePosition;
	uint8_t  ReservedMask;
	uint8_t  ReservedPosition;
	uint8_t  DirectColorAttributes;
	uint32_t FrameBufferAddress;
	uint32_t OffScreenMemoryOffset;
	uint16_t OffScreenMemorySize;
	uint8_t  Reserved[206];
}
__attribute__((packed))
VBE_MODE_INFO_BLOCK, *PVBE_MODE_INFO_BLOCK;

_Static_assert(sizeof(VBE_MODE_INFO_BLOCK) == 256, "this should be 256 bytes");

#endif
