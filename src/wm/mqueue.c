/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Window Manager Master Queue Module
******************************************/
#include "wi.h"

#define OFFSET_FROM_WINDOW_POINTER(pWindow) (pWindow - g_windows)
typedef struct
{
    PWINDOW m_destWindow;
    int m_eventType, m_parm1, m_parm2;
}
WindowEventQueueItem;
#define MASTER_WIN_EVT_QUEUE_MAX 4096
WindowEventQueueItem g_windowEventQueue[MASTER_WIN_EVT_QUEUE_MAX];

int g_windowEventQueueHead = 0;
int g_windowEventQueueTails[WINDOWS_MAX];

void OnWindowHung(Window *pWindow);

void WindowAddEventToMasterQueue(PWINDOW pWindow, int eventType, int parm1, int parm2)
{
	if (pWindow->m_flags & WF_FROZEN)
	{
		// Can't send events to frozen objects! Just pretend it's handled already
		pWindow->m_lastHandledMessagesWhen = GetTickCount();
		return;
	}
	
	//if hasn't responded in 5 seconds:
	if (pWindow->m_lastHandledMessagesWhen + 5000 <= GetTickCount())
	{
		OnWindowHung(pWindow);
	}
	
    WindowEventQueueItem item;
    item.m_destWindow = pWindow;
    item.m_eventType = eventType;
    item.m_parm1 = parm1;
    item.m_parm2 = parm2;
    g_windowEventQueue[g_windowEventQueueHead] = item;

    // Allow infinite re-use of the queue by looping it around.
    g_windowEventQueueHead = (g_windowEventQueueHead + 1) % MASTER_WIN_EVT_QUEUE_MAX;
}

//This pops an event on the master queue with the window id.  If there isn't one, return false,
//otherwise, return true and fill in the pointers.
bool WindowPopEventFromQueue(PWINDOW pWindow, int *eventType, int *parm1, int *parm2)
{
    // Start from the window event queue tail.  Go through the queue until you hit the same point you started at.
    int offset = OFFSET_FROM_WINDOW_POINTER(pWindow);
    int backup = g_windowEventQueueTails[offset];

    // First of all, do we have an event right now right in front of us?
    if (g_windowEventQueue[backup].m_destWindow == pWindow)
    {
        // Yes! Advance the tail and return
        g_windowEventQueueTails[offset] = (g_windowEventQueueTails[offset] + 1) % MASTER_WIN_EVT_QUEUE_MAX;

        g_windowEventQueue[backup].m_destWindow = NULL;
        *eventType = g_windowEventQueue[backup].m_eventType;
        *parm1     = g_windowEventQueue[backup].m_parm1;
        *parm2     = g_windowEventQueue[backup].m_parm2;
        return true;
    }

    // No.  Look through the entire queue array
    for (
        g_windowEventQueueTails[offset] = (g_windowEventQueueTails[offset] + 1) % MASTER_WIN_EVT_QUEUE_MAX;
        g_windowEventQueueTails[offset] != backup;
        g_windowEventQueueTails[offset] = (g_windowEventQueueTails[offset] + 1) % MASTER_WIN_EVT_QUEUE_MAX
    )
    {
        int tail = g_windowEventQueueTails[offset];
        if (g_windowEventQueue[tail].m_destWindow == pWindow)
        {
            // Yes! Advance the tail and return
            g_windowEventQueueTails[offset] = (g_windowEventQueueTails[offset] + 1) % MASTER_WIN_EVT_QUEUE_MAX;

            g_windowEventQueue[tail].m_destWindow = NULL;
            *eventType = g_windowEventQueue[tail].m_eventType;
            *parm1 = g_windowEventQueue[tail].m_parm1;
            *parm2 = g_windowEventQueue[tail].m_parm2;
            return true;
        }
    }

    // No, we still have not found it.  Return false, to signify that there are no more elements on the queue for now.
    return false;
}
