/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

          Window Animation module
******************************************/
#include "wi.h"

// Window effects when you maximize/minimize a window
#define EFFECT_PRECISION 1000

bool g_EffectRunning;
int  g_NextEffectUpdateIn = 0;
char g_EffectText[1000];

Rectangle g_EffectDest, g_EffectSrc;
Rectangle g_EffectStep;

void RefreshPixels(int oldX, int oldY, int oldWidth, int oldHeight);
void KillEffect()
{
	if (!g_EffectRunning) return;
	RefreshPixels (g_EffectSrc.left / EFFECT_PRECISION, g_EffectSrc.top / EFFECT_PRECISION, (g_EffectSrc.right - g_EffectSrc.left) / EFFECT_PRECISION + 1, (g_EffectSrc.bottom - g_EffectSrc.top) / EFFECT_PRECISION + 1);
	g_EffectRunning = false;
}
void RunOneEffectFrame()
{
	if (!g_EffectRunning) return;
	if (g_NextEffectUpdateIn < GetTickCount())
	{
		VBEData data = g_mainScreenVBEData;
		VidSetVBEData(&data); // Hack to avoid it also drawing on the clone
		
		g_NextEffectUpdateIn += 10;
		
		int l,t,w,h;
		l = g_EffectSrc.left / EFFECT_PRECISION,
		t = g_EffectSrc.top  / EFFECT_PRECISION,
		w = (g_EffectSrc.right - g_EffectSrc.left) / EFFECT_PRECISION,
		h = (g_EffectSrc.bottom - g_EffectSrc.top) / EFFECT_PRECISION;
		
		RefreshPixels (l,t,w + 2,h + 2);
		
		// Update
		g_EffectSrc.left    += g_EffectStep.left;
		g_EffectSrc.top     += g_EffectStep.top;
		g_EffectSrc.right   += g_EffectStep.right;
		g_EffectSrc.bottom  += g_EffectStep.bottom;
		
		// Do some boring checks if we've skipped the destrect
		#define EFFSTEPCHECK(dir) \
		if (g_EffectStep.dir < 0)\
		{\
			if (g_EffectSrc.dir < g_EffectDest.dir)\
			{\
				g_EffectSrc.dir = g_EffectDest.dir;\
				g_EffectStep.dir = 0;\
			}\
		}\
		else if (g_EffectStep.dir > 0)\
		{\
			if (g_EffectSrc.dir > g_EffectDest.dir)\
			{\
				g_EffectSrc.dir = g_EffectDest.dir;\
				g_EffectStep.dir = 0;\
			}\
		}
		
		EFFSTEPCHECK(left)
		EFFSTEPCHECK(top)
		EFFSTEPCHECK(right)
		EFFSTEPCHECK(bottom)
		
		// If all effect steps are zero, is there any point in continuing ?
		if (g_EffectStep.left == 0 && g_EffectStep.top == 0 && g_EffectStep.right == 0 && g_EffectStep.bottom == 0)
		{
			KillEffect();
			return;
		}
		
		// Render the effect now
		Rectangle effectSrc = g_EffectSrc;
		effectSrc.left   /= EFFECT_PRECISION;
		effectSrc.top    /= EFFECT_PRECISION;
		effectSrc.right  /= EFFECT_PRECISION;
		effectSrc.bottom /= EFFECT_PRECISION;
		VidSetClipRect(&effectSrc);
		
		VidFillRectHGradient(
			WINDOW_TITLE_ACTIVE_COLOR,
			WINDOW_TITLE_ACTIVE_COLOR_B,
			g_EffectSrc.left   / EFFECT_PRECISION,
			g_EffectSrc.top    / EFFECT_PRECISION,
			g_EffectSrc.right  / EFFECT_PRECISION - 1,
			g_EffectSrc.bottom / EFFECT_PRECISION - 1
		);
		
		VidSetFont(TITLE_BAR_FONT);
		VidDrawText(g_EffectText, effectSrc, TEXTSTYLE_VCENTERED | TEXTSTYLE_HCENTERED, FLAGS_TOO(TEXT_RENDER_BOLD, WINDOW_TITLE_TEXT_COLOR), TRANSPARENT);
		VidSetFont(SYSTEM_FONT);
		
		VidSetVBEData(NULL);
	}
}
void CreateMovingRectangleEffect(Rectangle src, Rectangle dest, const char* text)
{
	VBEData* pOld = VidSetVBEData(NULL);
	KillEffect();
	g_EffectRunning = true;
	g_EffectDest = dest;
	g_EffectDest.left   *= EFFECT_PRECISION;
	g_EffectDest.top    *= EFFECT_PRECISION;
	g_EffectDest.right  *= EFFECT_PRECISION;
	g_EffectDest.bottom *= EFFECT_PRECISION;
	g_EffectSrc  = src;
	g_EffectSrc.left   *= EFFECT_PRECISION;
	g_EffectSrc.top    *= EFFECT_PRECISION;
	g_EffectSrc.right  *= EFFECT_PRECISION;
	g_EffectSrc.bottom *= EFFECT_PRECISION;
	
	// Decide on what stepping variables we should have
	g_EffectStep.left   = (dest.left   - src.left  ) * EFFECT_PRECISION / 16;
	g_EffectStep.top    = (dest.top    - src.top   ) * EFFECT_PRECISION / 16;
	g_EffectStep.right  = (dest.right  - src.right ) * EFFECT_PRECISION / 16;
	g_EffectStep.bottom = (dest.bottom - src.bottom) * EFFECT_PRECISION / 16;
	
	g_NextEffectUpdateIn = GetTickCount();
	
	int sl = strlen (text);
	if (sl > 999) sl = 999;
	memcpy(g_EffectText, text, sl + 1);
	g_EffectText[sl] = 0;
	
	VidSetVBEData(pOld);
}
