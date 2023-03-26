/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Clipboard Utilitary Module
******************************************/

#ifndef _CLIP_H
#define _CLIP_H

#include <main.h>
#include <memory.h>
#include <task.h>
#include <string.h>

enum
{
	CLIPBOARD_DATA_NONE,
	CLIPBOARD_DATA_INTEGER,
	CLIPBOARD_DATA_BINARY,
	CLIPBOARD_DATA_TEXT,
	CLIPBOARD_DATA_LARGE_TEXT,
	
	// Add more clipboard data types here.  Unknown clipboard data types will be treated as generic binaries
};

typedef struct
{
	SafeLock m_lock;
	
	int  m_type;
	char m_short_str[256];
	int  m_blob_size;
	
	union {
		int   m_int_data;
		void *m_generic_data_ptr;
		char *m_char_str;
	};
}
ClipboardVariant;

/**
 * Gets the contents of the clipboard. Use CbRelease to release them.
 */
ClipboardVariant* CbGetCurrentVariant();

/**
 * Releases the contents of the clipboard, so that other programs can use it.
 */
void CbRelease(ClipboardVariant*);

/**
 * This is a function for testing. It types a small block of text from the clipboard (CLIPBOARD_DATA_TEXT)
 * as if it were a sequence of keyboard key presses.
 */
bool CbPushTextIntoBuffer();

/**
 * Clears the clipboard.
 */
void CbClear();

/**
 * Copies a block of text to the clipboard. If the function succeeds, it returns true.
 */
bool CbCopyText(const char* pText);

/**
 * Copies a binary blob to the clipboard. If the function succeeds, it returns true.
 */
bool CbCopyBlob(void* pBlob, size_t size);

/**
 * Debug: Dumps the contents of the clipboard to the console.
 */
void CbDump();

/**
 * Initializes the clipboard.
 */
void CbInit();

#endif