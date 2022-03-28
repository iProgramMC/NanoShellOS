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
	
	union {
		int   m_int_data;
		void *m_generic_data_ptr;
		char *m_char_str;
	};
}
ClipboardVariant;

ClipboardVariant* CbGetCurrentVariant();                // Locks the clipboard until the current variant is gotten rid of
void              CbRelease(ClipboardVariant*);         // Releases a lock to the clipboard variant.
void              CbPushTextIntoBuffer();               // If possible, pushes text as keyboard events.

void              CbClear();                            // Clears the clipboard
bool              CbCopyText(const char* pText);        // Copies a piece of text into the clipboard.  Will return true on success.
bool              CbCopyBlob(void* pBlob, size_t size); // Copies a piece of data into the clipboard.  Will return true on success.

#endif