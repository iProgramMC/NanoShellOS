//  ***************************************************************
//  wm/mbardb.c - Creation date: 12/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
//  
//  Module description:
//      This module is responsible for managing the menu bars
//    for each window.
//  
//  ***************************************************************
#include "wi.h"

void WmMenuInitializeMenuBarItemAsEmpty (MenuBarTreeItem* this, int comboID)
{
	// Call the generic initializor for the menu bar tree item.
	this->m_comboID          = comboID;
	this->m_childrenCount    = 0;
	this->m_childrenCapacity = 4;
	this->m_childrenArray    = (MenuBarTreeItem*)MmAllocateK (this->m_childrenCapacity * sizeof (MenuBarTreeItem));
	if (!this->m_childrenArray)
	{
		this->m_childrenCapacity = 0;
		return;
	}
	memset (this->m_childrenArray, 0, 4 * sizeof (MenuBarTreeItem));
	this->m_text[0]          = 0;
	this->m_isOpen           = false;
}

bool WmMenuTryAddItemTo (MenuBarTreeItem* this, int comboID_to, int comboID_as, const char* text)
{
	if (this->m_comboID == comboID_to)
	{
		// Can it fit now?
		if (this->m_childrenCount >= this->m_childrenCapacity)
		{
			// Doesn't fit.  Need to expand.
			MenuBarTreeItem* new = (MenuBarTreeItem*)MmAllocateK (this->m_childrenCapacity * 2 * sizeof (MenuBarTreeItem));
			if (!new) return false;
			memset (new, 0, this->m_childrenCapacity * 2 * sizeof (MenuBarTreeItem));
			memcpy (new, this->m_childrenArray, this->m_childrenCapacity * sizeof (MenuBarTreeItem));
			this->m_childrenCapacity *= 2;
			
			// Get rid of the old one.  We've moved.
			MmFreeK (this->m_childrenArray);
			
			this->m_childrenArray = new;
		}
		
		//If we've expanded or otherwise can fit our new item into, add it.
		MenuBarTreeItem* pItem = this->m_childrenArray + this->m_childrenCount;
		this->m_childrenCount++;
		
		strcpy (pItem->m_text, text);
		pItem->m_comboID = comboID_as;
		pItem->m_childrenCount = 0;
		
		//Allocate a default of 2 items.
		pItem->m_childrenCapacity = 2;
		pItem->m_childrenArray    = (MenuBarTreeItem*)MmAllocateK (2 * sizeof (MenuBarTreeItem));
		if (!pItem->m_childrenArray) return false;
		memset (pItem->m_childrenArray, 0, 2 * sizeof (MenuBarTreeItem));
		
		//We were able to add it.  Leave and tell the caller that we did it.
		return true;
	}
	else
	{
		//try adding it to one of the children:
		for (int i = 0; i < this->m_childrenCount; i++)
		{
			if (WmMenuTryAddItemTo(&this->m_childrenArray[i], comboID_to, comboID_as, text))
				return true;//just added it, no need to add it anymore
		}
		return false;//couldn't add here, try another branch
	}
}

void WmMenuAddMenuBarItem (Control* this, int comboID_to, int comboID_as, const char* text)
{
	WmMenuTryAddItemTo(&this->m_menuBarData.m_root, comboID_to, comboID_as, text);
}

void WmMenuDeInitializeChild(MenuBarTreeItem *this)
{
	// deinitialize the children first.
	for (int i = 0; i < this->m_childrenCount; i++)
	{
		WmMenuDeInitializeChild(&this->m_childrenArray[i]);
	}
	
	// then, deinitialize this
	MmFreeK(this->m_childrenArray);
}

MenuBarData* WmCreateMenuBar()
{
	MenuBarData* pData = MmAllocate(sizeof(MenuBarData));
	if (!pData) return NULL;
	
	memset(pData, 0, sizeof (*pData));
	
	WmMenuInitializeMenuBarItemAsEmpty(&pData->m_root, 0);
	
	return pData;
}

void WmDeleteMenuBar(MenuBarData* pData)
{
	WmMenuDeInitializeChild(&pData->m_root);
	
	MmFree(pData);
}
