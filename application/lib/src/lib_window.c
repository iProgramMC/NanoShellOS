#include "../include/window.h"
#include "lib_header.h"

// Defines the window's base
#define WINDOW_CALL_BASE 0xC0007C00

// Type Definitions.
#define CALL(funcName, funcIndex, retType, ...)\
	typedef retType(* P_ ## funcName) (__VA_ARGS__);
#define RARGS(...)
#define SARGS(...)
#define CALL_END

#include "wincalls.h"

#undef CALL
#undef RARGS
#undef SARGS
#undef CALL_END

// Wrapper function definitions.
#define CALL(funcName, funcIndex, retType, ...)\
	retType funcName (__VA_ARGS__)\
	{\
		*((uint32_t*)(WINDOW_CALL_BASE + 0xFC)) = funcIndex;\
		P_ ## funcName p_func = (P_ ## funcName)WINDOW_CALL_BASE;\
		
#define RARGS(...)\
		return p_func (__VA_ARGS__);\
	}	
#define SARGS(...)\
		p_func (__VA_ARGS__);\
	}
#define CALL_END //empty, use if needed

#include "wincalls.h"

#undef CALL
#undef RARGS
#undef SARGS
#undef CALL_END

// Other helpers
void VidFillRectangle(unsigned color, Rectangle rect)
{
	VidFillRect(color, rect.left, rect.top, rect.right, rect.bottom);
}
void VidDrawRectangle(unsigned color, Rectangle rect)
{
	VidDrawRect(color, rect.left, rect.top, rect.right, rect.bottom);
}

// End of File
