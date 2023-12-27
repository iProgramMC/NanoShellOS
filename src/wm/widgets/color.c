/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

   Widget library: Color Picker control
******************************************/
#include "../wi.h"

typedef int FixedPoint;

#define BOTTOM_BAR_HEIGHT 60

typedef struct
{
	uint32_t m_SelectedColor; // to be grabbed by the user of this control
	
	FixedPoint m_SelectedHue; // 0-360
	FixedPoint m_SelectedSat; // 0-1
	FixedPoint m_SelectedVal; // 0-1
	
	FixedPoint m_PrevSelectedHue; // 0-360
	FixedPoint m_PrevSelectedSat; // 0-1
	FixedPoint m_PrevSelectedVal; // 0-1
	
	Rectangle m_HVRect; // hue-val 2D picker
	Rectangle m_SRect;  // sat 1D picker
	Rectangle m_CPrevRect;
	Rectangle m_PropRects[6];
	
	int m_SelectedPropBox; // Selected property text box.
}
ColorPickerData;

// decent precision. Higher, you might hit an overflow unless you change FixedPoint to 64-bit
// Lower, you may make the ratios become 0, after which point the rect will just be fully red
static const int FIXED_PREC_LOG = 12;

static FixedPoint ColPick_FPMul(FixedPoint a, FixedPoint b)
{
    return (a * b) >> FIXED_PREC_LOG;
}

static FixedPoint ColPick_FPMulInt(FixedPoint a, int b)
{
    return a * b;
}

static FixedPoint ColPick_FPDivInt(FixedPoint a, int b)
{
    return a / b;
}

static int ColPick_FPRoundDown(FixedPoint a)
{
    return (int)(a >> FIXED_PREC_LOG);
}

#define COLPICK_TO_FP(a) ((FixedPoint)a << FIXED_PREC_LOG)

void HSVToRGB(FixedPoint hue, FixedPoint sat, FixedPoint val, int* red, int* green, int* blue)
{
    if (sat <= 0)
    {
        *red = *green = *blue = ColPick_FPRoundDown(ColPick_FPMulInt(val, 255));
        return;
    }

    if (hue >= COLPICK_TO_FP(360))
    {
        FixedPoint isat = ColPick_FPMul(COLPICK_TO_FP(1) - sat, val);

        *red = ColPick_FPRoundDown(ColPick_FPMulInt(val, 255));
        *green = *blue = ColPick_FPRoundDown(ColPick_FPMulInt(isat, 255));
        return;
    }

    int sector = ColPick_FPRoundDown(ColPick_FPDivInt(hue, 60));
    FixedPoint hue_mod_60 = ColPick_FPDivInt(hue, 60) - COLPICK_TO_FP(sector);
	
	FixedPoint v1, v2, v3;
	v1 = ColPick_FPMul(COLPICK_TO_FP(1) - sat, val);
	v2 = ColPick_FPMul(COLPICK_TO_FP(1) - ColPick_FPMul(sat, hue_mod_60), val);
	v3 = ColPick_FPMul(COLPICK_TO_FP(1) - ColPick_FPMul(sat, COLPICK_TO_FP(1) - hue_mod_60), val);

    switch (sector)
    {
        case 0:
            *red   = ColPick_FPRoundDown(val * 255);
            *green = ColPick_FPRoundDown(v3 * 255);
            *blue  = ColPick_FPRoundDown(v1 * 255);
            return;
        case 1:
            *red   = ColPick_FPRoundDown(v2 * 255);
            *green = ColPick_FPRoundDown(val * 255);
            *blue  = ColPick_FPRoundDown(v1 * 255);
            return;
        case 2:
            *red   = ColPick_FPRoundDown(v1 * 255);
            *green = ColPick_FPRoundDown(val * 255);
            *blue  = ColPick_FPRoundDown(v3 * 255);
            return;
        case 3:
            *red   = ColPick_FPRoundDown(v1 * 255);
            *green = ColPick_FPRoundDown(v2 * 255);
            *blue  = ColPick_FPRoundDown(val * 255);
            return;
        case 4:
            *red   = ColPick_FPRoundDown(v3 * 255);
            *green = ColPick_FPRoundDown(v1 * 255);
            *blue  = ColPick_FPRoundDown(val * 255);
            return;
        default:
            *red   = ColPick_FPRoundDown(val * 255);
            *green = ColPick_FPRoundDown(v1 * 255);
            *blue  = ColPick_FPRoundDown(v2 * 255);
            return;
    }
}

extern VBEData* g_vbeData;

uint32_t HSVToColor(FixedPoint hue, FixedPoint sat, FixedPoint val)
{
	int r=0, g=0, b=0;
	HSVToRGB(hue, sat, val, &r, &g, &b);
	
	// cap it to avoid numeric overflow because of a shortage of precision.
	// Do we need this? probably not, I've checked and I never got inside these ifs.
	// I'll comment them out right now. It speeds up the code, so I'll only add these back when I see issues.
	
	/*
	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;
	*/
	
	uint32_t col = 0xFF000000u;
	col |= b;
	col |= g << 8;
	col |= r << 16;
	
	return col;
}

void WidgetColorPicker_SubDrawHueValRect(Rectangle rect, Rectangle clipRect, FixedPoint saturation)
{
	int c_width  = rect.right - rect.left;
	int c_height = rect.bottom - rect.top;
	
	FixedPoint normY;
	
	// Note! We can't use these, sadly, since then the imprecision
	// would be obvious once you drag the little selection thingy.
	
	// Thankfully, we don't tend to draw the color picker in full screen
	// at all, so it should be fine.....
	
	//FixedPoint ratioX = ColPick_FPDivInt(COLPICK_TO_FP(1), c_width);
	//FixedPoint ratioY = ColPick_FPDivInt(COLPICK_TO_FP(1), c_height);
	
	int startY = 0, endY = 0, startX = 0, endX = 0;
	
	int y = rect.top;
	int sy = 0;
	if (y < 0)
	{
		sy = -y;
		y = 0;
	}
	if (y < clipRect.top)
	{
		int diff = clipRect.top - y;
		y = clipRect.top;
		sy += diff;
	}
	
	normY = ColPick_FPDivInt(COLPICK_TO_FP(sy), c_height);
	
	for (startY = y; y < rect.bottom; y++, sy++, endY = y)
	{
		if (y >= (int)g_vbeData->m_height) break;
		if (y >= clipRect.bottom) break;
		
		int x = rect.left;
		int sx = 0;
		
		if (x < 0)
		{
			sx = -x;
			x = 0;
		}
		if (x < clipRect.left)
		{
			int diff = clipRect.left - x;
			x = clipRect.left;
			sx += diff;
		}
		
		uint32_t* row = &g_vbeData->m_framebuffer32[y * g_vbeData->m_pitch32];
		
		FixedPoint normX = ColPick_FPDivInt(COLPICK_TO_FP(sx), c_width);
		
		for (startX = x; x < rect.right; x++, sx++, endX = x)
		{
			if (x >= (int)g_vbeData->m_width) break;
			if (x >= clipRect.right) break;
			
			row[x] = HSVToColor(ColPick_FPMulInt(normX, 360), saturation, COLPICK_TO_FP(1) - normY);
			
			//normX += ratioX;
			normX = ColPick_FPDivInt(COLPICK_TO_FP(sx), c_width);
		}
		
		//normY += ratioY;
		normY = ColPick_FPDivInt(COLPICK_TO_FP(sy), c_height);
	}
	
	// finally, log this as a dirty rectangle
	DirtyRectLogger(startX, startY, endX - startX, endY - startY);
}

ColorPickerData* ColorPicker_GetData(Control* this)
{
	return (ColorPickerData*)this->m_dataPtr;
}

void WidgetColorPicker_DrawSatPickerRect(Rectangle rect, Rectangle clipRect, FixedPoint hue, FixedPoint val)
{
	// this is simpler.
	int height = rect.bottom - rect.top;
	int y = rect.top, sy = 0;
	
	if (y < clipRect.top)
	{
		int diff = clipRect.top - y;
		y = clipRect.top;
		sy += diff;
	}
	
	for (; y < rect.bottom; y++, sy++)
	{
		if (y > clipRect.bottom) break;
		
		FixedPoint sat = COLPICK_TO_FP(sy) / height;
		
		uint32_t col = HSVToColor(hue, sat, val);
		
		VidDrawHLine(col, rect.left, rect.right - 1, y);
	}
}

void WidgetColorPicker_DrawRightArrow(int left, int x, int y, int top, int bottom)
{
	VidPlotPixel(x + 4, y - 3, WINDOW_TEXT_COLOR_LIGHT);
	VidPlotPixel(x + 4, y + 3, WINDOW_TEXT_COLOR_LIGHT);
	
	VidDrawVLine(WINDOW_TEXT_COLOR, y - 5, y + 5, x + 5);
	VidDrawLine (WINDOW_TEXT_COLOR, x, y, x + 5, y - 5);
	VidDrawLine (WINDOW_TEXT_COLOR, x, y, x + 5, y + 5);
	
	VidDrawHLine(WINDOW_TEXT_COLOR_LIGHT, x + 3, x + 4, y - 2);
	VidDrawHLine(WINDOW_TEXT_COLOR_LIGHT, x + 3, x + 4, y + 2);
	VidDrawHLine(WINDOW_TEXT_COLOR_LIGHT, x + 2, x + 4, y - 1);
	VidDrawHLine(WINDOW_TEXT_COLOR_LIGHT, x + 2, x + 4, y + 1);
	VidDrawHLine(WINDOW_TEXT_COLOR_LIGHT, x + 1, x + 4, y);
	VidDrawHLine(WINDOW_TEXT_COLOR_LIGHT, left, x - 3, y);
	
	if (y - 1 >= top)
		VidDrawHLine(WINDOW_TEXT_COLOR, left, x - 3, y - 1);
	if (y < bottom - 1)
		VidDrawHLine(WINDOW_TEXT_COLOR, left, x - 3, y + 1);
}

void WidgetColorPicker_DrawHVHighlight(int x, int y, int top, int bottom)
{
	int mid = (top + bottom) / 2;
	
	uint32_t color = 0x000000;
	if (y > mid)
		color = 0xFFFFFF;
	
	VidDrawHLine(color, x - 1, x + 1, y - 2);
	VidDrawHLine(color, x - 1, x + 1, y + 2);
	VidDrawVLine(color, y - 1, y + 1, x - 2);
	VidDrawVLine(color, y - 1, y + 1, x + 2);
}

void WidgetColorPicker_GetHVSelectionXY(Control* this, int* x, int* y)
{
	ColorPickerData* pData = ColorPicker_GetData(this);
	
	Rectangle hvRect = pData->m_HVRect;
	
	int Width  = hvRect.right - hvRect.left;
	int Height = hvRect.bottom - hvRect.top;
	
	*x = ColPick_FPRoundDown(ColPick_FPDivInt(pData->m_SelectedHue, 360) * Width);
	*y = ColPick_FPRoundDown((COLPICK_TO_FP(1) - pData->m_SelectedVal) * Height);
}

void WidgetColorPicker_GetHueValByXY(Rectangle rect, FixedPoint* hue, FixedPoint* val, int x, int y)
{
	int c_width  = rect.right - rect.left;
	int c_height = rect.bottom - rect.top;
	
	if (c_width  == 0) c_width  = 1;
	if (c_height == 0) c_height = 1;
	
	FixedPoint
	normX = ColPick_FPDivInt(COLPICK_TO_FP(x), c_width),
	normY = ColPick_FPDivInt(COLPICK_TO_FP(y), c_height);
	
	*hue = normX * 360;
	*val = COLPICK_TO_FP(1) - normY;
}

int WidgetColorPicker_GetSatSelectionY(Control* this)
{
	ColorPickerData* pData = ColorPicker_GetData(this);
	
	Rectangle sRect = pData->m_SRect;
	
	int y = ColPick_FPRoundDown((pData->m_SelectedSat) * (sRect.bottom - sRect.top));
	
	return y;
}

FixedPoint WidgetColorPicker_GetSatByY(Control* this, int y)
{
	ColorPickerData* pData = ColorPicker_GetData(this);
	
	Rectangle sRect = pData->m_SRect;
	int c_height = sRect.bottom - sRect.top;
	if (c_height == 0) c_height = 1;
	
	FixedPoint fp = (COLPICK_TO_FP(y) / c_height);
	
	return fp;
}

void WidgetColorPicker_DrawHueValRect(Control* this)
{
	ColorPickerData* pData = ColorPicker_GetData(this);
	
	Rectangle hsPickerRect = pData->m_HVRect;
	
	WidgetColorPicker_SubDrawHueValRect(hsPickerRect, hsPickerRect, pData->m_SelectedSat);
	
	int x = 0, y = 0;
	WidgetColorPicker_GetHVSelectionXY(this, &x, &y);
	
	// to avoid it drawing the highlight out of bounds and us having to redraw the edge
	VidSetClipRect(&hsPickerRect);
	WidgetColorPicker_DrawHVHighlight(hsPickerRect.left + x, hsPickerRect.top + y, hsPickerRect.top, hsPickerRect.bottom);
	VidSetClipRect(NULL);
}

void WidgetColorPicker_DrawSatRect(Control* this)
{
	ColorPickerData* pData = ColorPicker_GetData(this);
	
	Rectangle valPickerRect = pData->m_SRect;
	
	WidgetColorPicker_DrawSatPickerRect(valPickerRect, valPickerRect, pData->m_SelectedHue, pData->m_SelectedVal);
	
	int y = WidgetColorPicker_GetSatSelectionY(this);
	if (y > valPickerRect.bottom - 1)
		y = valPickerRect.bottom - 1;
	
	WidgetColorPicker_DrawRightArrow(valPickerRect.left, valPickerRect.right + 2, y + valPickerRect.top, valPickerRect.top, valPickerRect.bottom);
}

void WidgetColorPicker_DrawColPrev(Control* this)
{
	ColorPickerData* pData = ColorPicker_GetData(this);
	
	Rectangle cRect = pData->m_CPrevRect;
	
	uint32_t color = HSVToColor(pData->m_SelectedHue, pData->m_SelectedSat, pData->m_SelectedVal);
	VidFillRect(color, cRect.left, cRect.top, cRect.right - 1, cRect.bottom - 1);
	
	pData->m_SelectedColor = color;
}

int WidgetColorPicker_GetShownValue(Control* this, int propName)
{
	int valueShown = -1;
	ColorPickerData* pData = ColorPicker_GetData(this);
	
	if (propName >= 3)
	{
		union
		{
			uint32_t col;
			struct
			{
				unsigned b : 8;
				unsigned g : 8;
				unsigned r : 8;
			}
			x;
		}
		y;
		
		y.col = pData->m_SelectedColor;
		
		switch (propName)
		{
			case 3: // red
				valueShown = y.x.r;
				break;
			case 4: // green
				valueShown = y.x.g;
				break;
			case 5: // blue
				valueShown = y.x.b;
				break;
		}
	}
	else
	{
		switch (propName)
		{
			case 0: // hue
				valueShown = ColPick_FPRoundDown(pData->m_SelectedHue);
				break;
			case 1: // sat
				valueShown = ColPick_FPRoundDown(pData->m_SelectedSat * 256);
				break;
			case 2: // val
				valueShown = ColPick_FPRoundDown(pData->m_SelectedVal * 256);
				break;
		}
	}
	
	return valueShown;
}

void WidgetColorPicker_DrawPropertyField(Control *this, int propName)
{
	if  (propName < 0)  return;
	
	ColorPickerData* pData = ColorPicker_GetData(this);
	Rectangle cRect = pData->m_PropRects[propName];
	
	uint32_t color = WINDOW_TEXT_COLOR_LIGHT;
	if (pData->m_SelectedPropBox == propName)
		color = TABLE_VIEW_ALT_ROW_COLOR;
	
	VidFillRect(color, cRect.left, cRect.top, cRect.right - 1, cRect.bottom - 1);
	
	cRect.left++;
	cRect.top++;
	
	int valueShown = WidgetColorPicker_GetShownValue(this, propName);
	
	char buffer[10];
	snprintf(buffer, sizeof buffer, "%d", valueShown);
	
	VidDrawText(buffer, cRect, TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
}

void WidgetColorPicker_SetHueVal(Control* this, FixedPoint hue, FixedPoint val)
{
	ColorPickerData* pData = ColorPicker_GetData(this);
	
	Rectangle hvRect = pData->m_HVRect;
	
	// undraw the old selection cursor
	int x = 0, y = 0;
	WidgetColorPicker_GetHVSelectionXY(this, &x, &y);
	
	Rectangle rect = { x - 2 + hvRect.left, y - 2 + hvRect.top, x + 3 + hvRect.left, y + 3 + hvRect.top };
	if (rect.left < hvRect.left)
		rect.left = hvRect.left;
	if (rect.top  < hvRect.top)
		rect.top  = hvRect.top;
	if (rect.right > hvRect.right)
		rect.right = hvRect.right;
	if (rect.bottom > hvRect.bottom)
		rect.bottom = hvRect.bottom;
	
	VidSetClipRect(&rect);
	
	// draw the HV rect where the old cursor was
	WidgetColorPicker_SubDrawHueValRect(hvRect, rect, pData->m_SelectedSat);
	
	VidSetClipRect(&hvRect);
	
	// set the new values.
	pData->m_SelectedHue = hue;
	pData->m_SelectedVal = val;
	
	// draw a new selection cursor
	WidgetColorPicker_GetHVSelectionXY(this, &x, &y);
	
	WidgetColorPicker_DrawHVHighlight(hvRect.left + x, hvRect.top + y, hvRect.top, hvRect.bottom);
	
	VidSetClipRect(NULL);
}

void WidgetColorPicker_SetSat(Control* this, FixedPoint sat)
{
	ColorPickerData* pData = ColorPicker_GetData(this);
	
	Rectangle sRect = pData->m_SRect;
	
	// undraw the old selection cursor
	int y = WidgetColorPicker_GetSatSelectionY(this);
	
	Rectangle xRect = sRect;
	xRect.top    = sRect.top + y - 1;
	xRect.bottom = sRect.top + y + 2;
	if (xRect.top < sRect.top)
		xRect.top = sRect.top;
	if (xRect.bottom > sRect.bottom)
		xRect.bottom = sRect.bottom;
	
	VidSetClipRect(&xRect);
	WidgetColorPicker_DrawSatPickerRect(sRect, xRect, pData->m_SelectedHue, pData->m_SelectedVal);
	VidSetClipRect(NULL);
	
	xRect = sRect;
	xRect.left   = xRect.right + 2;
	xRect.right += 7;
	xRect.top    = sRect.top + y - 5;
	xRect.bottom = sRect.top + y + 6;
	VidFillRectangle(WINDOW_BACKGD_COLOR, xRect);
	
	pData->m_SelectedSat = sat;
	
	y = WidgetColorPicker_GetSatSelectionY(this);
	if (y > sRect.bottom - 1)
		y = sRect.bottom - 1;
	
	WidgetColorPicker_DrawRightArrow(sRect.left, sRect.right + 2, y + sRect.top, sRect.top, sRect.bottom);
}

void WidgetColorPicker_SetShownValue(Control* this, int propName, int value)
{
	ColorPickerData* pData = ColorPicker_GetData(this);
	
	switch (propName)
	{
		// TODO: RGB to HSV.
		case 1:
		{
			if (value < 0) value = 0;
			if (value >= 256) value = 256;
			FixedPoint sat = COLPICK_TO_FP(value) / 256;
			
			WidgetColorPicker_SetSat(this, sat);
			
			WidgetColorPicker_DrawHueValRect(this);
			WidgetColorPicker_DrawColPrev(this);
			
			pData->m_PrevSelectedSat = sat;
			
			WidgetColorPicker_DrawPropertyField(this, propName);
			
			for (int i = 3; i < 6; i++)
				WidgetColorPicker_DrawPropertyField(this, i);
			
			break;
		}
		case 0:
		case 2:
		{
			// set the hue
			FixedPoint hue = pData->m_SelectedHue, val = pData->m_SelectedVal;
			if (propName == 0)
			{
				if (value < 0) value = 0;
				if (value >= 360) value = 360;
				
				hue = COLPICK_TO_FP(value);
			}
			else
			{
				if (value < 0) value = 0;
				if (value >= 256) value = 256;
				
				val = COLPICK_TO_FP(value) / 256;
			}
			
			WidgetColorPicker_SetHueVal(this, hue, val);
			
			WidgetColorPicker_DrawSatRect(this);
			WidgetColorPicker_DrawColPrev(this);
			
			pData->m_PrevSelectedHue = hue;
			pData->m_PrevSelectedVal = val;
			
			WidgetColorPicker_DrawPropertyField(this, propName);
			
			for (int i = 3; i < 6; i++)
				WidgetColorPicker_DrawPropertyField(this, i);
			
			break;
		}
	}
}

bool WidgetColorPicker_OnEvent(Control* this, int eventType, UNUSED long parm1, UNUSED long parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_CREATE:
		{
			ColorPickerData* pData = MmAllocate(sizeof(ColorPickerData));
			memset(pData, 0, sizeof *pData);
			
			this->m_dataPtr = pData;
			
			pData->m_SelectedSat = COLPICK_TO_FP(1); // 100% saturated
			pData->m_SelectedVal = COLPICK_TO_FP(1); // 100% value
			
			break;
		}
		case EVENT_DESTROY:
		{
			if (this->m_dataPtr)
				MmFree(this->m_dataPtr);
			
			this->m_dataPtr = NULL;
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			ColorPickerData* pData = ColorPicker_GetData(this);
			if (pData->m_SelectedHue != pData->m_PrevSelectedHue || pData->m_SelectedVal != pData->m_PrevSelectedVal || pData->m_SelectedSat != pData->m_PrevSelectedSat)
			{
				WidgetColorPicker_DrawColPrev(this);
				
				for (int i = 0; i < 6; i++)
					WidgetColorPicker_DrawPropertyField(this, i);
			}
			if (pData->m_SelectedHue != pData->m_PrevSelectedHue || pData->m_SelectedVal != pData->m_PrevSelectedVal)
			{
				pData->m_PrevSelectedHue = pData->m_SelectedHue;
				pData->m_PrevSelectedVal = pData->m_SelectedVal;
				
				WidgetColorPicker_DrawSatRect(this);
			}
			if (pData->m_SelectedSat != pData->m_PrevSelectedSat)
			{
				pData->m_PrevSelectedSat = pData->m_SelectedSat;
				
				WidgetColorPicker_DrawHueValRect(this);
			}
			
			break;
		}
		case EVENT_CLICKCURSOR:
		{
			ColorPickerData* pData = ColorPicker_GetData(this);
			Point p = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			Rectangle hvRect = pData->m_HVRect;
			
			if (RectangleContains(&hvRect, &p))
			{
				p.x -= hvRect.left;
				p.y -= hvRect.top;
				
				FixedPoint selectedHue = 0, selectedVal = 0;
				
				WidgetColorPicker_GetHueValByXY(hvRect, &selectedHue, &selectedVal, p.x, p.y);
				
				WidgetColorPicker_SetHueVal(this, selectedHue, selectedVal);
			}
			
			Rectangle sRect = pData->m_SRect;
			sRect.top -= 8;
			sRect.bottom += 8;
			
			if (RectangleContains(&sRect, &p))
			{
				sRect.top += 8;
				sRect.bottom -= 8;
				p.x -= sRect.left;
				p.y -= sRect.top;
				if (p.y < 0)
					p.y = 0;
				if (p.y > sRect.bottom - sRect.top - 1)
					p.y = sRect.bottom - sRect.top - 1;
				
				FixedPoint sat = WidgetColorPicker_GetSatByY(this, p.y);
				
				WidgetColorPicker_SetSat(this, sat);
			}
			
			bool bChangedBox = false;
			
			for (int i = 0; i < 6; i++)
			{
				if (RectangleContains(&pData->m_PropRects[i], &p))
				{
					bChangedBox = true;
					
					// select this box and unselect the old one.
					int oldSelBox = pData->m_SelectedPropBox;
					
					// Set the new selected box.
					pData->m_SelectedPropBox = i;
					
					// repaint the old and new box.
					WidgetColorPicker_DrawPropertyField(this, oldSelBox);
					WidgetColorPicker_DrawPropertyField(this, i);
					
					break;
				}
			}
			
			if (!bChangedBox)
			{
				int oldSelBox = pData->m_SelectedPropBox;
				pData->m_SelectedPropBox = -1;
				WidgetColorPicker_DrawPropertyField(this, oldSelBox);
			}
			
			break;
		}
		case EVENT_KEYPRESS:
		{
			ColorPickerData* pData = ColorPicker_GetData(this);
			
			if (parm1 != '\x7F' && (parm1 < '0' || parm1 > '9')) break; // ignore anything other than those
			
			if (pData->m_SelectedPropBox < 0 || pData->m_SelectedPropBox >= 6) break;
			
			int val = WidgetColorPicker_GetShownValue(this, pData->m_SelectedPropBox);
			
			if (parm1 == '\x7F')
				// remove the last digit
				val /= 10;
			else
				// add a new digit
				val = val * 10 + (parm1 - '0');
			
			WidgetColorPicker_SetShownValue(this, pData->m_SelectedPropBox, val);
			
			break;
		}
		case EVENT_PAINT:
		{
			ColorPickerData* pData = ColorPicker_GetData(this);
			
			VidSetClipRect(&this->m_rect);
			
			Rectangle hsPickerRect = this->m_rect;
			hsPickerRect.top    += 6;
			hsPickerRect.right  -= BOTTOM_BAR_HEIGHT;
			hsPickerRect.bottom -= BOTTOM_BAR_HEIGHT + 6;
			Rectangle valPickerRect = this->m_rect;
			valPickerRect.top    += 6;
			valPickerRect.left    = valPickerRect.right - 30;
			valPickerRect.right  -= 7;
			valPickerRect.bottom -= BOTTOM_BAR_HEIGHT + 6;
			Rectangle colPrevRect   = this->m_rect;
			colPrevRect.left = colPrevRect.right  - 40;
			colPrevRect.top  = colPrevRect.bottom - 30;
			
			int ms1 = GetTickCount();
			
			// in case we also happen to be drawing some arrows or something
			Rectangle emptyGapRect = this->m_rect;
			emptyGapRect.left = hsPickerRect.right;
			emptyGapRect.right = valPickerRect.left - 1;
			emptyGapRect.bottom -= 41;
			VidFillRectangle(WINDOW_BACKGD_COLOR, emptyGapRect);
			emptyGapRect = this->m_rect;
			emptyGapRect.top = valPickerRect.bottom;
			emptyGapRect.bottom--;
			emptyGapRect.right--;
			VidFillRectangle(WINDOW_BACKGD_COLOR, emptyGapRect);
			emptyGapRect = this->m_rect;
			emptyGapRect.left = valPickerRect.right;
			emptyGapRect.right--;
			VidFillRectangle(WINDOW_BACKGD_COLOR, emptyGapRect);
			
			DrawEdge(hsPickerRect,  DRE_SUNKEN, 0);
			DrawEdge(valPickerRect, DRE_SUNKEN, 0);
			DrawEdge(colPrevRect,   DRE_SUNKEN, 0);
			
			hsPickerRect.left += 2;
			hsPickerRect.top += 2;
			hsPickerRect.right -= 2;
			hsPickerRect.bottom -= 2;
			valPickerRect.left += 2;
			valPickerRect.top += 2;
			valPickerRect.right -= 2;
			valPickerRect.bottom -= 2;
			colPrevRect.left += 2;
			colPrevRect.top += 2;
			colPrevRect.right -= 2;
			colPrevRect.bottom -= 2;
			
			pData->m_HVRect    = hsPickerRect;
			pData->m_SRect     = valPickerRect;
			pData->m_CPrevRect = colPrevRect;
			
			static const char* const arr1[] = { "(0-360) Hue", "(0-256) Sat", "(0-256) Lum",   "(0-255) Red", "(0-255) Green", "(0-255) Blue" };
			
			int widthCol1 = 80;
			int widthCol2 = 40;
			int widthCol3 = 80;
			int widthCol4 = 40;
			
			// generate the first three property rectangles for the hue, sat, val
			for (int i = 0; i < 3; i++)
			{
				Rectangle rect = this->m_rect;
				rect.left += widthCol1;
				rect.top = this->m_rect.bottom + (i - 3) * (BOTTOM_BAR_HEIGHT / 3);
				rect.right = rect.left + widthCol2;
				rect.bottom = rect.top + (BOTTOM_BAR_HEIGHT / 3);
				
				DrawEdge(rect, DRE_SUNKEN, 0);
				rect.left   += 2;
				rect.top    += 2;
				rect.right  -= 2;
				rect.bottom -= 2;
				
				// set the rect
				pData->m_PropRects[i] = rect;
				// draw the initial text too
				Rectangle textRect = rect;
				textRect.left -= widthCol1;
				textRect.right -= widthCol2 - 1;
				textRect.bottom--;
				
				VidDrawText(arr1[i], textRect, TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
				
				WidgetColorPicker_DrawPropertyField(this, i);
			}
			
			// generate the next three property rectangles for the red, green, blue
			for (int i = 0; i < 3; i++)
			{
				Rectangle rect = this->m_rect;
				rect.left += widthCol1 + widthCol2 + widthCol3;
				rect.top = this->m_rect.bottom + (i - 3) * (BOTTOM_BAR_HEIGHT / 3);
				rect.right = rect.left + widthCol4;
				rect.bottom = rect.top + (BOTTOM_BAR_HEIGHT / 3);
				
				DrawEdge(rect, DRE_SUNKEN, 0);
				rect.left   += 2;
				rect.top    += 2;
				rect.right  -= 2;
				rect.bottom -= 2;
				
				// set the rect
				pData->m_PropRects[i + 3] = rect;
				// draw the initial text too
				Rectangle textRect = rect;
				textRect.left -= widthCol3;
				textRect.right -= widthCol4;
				textRect.bottom--;
				
				VidDrawText(arr1[i + 3], textRect, TEXTSTYLE_VCENTERED, WINDOW_TEXT_COLOR, TRANSPARENT);
				
				WidgetColorPicker_DrawPropertyField(this, i + 3);
			}
			
			
			
			WidgetColorPicker_DrawHueValRect(this);
			WidgetColorPicker_DrawSatRect(this);
			WidgetColorPicker_DrawColPrev(this);
			
			int ms2 = GetTickCount();
			
			SLogMsg("ColorPicker: Repaint took %d ms", ms2-ms1);
			
			VidSetClipRect(NULL);
			
			break;
		}
	}
	
	return false;
}


