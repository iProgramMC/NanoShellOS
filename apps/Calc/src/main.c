/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
          Calculator application

             Main source file
******************************************/

#include <nsstandard.h>

#define VERSION_BUTTON_OK_COMBO 0x1000

#define CALC_WIDTH   (200)
#define CALC_HEIGHT  (204 + TITLE_BAR_HEIGHT)

float Calculate(const char*pText, int *errorCode);//calculate.c
const char* ErrorCodeToString (int ec);

int variable = 0, errorcode = 0;
char input[250], inputold[250];
char outpt[250], outptold[250];
char error[250], errorold[250];

float resultCalculation = 0;

void FormatFloatToString (char* out, float n, int precision)
{
	/*if (n != n)
	{
		//NaN
		sprintf(out, "nan");
		return;
	}*/
	
	//TODO: won't handle large enough numbers.  Precision should be smaller than 10.
	long long po10 = 1;
	for (int i = 0; i < precision; i++) po10 *= 10;
	
	float fmt = n * po10;
	
	long long fmtint = (long long)fmt;
	char format_string[100];
	sprintf(format_string, "%cx %cd.%c0%dd", '%', '%', '%', precision);
	
	sprintf(out, format_string, *((long*)&n), (int)(fmtint/po10), (int)(fmtint % po10));
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
char syms[] = "0123456789=%*/+-C";
#define WND_BORDER_SIZE 3
#define TITLE_BAR_SIZE  TITLE_BAR_HEIGHT
void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			//2-10+5*8-2*3 = 26
			
			Rectangle r;
			//add each button, a terrible solution indeed
			int bWidth = (pWindow->m_vbeData.m_width-WND_BORDER_SIZE*2)/4, bHeight = (pWindow->m_vbeData.m_height-4-4-TITLE_BAR_HEIGHT) / 5;
			
			#define BUTTON(x,y,t)\
				RECT(r, x * bWidth + 1 + WND_BORDER_SIZE, y * bHeight + 1 + TITLE_BAR_HEIGHT + WND_BORDER_SIZE, bWidth, bHeight);\
				AddControl (pWindow, CONTROL_BUTTON, r, #t, BUTTONID_CALC ## t, 0, 0);
			#define BUTTON2(x,y,t,t2)\
				RECT(r, x * bWidth + 1 + WND_BORDER_SIZE, y * bHeight + 1 + TITLE_BAR_HEIGHT + WND_BORDER_SIZE, bWidth, bHeight);\
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
		case EVENT_PAINT:
		{
			VidTextOut(inputold, 5, TITLE_BAR_HEIGHT+05, WINDOW_BACKGD_COLOR, WINDOW_BACKGD_COLOR);//undraw the old text
			VidTextOut(input,    5, TITLE_BAR_HEIGHT+05, 0x000000,            WINDOW_BACKGD_COLOR);//draw the new one on
			VidTextOut(outptold, 5, TITLE_BAR_HEIGHT+15, WINDOW_BACKGD_COLOR, WINDOW_BACKGD_COLOR);//undraw the old text
			VidTextOut(outpt,    5, TITLE_BAR_HEIGHT+15, 0x000000,            WINDOW_BACKGD_COLOR);//draw the new one on
			VidTextOut(errorold, 5, TITLE_BAR_HEIGHT+25, WINDOW_BACKGD_COLOR, WINDOW_BACKGD_COLOR);//undraw the old text
			VidTextOut(error,    5, TITLE_BAR_HEIGHT+25, 0x000000,            WINDOW_BACKGD_COLOR);//draw the new one on
			memcpy (inputold, input, sizeof (input));
			memcpy (outptold, outpt, sizeof (outpt));
			memcpy (errorold, error, sizeof (error));
			break;
		}
		case EVENT_COMMAND:
		{
			if (parm1 == BUTTONID_EQUAL)
			{
				//perform the calculation!
				int error_code = 0;
				float result = Calculate(input, &error_code);
				if (error_code)
				{
					strcpy(error, ErrorCodeToString(error_code));
				}
				else
				{
					FormatFloatToString(outpt, result, 4);
				}
				strcpy (input, "");
			}
			else if (parm1 == BUTTONID_CLEAR)
			{
				strcpy (input, "");
			}
			else
			{
				//Append symbol.
				char digit = syms[parm1 - BUTTONID_CALC0];
				char str[2]; str[0] = digit, str[1] = 0;
				
				if (strlen (input) < 240)
					strcat (input, str);
			}
			RequestRepaint (pWindow);
			break;
		}
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

int NsMain (int argc, char** argv)
{
	char fmt[100];
	float variable_a = 6;
	FormatFloatToString(fmt, 5.f / variable_a, 6);
	LogMsg("\n\n\n\nTest: %s", fmt);
	
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
	
	//memset (&time, 0, sizeof(time));
	
	while (HandleMessages (pWindow))
	{
		;
	}
	
	return 0;
}


