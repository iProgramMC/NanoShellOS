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

/**
 * A simple function to hash a string. Not cryptographically secure at all, but it should be fine.
 */
uint32_t HashString(const char *const str);

/**
 * Initialize the configuration manager.
 */
void CfgInit(void);

/**
 * Debug: Print the entries of the configuration.
 */
void CfgPrintEntries(void);

/**
 * Loads configuration data from a piece of text.
 *
 * This operation destroys the text that's being worked on, for a function that does not, see `CfgLoadFromText()`.
 */
void CfgLoadFromTextBasic(char* work);

/**
 * Loads configuration data from a file.
 */
void CfgLoadFromFile (const char * file);

/**
 * Loads configuration data from a piece of text.
 */
void CfgLoadFromText (const char* parms);

/**
 * Loads configuration data from a one line list of parameters. Useful to specify command line parameters.
 */
void CfgLoadFromParms(const char* parms);

//TODO: maybe duplicate these actually
//DON'T hold pointers to these, instead copy their values.
//If new entries are added the kernel may want to expand
//the array of config entries

/**
 * Adds a configuration entry to the global config manager.
 */
ConfigEntry* CfgAddEntry(ConfigEntry *pEntry);

/**
 * Gets the ConfigEntry pointer for a key, or NULL if it doesn't exist.
 *
 * Same TODO as CfgGetEntryValue.
 */
ConfigEntry* CfgGetEntry(const char* key);


/**
 * Gets the value of a configuration entry, or NULL if it doesn't exist.
 *
 * TODO: duplicate the contents for save modification. A thread could slip in a
 * modification while the application uses the return value of this function.
 */
const char *CfgGetEntryValue(const char *pKey);

/**
 * Check if a configuration entry has the value specified.
 */
bool CfgEntryMatches(const char *pKey, const char *pValueCmp);

/**
 * Gets an integer value from a config key, or 'default' if said key doesn't actually exist.
 */
void CfgGetIntValue(int* out, const char* key, int _default);

#endif//_CONFIG_H