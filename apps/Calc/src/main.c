/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
          Calculator application

             Main source file
******************************************/

#include <nsstandard.h>

#define VERSION_BUTTON_OK_COMBO 0x1000

#define CALC_WIDTH   (200)
#define CALC_HEIGHT  (204)

float Calculate(const char*pText, int *errorCode);//calculate.c
const char* ErrorCodeToString (int ec);

int variable = 0, errorcode = 0;
char input[250];
char outpt[250];
char error[250];

float resultCalculation = 0;

bool bResultMode = false;

long long absll (long long a)
{
	if (a < 0)
		a = -a;
	return a;
}

void FormatFloatToString (char* out, float n, int precision)
{
	//TODO: won't handle large enough numbers.  Precision should be smaller than 10.
	long long po10 = 1;
	for (int i = 0; i < precision; i++) po10 *= 10;
	
	float fmt = n * po10;
	
	long long fmtint = (long long)fmt, fmtintabs = absll(fmtint);
	char format_string[100];
	sprintf(format_string, "%cd.%c0%dd", '%', '%', precision);
	
	sprintf(out, format_string, (int)(fmtint/po10), (int)(fmtintabs % po10));
}

void RequestRepaint(Window *pWindow);

// CalcButtons
enum {
	BUTTONID_CALC0 = 0x2000,
	BUTTONID_CALC1,
	BUTTONID_CALC2,
	BUTTONID_CALC3,
	BUTTONID_CALC4,
	BUTTONID_CALC5,
	BUTTONID_CALC6,
	BUTTONID_CALC7,
	BUTTONID_CALC8,
	BUTTONID_CALC9,
	BUTTONID_EQUAL,
	BUTTONID_MODULO,
	BUTTONID_TIMES,
	BUTTONID_DIVIDE,
	BUTTONID_PLUS,
	BUTTONID_MINUS,
	BUTTONID_CLEAR,
};
void CopyTextBoxContents(Window* pWindow)
{
	const char *strin = TextInputGetRawText (pWindow, 100);
	if (!strin)
	{
		// it's not there, whoa
		// bye!
		DefaultWindowProc(pWindow, EVENT_DESTROY, 0, 0);
	}
	
	int sl = strlen (strin);
	if (sl > 240)
		sl = 240;
	memcpy (input, strin, sl);
	input[sl] = 0;
}
char syms[] = "0123456789=%*/+-C";
#define WND_BORDER_SIZE 3
void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			//2-10+5*8-2*3 = 26
			
			Rectangle r;
			//add each button, a terrible solution indeed
			int bWidth = (pWindow->m_vbeData.m_width-WND_BORDER_SIZE*2)/4, bHeight = (pWindow->m_vbeData.m_height-4-4) / 5;
			
			RECT (r, 10, 10, CALC_WIDTH - 20, 20);
			
			AddControl (pWindow, CONTROL_TEXTINPUT, r, "", 100, 0, 0);
			
			#define BUTTON_GAP 2
			#define BUTTON(x,y,t)\
				RECT(r,\
				x * bWidth + 1 + WND_BORDER_SIZE + BUTTON_GAP,\
				y * bHeight + 1 + WND_BORDER_SIZE + BUTTON_GAP,\
				bWidth  - BUTTON_GAP*2,\
				bHeight - BUTTON_GAP*2);\
				AddControl (pWindow, CONTROL_BUTTON, r, #t, BUTTONID_CALC ## t, 0, 0);
			#define BUTTON2(x,y,t,t2)\
				RECT(r,\
				x * bWidth + 1 + WND_BORDER_SIZE + BUTTON_GAP,\
				y * bHeight + 1 + WND_BORDER_SIZE + BUTTON_GAP,\
				bWidth  - BUTTON_GAP*2,\
				bHeight - BUTTON_GAP*2);\
				AddControl (pWindow, CONTROL_BUTTON, r, t, BUTTONID_ ## t2, 0, 0);
				
			BUTTON(0,1,7)
			BUTTON(1,1,8)
			BUTTON(2,1,9)
			BUTTON2(3,1,"+",PLUS)
			BUTTON(0,2,4)
			BUTTON(1,2,5)
			BUTTON(2,2,6)
			BUTTON2(3,2,"-",MINUS)
			BUTTON(0,3,1)
			BUTTON(1,3,2)
			BUTTON(2,3,3)
			BUTTON2(3,3,"/",DIVIDE)
			BUTTON2(0,4,"=",EQUAL)
			BUTTON(1,4,0)
			BUTTON2(2,4,"C",CLEAR)
			BUTTON2(3,4,"*",TIMES)
			
			break;
		}
		case EVENT_KEYRAW:
		{
			if (bResultMode)
			{
				bResultMode = false;
				strcpy (input, "");
				SetTextInputText (pWindow, 100, input);
			}
			DefaultWindowProc (pWindow, EVENT_KEYRAW, 0, 0);
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == BUTTONID_EQUAL)
			{
				//the input array maybe out of sync with the text box, so copy the textbox's contents over
				CopyTextBoxContents(pWindow);
				
				//perform the calculation!
				int error_code = 0;
				float result = Calculate(input, &error_code);
				strcpy (input, "");
				if (error_code)
				{
					MessageBox(pWindow, ErrorCodeToString(error_code), "Calculator", MB_OK | ICON_ERROR << 16);
					SetTextInputText (pWindow, 100, input);
				}
				else
				{
					FormatFloatToString(outpt, result, 4);
					bResultMode = true;
					SetTextInputText (pWindow, 100, outpt);
				}
			}
			else if (parm1 == BUTTONID_CLEAR)
			{
				bResultMode = false;
				strcpy (input, "");
				SetTextInputText (pWindow, 100, input);
			}
			else
			{
				if (bResultMode)
				{
					strcpy (input, "");
					bResultMode = false;
				}
				else
				{
					// take in the string
					CopyTextBoxContents(pWindow);
				}
				
				//Append symbol.
				char digit = syms[parm1 - BUTTONID_CALC0];
				char str[2]; str[0] = digit, str[1] = 0;
				
				
				if (strlen (input) < 240)
					strcat (input, str);
				
				// reput it in the textbox
				SetTextInputText (pWindow, 100, input);
			}
			RequestRepaint (pWindow);
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

int main()
{
	Window* pWindow = CreateWindow (
		"Calculator",
		300,
		100,
		CALC_WIDTH,
		CALC_HEIGHT,
		WndProc,
		0
	);
	SetWindowIcon(pWindow, ICON_CALCULATOR);
	
	if (!pWindow)
		return 1;
	
	while (HandleMessages (pWindow));
	
	return 0;
}


