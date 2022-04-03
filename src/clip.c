/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Clipboard Utilitary Module
******************************************/
#include <clip.h>
#include <keyboard.h>
#include <print.h>

ClipboardVariant g_clipboard;

static void CbClearUnsafe()
{
	if (g_clipboard.m_type)
	{
		if (g_clipboard.m_generic_data_ptr)
			MmFreeK(g_clipboard.m_generic_data_ptr);
		g_clipboard.m_generic_data_ptr = NULL;
		g_clipboard.m_blob_size = 0;
		
		g_clipboard.m_type = CLIPBOARD_DATA_NONE;
	}
}
void CbClear()
{
	LockAcquire (&g_clipboard.m_lock);
	
	CbClearUnsafe();
	
	LockFree (&g_clipboard.m_lock);
}
bool CbCopyText(const char *pText)
{
	LockAcquire (&g_clipboard.m_lock);
	
	CbClearUnsafe();
	
	size_t pTextLen = strlen (pText);
	
	if (pTextLen < sizeof (g_clipboard.m_short_str))
	{
		g_clipboard.m_type = CLIPBOARD_DATA_TEXT;
		strcpy (g_clipboard.m_short_str, pText);
	}
	else
	{
		g_clipboard.m_char_str  = MmAllocate(pTextLen + 1);
		g_clipboard.m_blob_size = pTextLen + 1;
		
		if (!g_clipboard.m_char_str)
		{
			LockFree (&g_clipboard.m_lock);
			return false;
		}
		strcpy (g_clipboard.m_char_str, pText);
		g_clipboard.m_type = CLIPBOARD_DATA_LARGE_TEXT;
	}
	
	LockFree (&g_clipboard.m_lock);
	return true;
}
bool CbCopyBlob (void *pBlob, size_t sz)
{
	LockAcquire (&g_clipboard.m_lock);
	
	CbClearUnsafe();
	
	g_clipboard.m_generic_data_ptr = MmAllocateK (sz);
	if (!g_clipboard.m_generic_data_ptr) return false;
	
	memcpy(g_clipboard.m_generic_data_ptr, pBlob, sz);
	g_clipboard.m_blob_size = sz;
	
	g_clipboard.m_type = CLIPBOARD_DATA_BINARY;
	
	LockFree (&g_clipboard.m_lock);
	return true;
}
ClipboardVariant* CbGetCurrentVariant()
{
	LockAcquire (&g_clipboard.m_lock);
	return &g_clipboard;
}
void CbRelease(ClipboardVariant* pVariant)
{
	LockFree(&pVariant->m_lock);
}
void CbInit()
{
	memset (&g_clipboard, 0, sizeof g_clipboard);
}
void CbDumpVar (ClipboardVariant *pVar)
{
	switch (pVar->m_type)
	{
		case CLIPBOARD_DATA_TEXT:
			LogMsg("Text: \"%s\"", pVar->m_short_str);
			break;
		case CLIPBOARD_DATA_LARGE_TEXT:
			LogMsg("Large text!");
		default:
		{
			LogMsg("Binary data of size %d, dumping data as hex (ptr: %x):", pVar->m_blob_size, pVar->m_generic_data_ptr);
			// just copy and paste from MemSpy
			if (!pVar->m_generic_data_ptr)
				LogMsg("(null)");
			else
				DumpBytesAsHex (pVar->m_generic_data_ptr, pVar->m_blob_size, true);
			
			break;
		}
	}
}

void CbDump()
{
	CbDumpVar (&g_clipboard);
}

// A hacked together thing to shoehorn clipboard support into any application that wants text input.
// This hooks directly into the keyboard driver, and as such, it can infiltrate ANY application that requests keyboard input.
// Note that this may interfere / mess with some games which use the raw keyboard scan codes, so use wisely.
void KbAddKeyToBuffer(char key);
void KbAddRawKeyToBuffer(char key);

// high 8 bits - flags (e.g. shift), low 8 bits - second code.
// Shift will be released automatically when a non-shift key appears
const unsigned short g_ascii_to_raw_input_codes[] =
{
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0039, 0x8002, 0x8028, 0x8004, 0x8005, 0x8006, 0x8008, 0x0028, 0x800A, 0x800B, 0x8009, 0x800D, 0x0033, 0x000C, 0x0034, 0x0035,
	0x000B, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x8027, 0x0027, 0x8033, 0x000D, 0x8034, 0x8035,
	0x8003, 0x801E, 0x8030, 0x802E, 0x8020, 0x8012, 0x8021, 0x8022, 0x8023, 0x8017, 0x8024, 0x8025, 0x8026, 0x8032, 0x8031, 0x8018,
	0x8019, 0x8010, 0x8013, 0x801F, 0x8014, 0x8016, 0x802F, 0x8011, 0x802D, 0x8015, 0x802C, 0x001A, 0x002B, 0x001B, 0x8007, 0x800C,
	0x0029, 0x001E, 0x0030, 0x002E, 0x0020, 0x0012, 0x0021, 0x0022, 0x0023, 0x0017, 0x0024, 0x0025, 0x0026, 0x0032, 0x0031, 0x0018,
	0x0019, 0x0010, 0x0013, 0x001F, 0x0014, 0x0016, 0x002F, 0x0011, 0x002D, 0x0015, 0x002C, 0x801A, 0x802B, 0x801B, 0x8029, 0x0000,
};

bool CbPushTextIntoBufferInt(ClipboardVariant* pVar)
{
	if (pVar->m_type == CLIPBOARD_DATA_TEXT)
	{
		const char *pChars = pVar->m_short_str;
		bool holds_shift = false;
		
		// hacky hack alert:
		// send a shift release so that the window manager does not get confused
		// note that this may mess with user's keyboard inputs, so be careful
		KbAddRawKeyToBuffer ((char)(SCANCODE_RELEASE | KEY_LSHIFT));
		KbAddRawKeyToBuffer ((char)(SCANCODE_RELEASE | KEY_RSHIFT));
		
		while (*pChars)
		{
			char chr = *pChars;
			if ((unsigned char)(chr) >= 0x7F)
			{
				pChars++;
				continue;
			}
			
			KbAddKeyToBuffer (chr);
			
			// Support adding at codes too, for window manager to handle.
			// I don't really feel like adding a SECONDARY input queue JUST for this
			// purpose, so I'm not.
			unsigned short at_code = g_ascii_to_raw_input_codes[(unsigned char)chr];
			if (!at_code)
			{
				pChars++;
				continue;
			}
			
			if (at_code & 0x8000) // Wants shift?
			{
				if (!holds_shift)
					KbAddRawKeyToBuffer (KEY_LSHIFT);
				holds_shift = true;
			}
			else
			{
				if (holds_shift)
					KbAddRawKeyToBuffer ((char)(SCANCODE_RELEASE | KEY_LSHIFT));
				holds_shift = false;
			}
			
			// Add the key code itself
			unsigned char nope = (unsigned char)at_code;
			KbAddRawKeyToBuffer (nope);
			
			// Immediately release this key
			KbAddRawKeyToBuffer ((char)(SCANCODE_RELEASE | nope));
			
			pChars++;
		}
		return true;
	}
	else if (pVar->m_type == CLIPBOARD_DATA_LARGE_TEXT)
	{
		SLogMsg("E: Avoided pasting, because it may take a long time. Better let the application itself do this.");
		return false;
	}
	else
	{
		SLogMsg("E: The clipboard does not contain text.");
		return false;
	}
}
bool CbPushTextIntoBuffer()
{
	return CbPushTextIntoBufferInt (&g_clipboard);
}

