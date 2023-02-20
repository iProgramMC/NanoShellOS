//  ***************************************************************
//  rt.c - Creation date: 20/02/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// This module's purpose: handle the resource table of a process.
// A resource table can contain icons, bitmaps, and strings.

#include <process.h>

void RstFreeResource(Resource* pResource)
{
	switch (pResource->m_type)
	{
		case RES_STRING:
			MmFree((void*)pResource->m_pString);
			break;
		// note: The image is allocated with `BitmapAllocate`
		case RES_BITMAP:
			MmFree(pResource->m_pImage);
			break;
		// note: The icon is also allocated with `BitmapAllocate`
		case RES_ICON:
			MmFree(pResource->m_pIcon);
			break;
		default:
			SLogMsg("Don't know how to dispose of resource %p with ID %d and type %d", pResource, pResource->m_id, pResource->m_type);
			break;
	}
}
