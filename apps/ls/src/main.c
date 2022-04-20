#include <nsstandard.h>

int NsMain (UNUSED int argc, UNUSED char** argv)
{
	//LogMsg("%s", _I_FiGetCwd());
	
	LogMsg("Directory of %s", FiGetCwd());
	
	return 0;
}
