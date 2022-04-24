#include <stdint.h> // come on, freestanding has this

typedef void (*PutString_t)(const char *pString);

void PutString (const char *pStr)
{
	PutString_t func = (PutString_t)0xC0007C00;
	
	// write system call number
	*((uint32_t*)0xC0007CFC) = 36; //CON_PUTSTRING
	func (pStr);
}



int _NsStart (__attribute__((unused)) const char *pArg)
{
	PutString ("Hello!");
	return 0;
}