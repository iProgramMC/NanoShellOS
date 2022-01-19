#include "lib_header.h"

void PutString(const char* pText)
{
	DoSyscall(LOGMSG, (int)pText, 0, 0, 0);
}
void *Allocate(int size)
{
	return (void*)DoSyscall(MALLOC, size, 0, 0, 0);
}
void Free(void* ptr)
{
	DoSyscall(FREE, (int)ptr, 0, 0, 0);
}
void MmDebugDump()
{
	DoSyscall(DUMPMEM, 0, 0, 0, 0);
}
