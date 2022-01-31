#include <nanoshell.h>

int cosa[]={    0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 995,1000, 995, 978, 951, 914, 866, 809, 743, 669, 588, 500, 407, 309, 208, 105,   0,-105,-208,-309,-407,-500,-588,-669,-743,-809,-866,-914,-951,-978,-995,-1000,-995,-978,-951,-914,-866,-809,-743,-669,-588,-500,-407,-309,-208,-105 };
int sina[]={ -1000,-995,-978,-951,-914,-866,-809,-743,-669,-588,-500,-407,-309,-208,-105,   0, 105, 208, 309, 407, 500, 588, 669, 743, 809, 866, 914, 951, 978, 995,1000, 995, 978, 951, 914, 866, 809, 743, 669, 588, 500, 407, 309, 208, 105,   0,-105,-208,-309,-407,-500,-588,-669,-743,-809,-866,-914,-951,-978,-995 };

#define FontWidthHeightRatio 2

#define KB_BUF_SIZE 128//trust me.
typedef struct
{
	//TODO: jam this into the API libraries. This is quick&dirty.
	int  type; // ConsoleType enum
	int  width, height; // width and height
	uint16_t *textBuffer; // unused in fb mode
	uint16_t color; // colors
	int  curX, curY; // cursor X and Y positions
	bool pushOrWrap;// check if we should push whole screen up, or clear&wrap
	void* m_vbeData;//vbe data to switch to when drawing.  Don't need this.
	int  offX, offY;
	int  font;
	int  cwidth, cheight;
	bool m_dirty;
	char m_inputBuffer[KB_BUF_SIZE];
	int  m_inputBufferBeg, m_inputBufferEnd;
}
Console;
Console* g_pCurrentConsole;

void DrawPoint (int x, int y, char p)
{
	char text [2];
	g_pCurrentConsole->curX = x;
	g_pCurrentConsole->curY = y;
	text[0] = p; text[1] = '\0';
	PutString (text);
}

void DrawText(const char* text, int x, int y)
{
	g_pCurrentConsole->curX = x;
	g_pCurrentConsole->curY = y;
	PutString(text);
}

void DrawCircle(int handMax, int sXcen, int sYcen)
{
	int x, y, r;
	char c;
	for (r = 0; r < 60; r++)
	{
		x = (cosa[r] * handMax                        / 1000) + sXcen;
		y = (sina[r] * handMax / FontWidthHeightRatio / 1000) + sYcen;
		
		if (r % 5 == 0)
			c = 'o';
		else
			c = '.';
		
		DrawPoint (x, y, c);
	}
}

void DrawHand (int minute, int length, char c, int sXcen, int sYcen)
{
	for (int n = 1; n < length*2; n++)
	{
		int x = (cosa[minute] * n                         / 1000) + sXcen;
		int y = (sina[minute] * n / FontWidthHeightRatio  / 1000) + sYcen;
		DrawPoint(x, y, c);
	}
}

TimeStruct time;
int main()
{
	// Obtain console handle.
	Console* pConsole = GetConsole();
	g_pCurrentConsole = pConsole;
	
	int sXmax, sYmax, smax, handMax, sXcen, sYcen;
	sXmax = pConsole->width, sYmax = pConsole->height;
	
	if (sXmax <= sYmax)
		smax   = sXmax;
	else
		smax   = sYmax;
	
	handMax = (smax/2)-1;
	sXcen = sXmax/2;
	sYcen = sYmax/2;
	char digital_time[16];
	
	while (1)
	{
		time = *GetTime();
		
		DrawCircle(handMax*2-1, sXcen, sYcen);
		
		DrawHand(time.seconds,                    handMax - 1, '.', sXcen, sYcen);
		DrawHand(time.minutes,                    handMax - 2, 'm', sXcen, sYcen);
		DrawHand(time.hours%12*5+time.minutes/12, 2*handMax/3, 'h', sXcen, sYcen);
		
		DrawText(".:ACLOCK:.", sXcen-5, sYcen-(3*handMax/5));
		sprintf (digital_time, "[%02d:%02d:%02d]", time.hours, time.minutes, time.seconds);
		DrawText(digital_time, sXcen-5, sYcen+(3*handMax/5));
		
		TmSleep(1000);
		
		DrawHand(time.hours%12*5+time.minutes/12, 2*handMax/3, ' ', sXcen, sYcen);
		DrawHand(time.minutes,                    handMax - 2, ' ', sXcen, sYcen);
		DrawHand(time.seconds,                    handMax - 1, ' ', sXcen, sYcen);
	}
	
	return 0;
}


