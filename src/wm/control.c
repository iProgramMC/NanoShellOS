/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

     Window Widget Management Module
******************************************/
#include "wi.h"

Control* GetControlByComboID(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_active)
		{
			if (pWindow->m_pControlArray[i].m_comboID == comboID)
				return &pWindow->m_pControlArray[i];
		}
	}
	return NULL;
}

//Returns an index, because we might want to relocate the m_pControlArray later.
int AddControlEx(Window* pWindow, int type, int anchoringMode, Rectangle rect, const char* text, int comboID, int p1, int p2)
{
	if (type == CONTROL_SIMPLE_VLINE)
	{
		type = CONTROL_SIMPLE_HLINE;
		p1 |= 1;
	}
	if (!pWindow->m_pControlArray)
	{
		VidSetVBEData(NULL);
		ILogMsg("No pControlArray?");
		return -1;
	}
	int index = -1;
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (!pWindow->m_pControlArray[i].m_active)
		{
			index = i;
			break;
		}
	}
	if (index <= -1)
	{
		//Couldn't find a spot in the currently allocated thing.
		//Perhaps we need to expand the array.
		int cal = pWindow->m_controlArrayLen;
		if (cal < 2) cal = 2;
		
		cal += cal / 2;
		//series: 2, 3, 4, 6, 9, 13, 19, 28, 42, ...
		
		SLogMsg("Expanding control array from %d to %d", pWindow->m_controlArrayLen, cal);
		
		Control* newCtlArray = MmReAllocateK(pWindow->m_pControlArray, sizeof(Control) * cal);
		if (!newCtlArray)
		{
			SLogMsg("Oops, cannot add control %d to window %x", type, pWindow);
			return -1;
		}
		pWindow->m_pControlArray = newCtlArray;
		memset (&pWindow->m_pControlArray[pWindow->m_controlArrayLen], 0, sizeof(Control) * (cal - pWindow->m_controlArrayLen));
		pWindow->m_controlArrayLen = cal;
		
		// last, re-search the thing
		index = -1;
		for (int i = 0; i < pWindow->m_controlArrayLen; i++)
		{
			if (!pWindow->m_pControlArray[i].m_active)
			{
				index = i;
				break;
			}
		}
	}
	if (index <= -1)
	{
		return -1;
	}
	
	// add the control itself:
	Control *pControl = &pWindow->m_pControlArray[index];
	pControl->m_active  = true;
	pControl->m_type    = type;
	pControl->m_dataPtr = NULL;
	pControl->m_rect    = pControl->m_triedRect = rect;
	pControl->m_comboID = comboID;
	pControl->m_parm1   = p1;
	pControl->m_parm2   = p2;
	pControl->m_bMarkedForDeletion = false;
	pControl->m_anchorMode = anchoringMode;
	
	if (text)
		strcpy (pControl->m_text, text);
	else
		pControl->m_text[0] = '\0';
	
	pControl->OnEvent = GetWidgetOnEventFunction(type);
	
	if (type == CONTROL_VSCROLLBAR || type == CONTROL_HSCROLLBAR)
	{
		pControl->m_scrollBarData.m_min = (pControl->m_parm1   >>  16);
		pControl->m_scrollBarData.m_max = (pControl->m_parm1 & 0xFFFF);
		pControl->m_scrollBarData.m_pos = (pControl->m_parm2 & 0xFFFF);
		pControl->m_scrollBarData.m_dbi = (pControl->m_parm2   >>  16);
		
		if (pControl->m_scrollBarData.m_dbi == 0)
			pControl->m_scrollBarData.m_dbi  = 1;
		
		if (pControl->m_scrollBarData.m_pos < pControl->m_scrollBarData.m_min)
			pControl->m_scrollBarData.m_pos = pControl->m_scrollBarData.m_min;
		if (pControl->m_scrollBarData.m_pos >= pControl->m_scrollBarData.m_max)
			pControl->m_scrollBarData.m_pos =  pControl->m_scrollBarData.m_max - 1;
	}
	else if (type == CONTROL_TEXTINPUT)
	{
		//by default you have single line
		CtlTextInputUpdateMode (pControl);
	}
	else if (type == CONTROL_CHECKBOX)
	{
		//by default you have single line
		pControl->m_checkBoxData.m_checked = p1 != 0;
		pControl->m_checkBoxData.m_clicked = 0;
	}
	
	//register an event for the window:
	//WindowRegisterEvent(pWindow, EVENT_PAINT, 0, 0);
	
	//call EVENT_CREATE to let the ctl initialize its data
	pControl->OnEvent(pControl, EVENT_CREATE, 0, 0, pWindow);
	
	//The control should be able to adjust its starting rect when created.
	pControl->m_triedRect = pControl->m_rect;
	
	return index;
}

int AddControl(Window* pWindow, int type, Rectangle rect, const char* text, int comboID, int p1, int p2)
{
	return
	AddControlEx(pWindow, type, 0, rect, text, comboID, p1, p2);
}

void RemoveControlInternal (Window* pWindow, int controlIndex)
{
	if (controlIndex >= pWindow->m_controlArrayLen || controlIndex < 0) return;
	
	Control* pControl = &pWindow->m_pControlArray[controlIndex];
	pControl->m_active = false;
	pControl->m_bMarkedForDeletion = false;
	pControl->OnEvent = NULL;
}

void RemoveControl(Window* pWindow, int comboID)
{
	int controlIndex = -1;
	
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		Control* p = &pWindow->m_pControlArray[i];
		if (p->m_active  &&  p->m_comboID == comboID)
		{
			controlIndex = i;
			break;
		}
	}
	
	if (controlIndex == -1)
	{
		return;
	}
	
	Control* pControl = &pWindow->m_pControlArray[controlIndex];
	pControl->OnEvent(pControl, EVENT_DESTROY, 0, 0, pWindow);
	
	RemoveControlInternal(pWindow, controlIndex);
}