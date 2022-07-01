/*******************************************
		 NanoShell Operating System
		   (C) 2022 iProgramInCpp

 Virtual FileSystem device file definitions
********************************************/

DEFINE_DEVICE ("Null",   FsNullRead,         FsNullWrite,         0);
DEFINE_DEVICE ("Random", FsRandomRead,       FsRandomWrite,       0);
DEFINE_DEVICE ("ConVid", FsTeletypeRead,     FsTeletypeWrite,     DEVICE_DEBUG_CONSOLE);
DEFINE_DEVICE ("ConDbg", FsTeletypeRead,     FsTeletypeWrite,     DEVICE_DEBUG_E9_CONSOLE);
DEFINE_DEVICE ("Sb16",   FsSoundBlasterRead, FsSoundBlasterWrite, 0);