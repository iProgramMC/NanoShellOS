/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
     Fullscreen fireworks application

             Main source file
******************************************/
#include <nsstandard.h>

// 2022 may not have been great, but it sure was NanoShell's first year.
// Here's to hoping 2023 will be better. -iProgramInCpp
// (This is the last line of code written in 2022.)

#define UPDATE_MS 15
int g_tickCountRightNow = 0;

typedef struct Particle
{
	struct Particle *m_next, *m_prev;
	int m_x, m_y;
	double m_dx, m_dy;
	int m_color;
	double m_velX, m_velY;
	int m_expireAt;
	bool m_bCanExplode;
	int m_explosionRange;
	unsigned m_oldColor;
}
Particle;

Particle* g_first;

void GetTimeStampCounter(uint32_t* high, uint32_t* low)
{
	if (!high && !low) return; //! What's the point?
	int edx, eax;
	__asm__ volatile ("rdtsc":"=a"(eax),"=d"(edx));
	if (high) *high = edx;
	if (low ) *low  = eax;
}

int g_seed = 0;

// Gets a random double between 0 and 1.
double GetRandomDouble()
{
	return (double)rand() / (double)RAND_MAX;
}

#define LE_PI 3.14159265358979323846264338327950

// useful double functions
double sin(double x)
{
    double result;
    __asm__ volatile("fsin" : "=t"(result) : "0"(x));
    return result;
}
double sqrt(double x)
{
    double result;
    __asm__ volatile("fsqrt" : "=t"(result) : "0"(x));
    return result;
}
double cos(double x)
{
    double result;
    __asm__ volatile("fcos" : "=t"(result) : "0"(x));
    return result;
}

void AddParticle(Particle* pPart)
{
	if (g_first)
	{
		g_first->m_prev = pPart;
		pPart->m_next = g_first;
	}
	g_first = pPart;
}

void RemoveParticle(Particle* pPart)
{
	if (pPart->m_next)
	{
		pPart->m_next->m_prev = pPart->m_prev;
	}
	if (pPart->m_prev)
	{
		pPart->m_prev->m_next = pPart->m_next;
	}
	if (pPart == g_first)
	{
		g_first = pPart->m_next;
	}
	
	free(pPart);
}

// Gets a random double between -1 and 1.
double GetRandomDoubleSigned()
{
	return GetRandomDouble() * 2 - 1;
}

unsigned GetRandomColor()
{
	return (rand() + 0x808080) & 0xFFFFFF;
}

int GetRandomExpiry(int a, int b)
{
	return g_tickCountRightNow + a + rand() % b;
}

void SetupParticle(Particle* pPart, Particle* pFireworkBase, bool bCanExplode)
{
	int offsetY = GetScreenSizeY() * 300 / 768;
	int offsetX = GetScreenSizeX() * 400 / 1024;
	
	pPart->m_bCanExplode = bCanExplode;
	
	if (!bCanExplode)
	{
		// Is a remnant of an explosion.
		
		double angle = GetRandomDoubleSigned() * (2*LE_PI);
		
		pPart->m_velX = cos(angle) * GetRandomDoubleSigned() * pFireworkBase->m_explosionRange;
		pPart->m_velY = sin(angle) * GetRandomDoubleSigned() * pFireworkBase->m_explosionRange;
		pPart->m_expireAt = GetRandomExpiry(2000, 1000);
		pPart->m_color = GetRandomColor();
		pPart->m_x = pFireworkBase->m_x;
		pPart->m_y = pFireworkBase->m_y;
		pPart->m_dx = (double)pPart->m_x;
		pPart->m_dy = (double)pPart->m_y;
	}
	else
	{
		// Is the fire. Don't have a base (pFireworkBase is NULL)
		pPart->m_x = GetScreenSizeX() / 2;
		pPart->m_y = GetScreenSizeY() - 1;
		pPart->m_color = GetRandomColor();
		pPart->m_explosionRange = (rand() % 100) + 100;
		pPart->m_velY = -(double)(400 + rand() % 400);
		pPart->m_velX = (double)(GetRandomDoubleSigned() * offsetX);
		pPart->m_expireAt = GetRandomExpiry(500, 500);
		pPart->m_dx = (double)pPart->m_x;
		pPart->m_dy = (double)pPart->m_y;
	}
}

void OnExplode(Particle* pPart)
{
	int nParts = rand() % 100 + 100;
	
	for (int i = 0; i < nParts; i++)
	{
		Particle* part = calloc(1, sizeof(Particle));
		SetupParticle(part, pPart, false);
		AddParticle(part);
	}
	
	RemoveParticle(pPart);
}

int g_lastDelta = UPDATE_MS;

void UpdateParticle(Particle* part)
{
	double deltaSec = ((double)g_lastDelta / 1000.0);
	
	part->m_dx = (part->m_dx + deltaSec * part->m_velX);
	part->m_dy = (part->m_dy + deltaSec * part->m_velY);
	part->m_x = (int)part->m_dx;
	part->m_y = (int)part->m_dy;
	
	//part->m_velX *= 0.98f * deltaSec;
	part->m_velY += 10.0f * deltaSec;
	
	if (part->m_expireAt < g_tickCountRightNow)
	{
		if (part->m_bCanExplode)
			OnExplode(part);
		else
			RemoveParticle(part);
	}
	else if (part->m_bCanExplode && part->m_y < (GetScreenSizeY() / 6))
	{
		OnExplode(part);
	}
}

void RenderParticle(Particle* part)
{
	part->m_oldColor = VidReadPixel(part->m_x, part->m_y);
	VidPlotPixel(part->m_x, part->m_y, part->m_color);
}

void UnrenderParticle(Particle* part)
{
	//note: this won't be exactly accurate, but good enough
	VidPlotPixel(part->m_x, part->m_y, part->m_oldColor);
}

void Update()
{
	Particle* part = g_first;
	while (part)
	{
		UpdateParticle(part);
		part = part->m_next;
	}
}

void Unrender()
{
	Particle* part = g_first;
	while (part)
	{
		UnrenderParticle(part);
		part = part->m_next;
	}
}

void Render()
{
	Particle* part = g_first;
	while (part)
	{
		RenderParticle(part);
		part = part->m_next;
	}
}

VBEData* g_pCloneVbeData;
VBEData* VidGetVbeData();

void CloneVbeData()
{
	g_pCloneVbeData = calloc(1, sizeof (VBEData));
	
	VBEData* pMainData = VidGetVbeData();
	g_pCloneVbeData->m_available = true;
	g_pCloneVbeData->m_width  = pMainData->m_width;
	g_pCloneVbeData->m_height = pMainData->m_height;
	g_pCloneVbeData->m_pitch  = pMainData->m_pitch;
	g_pCloneVbeData->m_bitdepth = pMainData->m_bitdepth;
	g_pCloneVbeData->m_dirty    = pMainData->m_dirty;
	g_pCloneVbeData->m_framebuffer32 = pMainData->m_framebuffer32;
	g_pCloneVbeData->m_pitch32 = pMainData->m_pitch32;
	g_pCloneVbeData->m_pitch16 = pMainData->m_pitch16;
	g_pCloneVbeData->m_clipRect = pMainData->m_clipRect;
	
	VidSetVbeData(g_pCloneVbeData);
}

int NsMain (UNUSED int argc, UNUSED char** argv)
{
	VidSetVbeData(NULL); // take over the screen
	
	CloneVbeData();
	
	int spawnNext = 0;
	while (true)
	{
		int start = GetTickCount();
		
		Unrender();
		Update();
		Render();
		
		int end1 = GetTickCount();
		
		int timeout = UPDATE_MS - (end1 - start);
		if (timeout > 0)
			sleep(timeout);
		
		int end = GetTickCount();
		
		g_tickCountRightNow = end;
		
		g_lastDelta = end - start;
		
		if (spawnNext < end)
		{
			int ne = rand() % 2 + 1;
			for (int i = 0; i < ne; i++)
			{
				Particle* explosion = calloc(1, sizeof(Particle));
				SetupParticle(explosion, NULL, true);
				AddParticle(explosion);
			}
			
			spawnNext = GetRandomExpiry(1000, 1000);
		}
	}
	return 0;
}
