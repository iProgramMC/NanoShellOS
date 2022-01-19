DEFINE_DEVICE ("Null", FsNullRead, FsNullWrite, 0);
DEFINE_DEVICE ("Random", FsRandomRead, FsRandomWrite, 0);
DEFINE_DEVICE ("ConVid", FsTeletypeRead, FsTeletypeWrite, DEVICE_DEBUG_CONSOLE);
DEFINE_DEVICE ("ConDbg", FsTeletypeRead, FsTeletypeWrite, DEVICE_DEBUG_E9_CONSOLE);