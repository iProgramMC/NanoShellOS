//  ***************************************************************
//  rt.h - Creation date: 20/02/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022-2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#ifndef _RT_H
#define _RT_H

enum // eResourceType
{
	RES_NONE,
	RES_STRING,
	RES_ICON,
	RES_BITMAP,
	RES_BLOB,
};

enum // eSubsystem
{
	SUBSYSTEM_CONSOLE,
	SUBSYSTEM_WINDOWED,
};

typedef int eResourceType;
typedef int eSubsystem;

typedef struct
{
	// The type of resource.
	eResourceType m_type;
	
	// The resource's ID.
	int m_id;
	
	// The pointer to data.
	union
	{
		void       *m_pResource;
		Image      *m_pIcon;
		Image      *m_pImage;
		const char *m_pString;
	};
}
Resource;

// The resource table is an array of resources sorted in ascending order by their ID.
typedef struct
{
	Resource* m_pResources;
	int m_nResources;
}
ResourceTable;

// This represents the contents of the .nanoshell section.
typedef struct
{
	eSubsystem m_subsystem;
	union
	{
		uint32_t Data;
		struct
		{
			uint16_t BuildNum;
			uint8_t Minor;
			uint8_t Major;
		}
		__attribute__((packed));
	}
	m_Version;
	
	const char *m_AppName;     // Program name
	const char *m_AppAuthor;   // Author of the program
	const char *m_AppCopyright;// Copyright information
	const char *m_ProjName;    // Project name (should this program be a part of a bigger project)
}
ProgramInfoBlock;

typedef struct
{
	// These three shorts are defined by elf.h
	uint16_t m_machine;    // Machine type. A value of 3 means 'i386'
	uint8_t  m_word_size;  // Word size. 32 (1) or 64 bit (2)
	uint8_t  m_byte_order; // Byte order.
	uint8_t  m_os_abi;     // OS ABI. We're using System V ABI here.
	ProgramInfoBlock m_info;
}
ProgramInfo;

void RstFreeResource(Resource* pResource);
ProgramInfo* RstRetrieveProgramInfoFromFile(const char* pFileName);

#endif//_RT_H