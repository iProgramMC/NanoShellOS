/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

        Clipboard Utilitary Module
******************************************/
#include <clip.h>

ClipboardVariant g_clipboard;

static void CbClearUnsafe()
{
	if (g_clipboard.m_type)
	{
		if (g_clipboard.m_generic_data_ptr)
			MmFreeK(g_clipboard.m_generic_data_ptr);
		g_clipboard.m_generic_data_ptr = NULL;
		
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
		g_clipboard.m_char_str = MmAllocate(pTextLen + 1);
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
	
	g_clipboard.m_type = CLIPBOARD_DATA_BINARY;
	
	LockFree (&g_clipboard.m_lock);
	return true;
}
void CbPushTextIntoBuffer()
{
	SLogMsg("TODO %s", __func__);
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
