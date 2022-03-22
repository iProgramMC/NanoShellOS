/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

  Kernel Configuration module header file
******************************************/
#ifndef _CONFIG_H
#define _CONFIG_H

#include <main.h>
#include <memory.h>
#include <string.h>

typedef struct ConfigEntry
{
    uint32_t entry_hash;
    char     entry[64];
    char     value[250];
}
ConfigEntry;

// This isn't a cryptographically secure hash.
// I mean I couldn't care less about that, since
// my OS is the definition of insecure
uint32_t HashString(const char *const str);

void CfgInitialize(void);
void CfgPrintEntries(void);
void CfgLoadFromTextBasic(char* work);
void CfgLoadFromFile (const char * file);//TODO
void CfgLoadFromText (const char* parms);
void CfgLoadFromParms(const char* parms);
//DON'T hold pointers to these, instead copy their values.
//If new entries are added the kernel may want to expand
//the array of config entries
ConfigEntry* CfgAddEntry(ConfigEntry *pEntry);
ConfigEntry* CfgGetEntry(const char* key);

#endif//_CONFIG_H