/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

     ELF Executable module header file
******************************************/
#ifndef _ELF_H
#define _ELF_H

#include <stdint.h>
#include <main.h>

typedef uint16_t EHalf;
typedef uint32_t EOffs;
typedef uint32_t EAddr;
typedef uint32_t EWord;
typedef  int32_t ESwrd;

typedef int (*ElfEntry)(const char*);

#define ELF_IDENT_SIZE 16
typedef struct {
	uint8_t m_ident[ELF_IDENT_SIZE];
	EHalf m_type;
	EHalf m_machine;
	EWord m_version;
	EAddr m_entry;
	EOffs m_phOffs;
	EOffs m_shOffs;
	EWord m_flags;
	EHalf m_ehSize;
	EHalf m_phEntSize;
	EHalf m_phNum;
	EHalf m_shEntSize;
	EHalf m_shNum;
	EHalf m_shStrNdx;
}
__attribute__((packed))
ElfHeader;

typedef struct {
	EWord m_type;
	EOffs m_offset;
	EAddr m_virtAddr;
	EAddr m_physAddr;//we don't care about this
	EWord m_fileSize;
	EWord m_memSize;
	EWord m_flags;
	EWord m_align;//0x1000
}
__attribute__((packed))
ElfProgHeader;

typedef struct {
	EWord m_name;
	EWord m_type;
	EWord m_flags;
	EAddr m_addr;
	EOffs m_offset;
	EWord m_shSize;
	EWord m_shLink;
	EWord m_shInfo;
	EWord m_shAddrAlign;
	EWord m_shEntSize;
}
__attribute__((packed))
ElfSectHeader;

enum {
	EI_MAG0 = 0, 	//0x7f
	EI_MAG1,		//'E'
	EI_MAG2,		//'L'
	EI_MAG3,		//'F'
	EI_CLASS,		//Arch (32/64). We only support 32 bit
	EI_DATA,		//Byte order.  We only support little-endian.
	EI_VERSION,		//ELF Version.  Usually 0x01
	EI_OSABI,		//OS Specific
	EI_ABIVER,		//OS Specific
	EI_PADDING,		//Padding
};
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELF_DATA_BYTEORDER 	1
#define ELF_CLASS_32BIT		1

enum {
	ELF_TYPE_NONE = 0, 	//Unknown
	ELF_TYPE_RELOC,		//Relocatable file
	ELF_TYPE_EXEC,		//Executable file
};

#define ELF_MACH_386	3 //x86 Machine type
#define ELF_VER_CURRENT 1 //current version. V1

enum {
	ELF_ERROR_NONE = 0x40000000,
	ELF_HEADER_INCORRECT,
	ELF_MACHINE_INCORRECT,
	ELF_ENDIANNESS_INCORRECT,
	ELF_CLASS_INCORRECT,
	ELF_VERSION_INCORRECT,
	ELF_FILETYPE_INCORRECT,
	ELF_RELOCATE_ERROR,
	ELF_CANT_MAKE_HEAP,
	ELF_INVALID_SEGMENTS,
	ELF_FILE_NOT_FOUND,
	ELF_FILE_IO_ERROR,
	ELF_OUT_OF_MEMORY,
	ELF_PROCESS_ERROR,
	ELF_KILLED,
	ELF_ERR_COUNT,
};

enum
{
	SHT_NULL,
	SHT_PROGBITS,
	SHT_SYMTAB,
	SHT_STRTAB,
	SHT_RELA,
	SHT_HASH,
	SHT_DYNAMIC,
	SHT_NOTE,
	SHT_NOBITS,
	SHT_REL,
	SHT_SHLIB,
	SHT_DYNSYM,
	//...
};

int GetDefaultHeapSize();
void SetDefaultHeapSize(int);

void ElfPerformTest();
const char *ElfGetErrorMsg (int error_code);
int ElfRunProgram(const char *pFileName, const char *args, bool bAsync, bool bGui, int nHeapSize, int *pElfErrorCodeOut);

#endif//_ELF_H