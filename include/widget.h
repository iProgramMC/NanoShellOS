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
 * Sets the event handler of a control with a combo ID.
 *
 * This calls the EVENT_DESTROY of the old handler, and calls
 * EVENT_CREATE of the new handler.
 *
 * While this function makes sure everything in the back end is safe, the
 * caller shall also do their part by ensuring the handler keeps existing
 * until the control is removed, or the window is destroyed.
 *
 * For information on how to define a widget event handler, check the
 * definition of the `WidgetEventHandler`.
 */
void SetWidgetEventHandler (Window *pWindow, int comboID, WidgetEventHandler handler);

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
 * Adding to comboIdTo=0 adds to the main list of items.
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

/**
 * Sets the widget mode in an image control.
 *
 * The mode is a bit set.  You can use one or more of the following flags:
 * IMAGECTL_PAN:  Allows user to pan around the image.
 * IMAGECTL_ZOOM: Allows user to zoom into the image. (TODO)
 * IMAGECTL_PEN:  Allows user to sketch onto the image using the color specified
 *                using SetImageCtlColor.
 * IMAGECTL_FILL: Allows user to fill in a portion of the image using
 *                the color specified using SetImageCtlColor.
 */
void SetImageCtlMode (Window *pWindow, int comboID, int mode);

/**
 * Sets the pen color in an image control
 */
void SetImageCtlColor (Window *pWindow, int comboID, uint32_t color);

/**
 * Sets the current image.  This works the same way as creating a new
 * image control does (the pImage gets cloned for local use and the 
 * user still owns the pointer).
 */
void SetImageCtlCurrentImage (Window *pWindow, int comboID, Image* pImage);

/**
 * Gets the current image of an image control.  The caller may not later
 * free the image, as it is owned by the widget.  However they may clone
 * it for later use.
 */
Image* GetImageCtlCurrentImage (Window *pWindow, int comboID);

#endif//_WIDGET_H
