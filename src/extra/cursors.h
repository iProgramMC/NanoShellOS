/*************************************************************
                  NanoShell Operating System
		         (C) 2021, 2022 iProgramInCpp

                   Cursor definition module
*************************************************************/

#define X 0X00FFFFFF,
//#define S 0X007F7F7F,
#define S SEMI_TRANSPARENT,
#define B 0XFF000000,
#define o TRANSPARENT,

uint32_t ImageDefaultCursor[] = 
{
	B o o o o o o o o o o o
	B B o o o o o o o o o o
	B X B o o o o o o o o o
	B X X B o o o o o o o o
	B X X X B o o o o o o o
	B X X X X B o o o o o o
	B X X X X X B o o o o o
	B X X X X X X B o o o o
	B X X X X X X X B o o o
	B X X X X X X X X B o o
	B X X X X X B B B B B o
	B X X B X X B S S S S S
	B X B S B X X B o o o o
	B B S S B X X B S o o o
	B S S o o B X X B o o o
	o S o o o B X X B S o o
	o o o o o o B B S S o o
	o o o o o o o S S o o o
};

Cursor g_defaultCursor = {
	12, 18, 0, 0, 
	ImageDefaultCursor,
	true,
	false, 12, 18, -1, -1
};

uint32_t ImageWaitingCursor[] = 
{
	B B B B B B B B B B B B B B B
	B B B B B B B B B B B B B B B
	B B X X X X X X X X X X X B B
	o B B B B B B B B B B B B B o
	o B B X X X X X X X X X B B o
	o B B X X X X X X X X X B B o
	o B B X X X X X X B X X B B o
	o B B X B X B X B X B X B B o
	o B B X X B X B X B X X B B o
	o o B B X X B X B X X B B o o
	o o o B B X X B X X B B o o o
	o o o o B B X B X B B o o o o
	o o o o o B B X B B o o o o o
	o o o o o B B X B B o o o o o
	o o o o o B B X B B o o o o o
	o o o o B B X X X B B o o o o
	o o o B B X X X X X B B o o o
	o o B B X X X B X X X B B o o
	o B B X X X X X X X X X B B o
	o B B X X X X B X X X X B B o
	o B B X X X B X B X X X B B o
	o B B X X B X B X B X X B B o
	o B B X B X B X B X B X B B o
	o B B B B B B B B B B B B B o
	B B X X X X X X X X X X X B B
	B B B B B B B B B B B B B B B
	B B B B B B B B B B B B B B B
};

Cursor g_waitCursor = {
	15, 27, 7, 14, 
	ImageWaitingCursor,
	true,
	false, 15, 27, -1, -1
};

uint32_t ImageCrossCursor[] = {
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	X X X X X X X X X X o X X X X X X X X X X
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
	o o o o o o o o o o X o o o o o o o o o o 
};

Cursor g_crossCursor = {
	21, 21, 10, 10, 
	ImageCrossCursor,
	true,
	false, 21, 21, -1, -1
};

uint32_t ImageIBeamCursor[] = {
	X X X o X X X
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	o o o X o o o
	X X X o X X X
};

Cursor g_iBeamCursor = {
	7, 16, 10, 10, 
	ImageIBeamCursor,
	true,
	false, 7, 16, -1, -1
};


/*****************************************
        NanoShell Operating System
          (c) 2022 iProgramInCpp

  This is a converted icon to embed into
            the kernel image.
 * Converted File: Sprite-0001.png
 * Converted Date: 19/03/2022
 * Icon Last Mod:  19/03/2022
*****************************************/
uint32_t ImagePencilCursor[] = {
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00000000,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FF0000,0x00800000,0x00000000,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00000000,0x00800000,0x00000000,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0xFFFFFFFF,0x00000000,0x00FFFFFF,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0x00000000,0x00FFFF00,0x00FFFF00,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0x00000000,0x00000000,0x00808000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
	0x00000000,0x00000000,0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
};

Cursor g_pencilCursor = {
	21, 21, 0, 21, 
	ImagePencilCursor,
	true,
	false, 21, 21, -1, -1
};

uint32_t ImageResizeWECursor[] = {
	o o o o o o o o o o o o o o o o
	o o o o o o o o o o o o o o o o
	o o o o o o o o o o o o o o o o
	o o o B B B o o o o B B B o o o
	o o B B X B o o o o B X B B o o
	o B B X X B o o o o B X X B B o
	B B X X X B B B B B B X X X B B
	B X X X X X X X X X X X X X X B
	B X X X X X X X X X X X X X X B
	B B X X X B B B B B B X X X B B
	o B B X X B o o o o B X X B B o
	o o B B X B o o o o B X B B o o
	o o o B B B o o o o B B B o o o
	o o o o o o o o o o o o o o o o
	o o o o o o o o o o o o o o o o
	o o o o o o o o o o o o o o o o
};

uint32_t ImageResizeNSCursor[] = {
	o o o o o o B B B B o o o o o o
	o o o o o B B X X B B o o o o o
	o o o o B B X X X X B B o o o o
	o o o B B X X X X X X B B o o o
	o o o B X X X X X X X X B o o o
	o o o B B B B X X B B B B o o o
	o o o o o o B X X B o o o o o o
	o o o o o o B X X B o o o o o o
	o o o o o o B X X B o o o o o o
	o o o o o o B X X B o o o o o o
	o o o B B B B X X B B B B o o o
	o o o B X X X X X X X X B o o o
	o o o B B X X X X X X B B o o o
	o o o o B B X X X X B B o o o o
	o o o o o B B X X B B o o o o o
	o o o o o o B B B B o o o o o o
};

uint32_t ImageResizeNWSECursor[] = {
	o o o o o o o o o o o o o o o o
	o o o o o o o o o o o o o o o o
	o o B B B B B B B B o o o o o o
	o o B X X X X X X B o o o o o o
	o o B X X X X X B B o o o o o o
	o o B X X X X B B o o o o o o o
	o o B X X X X X B B o B B B o o
	o o B X X B X X X B B B X B o o
	o o B X B B B X X X B X X B o o
	o o B B B o B B X X X X X B o o
	o o o o o o o B B X X X X B o o
	o o o o o o B B X X X X X B o o
	o o o o o o B X X X X X X B o o
	o o o o o o B B B B B B B B o o
	o o o o o o o o o o o o o o o o
	o o o o o o o o o o o o o o o o
};

uint32_t ImageResizeNESWCursor[] = {
	o o o o o o o o o o o o o o o o
	o o o o o o o o o o o o o o o o
	o o o o o o B B B B B B B B o o
	o o o o o o B X X X X X X B o o
	o o o o o o B B X X X X X B o o
	o o o o o o o B B X X X X B o o
	o o B B B o B B X X X X X B o o
	o o B X B B B X X X B X X B o o
	o o B X X B X X X B B B X B o o
	o o B X X X X X B B o B B B o o
	o o B X X X X B B o o o o o o o
	o o B X X X X X B B o o o o o o
	o o B X X X X X X B o o o o o o
	o o B B B B B B B B o o o o o o
	o o o o o o o o o o o o o o o o
	o o o o o o o o o o o o o o o o
};

uint32_t ImageResizeAllCursor[] = {
	o o o o o o o o o B B B B o o o o o o o o o
	o o o o o o o o B B X X B B o o o o o o o o
	o o o o o o o B B X X X X B B o o o o o o o
	o o o o o o B B X X X X X X B B o o o o o o
	o o o o o o B X X X X X X X X B o o o o o o
	o o o o o o B B B B X X B B B B o o o o o o
	o o o B B B o o o B X X B o o o B B B o o o
	o o B B X B o o o B X X B o o o B X B B o o
	o B B X X B o o o B X X B o o o B X X B B o
	B B X X X B B B B B X X B B B B B X X X B B
	B X X X X X X X X X X X X X X X X X X X X B
	B X X X X X X X X X X X X X X X X X X X X B
	B B X X X B B B B B X X B B B B B X X X B B
	o B B X X B o o o B X X B o o o B X X B B o
	o o B B X B o o o B X X B o o o B X B B o o
	o o o B B B o o o B X X B o o o B B B o o o
	o o o o o o B B B B X X B B B B o o o o o o
	o o o o o o B X X X X X X X X B o o o o o o
	o o o o o o B B X X X X X X B B o o o o o o
	o o o o o o o B B X X X X B B o o o o o o o
	o o o o o o o o B B X X B B o o o o o o o o
	o o o o o o o o o B B B B o o o o o o o o o
};

Cursor g_resizeNSCursor = {
	16, 16, 8, 8, 
	ImageResizeNSCursor,
	true,
	false, 16, 16, -1, -1
};

Cursor g_resizeWECursor = {
	16, 16, 8, 8, 
	ImageResizeWECursor,
	true,
	false, 16, 16, -1, -1
};

Cursor g_resizeNESWCursor = {
	16, 16, 8, 8, 
	ImageResizeNESWCursor,
	true,
	false, 16, 16, -1, -1
};

Cursor g_resizeNWSECursor = {
	16, 16, 8, 8, 
	ImageResizeNWSECursor,
	true,
	false, 16, 16, -1, -1
};

Cursor g_resizeAllCursor = {
	22, 22, 10, 10, 
	ImageResizeAllCursor,
	true,
	false, 22, 22, -1, -1
};

