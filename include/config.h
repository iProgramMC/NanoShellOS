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

void CfgInit(void);
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

const char *CfgGetEntryValue(const char *pKey);
bool CfgEntryMatches(const char *pKey, const char *pValueCmp);

// Gets an integer value from a config key, or 'default' if said key doesn't actually exist.
void CfgGetIntValue(int* out, const char* key, int _default);

#endif//_CONFIG_H