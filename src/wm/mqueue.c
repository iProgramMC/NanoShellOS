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
#define MASTER_WIN_EVT_QUEUE_MAX 32768
WindowEventQueueItem g_windowEventQueue[MASTER_WIN_EVT_QUEUE_MAX];

int g_windowEventQueueHead = 0;
int g_windowEventQueueTails[WINDOWS_MAX];
SafeLock g_windowEventQueueLock;

void OnWindowHung(Window *pWindow);

void WindowAddEventToMasterQueue(PWINDOW pWindow, int eventType, int parm1, int parm2)
{
	int tickCount = GetTickCount();
	if (pWindow->m_flags & WF_FROZEN)
	{
		// Can't send events to frozen objects! Just pretend it's handled already
		if (eventType != EVENT_BORDER_SIZE_UPDATE_PRIVATE && eventType != EVENT_REQUEST_RESIZE_PRIVATE && eventType != EVENT_REPAINT_PRIVATE)
		{
			pWindow->m_lastHandledMessagesWhen = tickCount;
			return;
		}
	}
	else
	{
		//if hasn't responded in 5 seconds
		if (pWindow->m_lastHandledMessagesWhen + 5000 <= tickCount)
		{
			//if we haven't sent a message in the last 5 seconds:
			if (pWindow->m_lastSentMessageTime + 5000 <= tickCount)
			{
				//we shouldn't actually think it's frozen. It might not be!
				pWindow->m_lastHandledMessagesWhen = tickCount;
				pWindow->m_lastSentMessageTime = tickCount;
			}
			else
			{
				OnWindowHung(pWindow);
			}
		}
	}
	
	// TODO: Check for queue overflow.
	
    WindowEventQueueItem item;
    item.m_destWindow = pWindow;
    item.m_eventType = eventType;
    item.m_parm1 = parm1;
    item.m_parm2 = parm2;
	
	LockAcquire(&g_windowEventQueueLock);
	
    g_windowEventQueue[g_windowEventQueueHead] = item;

    // Allow infinite re-use of the queue by looping it around.
    g_windowEventQueueHead = (g_windowEventQueueHead + 1) % MASTER_WIN_EVT_QUEUE_MAX;
	pWindow->m_lastSentMessageTime = GetTickCount();
	
	KeUnsuspendTasksWaitingForWM();
	KeUnsuspendTasksWaitingForObject(pWindow);
	
	LockFree(&g_windowEventQueueLock);
}

//This pops an event on the master queue with the window id.  If there isn't one, return false,
//otherwise, return true and fill in the pointers.
bool WindowPopEventFromQueue(PWINDOW pWindow, int *eventType, int *parm1, int *parm2)
{
    // Start from the window event queue tail.  Go through the queue until you hit the same point you started at.
    int offset = OFFSET_FROM_WINDOW_POINTER(pWindow);
    int backup = g_windowEventQueueTails[offset];
	
	LockAcquire(&g_windowEventQueueLock);

    // First of all, do we have an event right now right in front of us?
    if (g_windowEventQueue[backup].m_destWindow == pWindow)
    {
        // Yes! Advance the tail and return
        g_windowEventQueueTails[offset] = (g_windowEventQueueTails[offset] + 1) % MASTER_WIN_EVT_QUEUE_MAX;

        g_windowEventQueue[backup].m_destWindow = NULL;
        *eventType = g_windowEventQueue[backup].m_eventType;
        *parm1     = g_windowEventQueue[backup].m_parm1;
        *parm2     = g_windowEventQueue[backup].m_parm2;
		LockFree(&g_windowEventQueueLock);
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
			LockFree(&g_windowEventQueueLock);
            return true;
        }
    }

	LockFree(&g_windowEventQueueLock);
    // No, we still have not found it.  Return false, to signify that there are no more elements on the queue for now.
    return false;
}

void WmWaitForEvent(Window* pWindow)
{
	cli;
	
	for (int i = 0; i < MASTER_WIN_EVT_QUEUE_MAX; i++)
	{
		if (g_windowEventQueue[i].m_destWindow == pWindow)
		{
			// there are actually events! Don't wait.
			sti;
			return;
		}
	}
	
	WaitObject(pWindow); // this will restore interrupts later
}
