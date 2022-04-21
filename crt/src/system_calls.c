#include "crtlib.h"

#define WINDOW_CALL_BASE 0xc0007c00

// Type Definitions.
#define CALL(funcName, funcIndex, retType, ...)\
	typedef retType(* P_I_ ## funcName) (__VA_ARGS__);
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
	retType _I_ ## funcName (__VA_ARGS__)\
	{\
		*((uint32_t*)(WINDOW_CALL_BASE + 0xFC)) = funcIndex;\
		P_I_ ## funcName p_func = (P_I_ ## funcName)WINDOW_CALL_BASE;\
		
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


