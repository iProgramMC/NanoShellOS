/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

      Shell widget module headerfile
******************************************/
#ifndef _WIDGET_H
#define _WIDGET_H

#include<window.h>

#define TEXTEDIT_MULTILINE (1)
#define TEXTEDIT_LINENUMS  (2)
#define TEXTEDIT_READONLY  (4)
#define TEXTEDIT_STYLING   (8)

#define IMAGECTL_PAN  (1)
#define IMAGECTL_ZOOM (2)
#define IMAGECTL_PEN  (4)
#define IMAGECTL_FILL (8)

/**
 * Gets the OnEvent function corresponding to the widget type.
 */
WidgetEventHandler GetWidgetOnEventFunction (int type);

/**
 * Sets the minimum value for a SCROLLBAR control with a certain comboID.
 */
void SetScrollBarMin (Window *pWindow, int comboID, int min);

/**
 * Sets the maximum value for a SCROLLBAR control with a certain comboID.
 */
void SetScrollBarMax (Window *pWindow, int comboID, int max);

/**
 * Sets the current progress value for a SCROLLBAR control with a certain comboID.
 */
void SetScrollBarPos (Window *pWindow, int comboID, int pos);

/**
 * Gets the current progress value for a SCROLLBAR control with a certain comboID.
 */
int GetScrollBarPos (Window *pWindow, int comboID);

/**
 * Adds an element to a ListView component with a certain comboID.
 */
void AddElementToList (Window* pWindow, int comboID, const char* pText, int optionalIcon);

/**
 * Gets an element's string contents from a ListView component with a certain comboID.
 */
const char* GetElementStringFromList (Window* pWindow, int comboID, int index);

/**
 * Removes an element from a ListView component with a certain comboID.
 */
void RemoveElementFromList (Window* pWindow, int comboID, int elemIndex);

/**
 * Clears the items from a ListView component with a certain comboID.
 */
void ResetList (Window* pWindow, int comboID);

/**
 * Changes the text of any component with text with a certain comboID.
 */
void SetLabelText (Window *pWindow, int comboID, const char* pText);

/**
 * Changes the icon of an icon component with a certain comboID.
 */
void SetIcon (Window *pWindow, int comboID, int icon);

/**
 * Changes the text of a TEXTHUGE with text with a certain comboID.
 */
void SetHugeLabelText (Window *pWindow, int comboID, const char* pText);

/**
 * Changes the text of a TEXTINPUT with text with a certain comboID.
 */
void SetTextInputText(Window* pWindow, int comboID, const char* pText);

/**
 * Works on the control with the comboID of 'menuBarControlId'.
 * To that control, it adds a menu item with the comboID of 'comboIdAs' to the menu item with the comboID of 'comboIdTo'.
 */
void AddMenuBarItem (Window* pWindow, int menuBarControlId, int comboIdTo, int comboIdAs, const char* pText);

/**
 * Checks if the text has been changed in a TextInput control.  Returns false if the control is not found.
 */
bool TextInputQueryDirtyFlag(Window* pWindow, int comboID);

/**
 * Clears the dirty flag in a TextInput control.
 */
void TextInputClearDirtyFlag(Window* pWindow, int comboID);

/**
 * Gets the checked state in a Checkbox control.
 *
 * If the control does not exist, this returns false.
 */
bool CheckboxGetChecked(Window* pWindow, int comboID);

/**
 * Sets the checked state in a Checkbox control.
 */
void CheckboxSetChecked(Window* pWindow, int comboID, bool checked);

/**
 * Gets the raw text of a TextInput control.
 */
const char* TextInputGetRawText(Window* pWindow, int comboID);

#endif//_WIDGET_H
