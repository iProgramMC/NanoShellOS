/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
         Command-line text editor

             Main source file
******************************************/
#include <nanoshell/nanoshell.h>

Console* GetConsole();

int EditorReadFile(const char * filename);

void EditorRun();

int NsMain (UNUSED int argc, UNUSED char** argv)
{
	if (argc < 2)
	{
		//TODO: Allow getting whether or not a person ran this from the cabinet
		LogMsg("usage: edit [file name]");
		return 0;
	}
	
	// read the file
	int result = EditorReadFile(argv[1]);
	
	if (result < 0)
	{
		LogMsg("%s: %s: %s", argv[0], argv[1], ErrNoStr(result));
		return 0;
	}
	
	EditorRun();
	
	return 0;
}
