#include <nanoshell.h>
#include <window.h>

#define CHART_RANDOMIZE 0x1000

#define CHART_WINDOW_WIDTH  (320)
#define CHART_WINDOW_HEIGHT (240)

#define CHART_AREA (CHART_WINDOW_HEIGHT - 36)
#define CHART_AREA_WIDTH (CHART_WINDOW_WIDTH - 50)

// simple shitty RNG
int g_randGen = 0x9521af17;

int Random (void)
{
	g_randGen += (int)0xe120fc15;
	uint64_t tmp = (uint64_t)g_randGen * 0x4a39b70d;
	uint32_t m1 = (tmp >> 32) ^ tmp;
	tmp = (uint64_t)m1 * 0x12fad5c9;
	uint32_t m2 = (tmp >> 32) ^ tmp;
	return m2 & 0x7FFFFFFF;//make it always positive.
}

int g_ChartPoints[6];

void GenerateChartPoints()
{
	for (uint32_t i = 0; i < ARRAY_COUNT(g_ChartPoints); i++)
	{
		g_ChartPoints [i] = Random () % CHART_AREA;
	}
}

void RenderChart(uint32_t color)
{
	//VidFillRect(0xFFFFFF, 2, TITLE_BAR_HEIGHT+2+30, GetScreenSizeX()-2, GetScreenSizeY()-2);
	for (uint32_t i = 0; i < ARRAY_COUNT(g_ChartPoints) - 1; i++)
	{
		int point1 = g_ChartPoints[i+0] + TITLE_BAR_HEIGHT + 30, 
		    point2 = g_ChartPoints[i+1] + TITLE_BAR_HEIGHT + 30;
		VidDrawLine(color /*Green*/, 
					3 + (i+0) * CHART_AREA_WIDTH / (ARRAY_COUNT(g_ChartPoints)-1), point1,
					3 + (i+1) * CHART_AREA_WIDTH / (ARRAY_COUNT(g_ChartPoints)-1), point2);
	}
}

void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	switch (messageType)
	{
		case EVENT_CREATE:
		{
			Rectangle r;
			RECT(r, 3, TITLE_BAR_HEIGHT + 3, CHART_WINDOW_WIDTH - 6, 25);
			AddControl(pWindow, CONTROL_BUTTON, r, "Randomize!", CHART_RANDOMIZE, 0, 0);
			
			GenerateChartPoints();
			
			break;
		}
		case EVENT_PAINT:
		{
			RenderChart(0x00FF00);
			break;
		}
		case EVENT_COMMAND:
			if (parm1 == CHART_RANDOMIZE)
			{
				// Un-render the chart.
				RenderChart(WINDOW_BACKGD_COLOR);
				
				// Randomize
				GenerateChartPoints();
				
				// Request a paint
				WndProc(pWindow, EVENT_PAINT, 0, 0);
			}
			break;
		default:
			DefaultWindowProc(pWindow, messageType, parm1, parm2);
			break;
	}
}

int main ()
{
	Window* pWindow = CreateWindow ("Chart Demo", 150, 150, CHART_WINDOW_WIDTH, CHART_WINDOW_HEIGHT + TITLE_BAR_HEIGHT, WndProc, 0);
	
	if (!pWindow)
		return 1;
	
	while (HandleMessages (pWindow));
	
	return 0;
}


