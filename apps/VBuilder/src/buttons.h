/*****************************************
		NanoShell Operating System
	      (C) 2023 iProgramInCpp

   Codename V-Builder - Button renderer
******************************************/

#ifndef BUTTONS_H
#define BUTTONS_H

void RenderButtonShapeNoRounding(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle);
void RenderButtonShapeSmall(Rectangle rectb, unsigned colorDark, unsigned colorLight, unsigned colorMiddle);
void RenderButtonShapeSmallInsideOut(Rectangle rectb, unsigned colorLight, unsigned colorDark, unsigned colorMiddle);
void RenderButtonShape(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle);
void RenderCheckbox(Rectangle rect, bool checked, const char * text);

#endif//BUTTONS_H
