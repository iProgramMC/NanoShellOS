/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

     Codename V-Builder - Form Preview
******************************************/

#include <nanoshell/nanoshell.h>

#include "buttons.h"
#include "s_all.h"
#include "w_defs.h"

extern Window *g_pFormDesignerWindow, *g_pMainWindow;
extern DesignerControl* g_controlsFirst, *g_controlsLast;
extern DesignerControl* g_pEditedControl;

extern char* g_SourceCode;

Window* g_pPreviewWindow;

void CALLBACK VbPreviewWindow();

extern jmp_buf g_errorJumpBuffer;
extern int g_lineNum;

char g_ErrorBuffer[ERROR_BUFFER_SIZE];
char g_ErrorBuffer_Temp[ERROR_BUFFER_SIZE];

bool g_bFirstPaint = false;

void VbOnRuntimeError(Window* pWindow, int error, const char* pTitle)
{
	snprintf(g_ErrorBuffer_Temp, sizeof g_ErrorBuffer_Temp, "%s\n\n%s\nERROR %c%04d: %s", pTitle, g_ErrorBuffer, GetErrorCategory(error), GetErrorNo(error), GetErrorMessage(error));
	
	MessageBox(pWindow, g_ErrorBuffer_Temp, pTitle, MB_OK | (ICON_ERROR << 16));
	DestroyWindow(pWindow);
}

// TODO: Identify the bug that makes it fail when the code is inlined within the switch case
void VbTryCompile(Window* pWindow)
{
	int error = setjmp(g_errorJumpBuffer);
	if (error != 0)
	{
		VbOnRuntimeError(pWindow, error, "Compiler Error!");
		return;
	}
	
	CompileSource(g_SourceCode);
}

void VbPreviewOnClickButton(Window* pWindow, int comboID)
{
	// lookup the control with the comboID
	DesignerControl* C;
	for (C = g_controlsFirst; C; C = C->m_pNext)
	{
		if (C->m_comboID == comboID) break;
	}
	
	if (!C) return;
	
	// try to locate the button's click event
	char c[512];
	snprintf(c, sizeof c, "%s_Click", C->m_name);
	
	
	int error = setjmp(g_errorJumpBuffer);
	if (error != 0)
	{
		VbOnRuntimeError(pWindow, error, "Runtime Error!");
		return;
	}
	
	// try to call that function
	RunnerCall(c, "");
}

void FreezeWindow(Window* pWindow)
{
	SetWindowFlags(pWindow, GetWindowFlags(pWindow) | WF_FROZEN);
}

void UnfreezeWindow(Window* pWindow)
{
	SetWindowFlags(pWindow, GetWindowFlags(pWindow) & ~WF_FROZEN);
}

void CALLBACK PrgVbPreviewProc (Window* pWindow, int messageType, long parm1, long parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			g_bFirstPaint = true;
			
			int i = 0;
			for (DesignerControl* C = g_controlsFirst; C; C = C->m_pNext)
			{
				AddControl (pWindow, C->m_type, C->m_rect, C->m_text, 1000 + i, C->m_prm1, C->m_prm2);
				C->m_comboID = 1000 + i;
				i++;
			}
			
			break;
		}
		case EVENT_PAINT:
		{
			if (g_bFirstPaint)
			{
				VbTryCompile(pWindow);
				g_bFirstPaint = false;
			}
			break;
		}
		
		case EVENT_COMMAND:
		{
			VbPreviewOnClickButton(pWindow, parm1);
			break;
		}
		case EVENT_DESTROY:
		{
			CompilerTeardown();
			
			UnfreezeWindow(g_pFormDesignerWindow);
			
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
	}
}

void CALLBACK VbPreviewWindow()
{
	if (g_pPreviewWindow)
	{
		MessageBox(g_pMainWindow, "The preview is already running.", "Codename V-Builder", MB_OK | ICON_WARNING << 16);
		return;
	}
	
	char buffer[1024];
	strcpy(buffer, "Preview - ");
	strcat(buffer, GetWindowTitle(g_pFormDesignerWindow));
	
	Rectangle rect;
	GetWindowRect(g_pFormDesignerWindow, &rect);
	
	// Freeze the form designer window
	FreezeWindow(g_pFormDesignerWindow);
	
	g_pPreviewWindow = CreateWindow(
		buffer,
		rect.left + 30,
		rect.top  + 30,
		GetWidth(&rect),
		GetHeight(&rect),
		PrgVbPreviewProc,
		0
	);
}
