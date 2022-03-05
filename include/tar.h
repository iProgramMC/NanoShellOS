/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

      Tar file structure header file
******************************************/
#ifndef _TAR_H
#define _TAR_H

typedef struct 
{
	union
	{
		struct
		{
			char name[100];
			char mode[8];
			char uid[8];
			char gid[8];
			char size[12];
			char mtime[12];
			char check[8];
			char type;
			char link_name[100];
			char ustar[8];
			char owner[32];
			char group[32];
			char major[8];
			char minor[8];
			char prefix[155];
		};
		
		char block[512];
	};
	
	char buffer[];
}
Tar;

#endif //_TAR_H