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
#define TEXTEDIT_SYNTHILT  (16)

// Text Edit commands. TextInputRequestCommand
enum
{
	// clipboard commands. `parm` is ignored.
	TEDC_PASTE,
	TEDC_CUT,
	TEDC_COPY,
	
	TEDC_INSERT,     // inserts an arbitrary piece of text. Requires a parameter in `parm`.
	
	TEDC_GOTOLINE,   // `parm` is here treated as a pointer to an integer
	TEDC_GOTOOFFSET, // same here
	TEDC_UNDO,
	TEDC_SELECT_ALL,
	TEDC_DELETE,
};

#define IMAGECTL_PAN  (1)
#define IMAGECTL_ZOOM (2)
#define IMAGECTL_PEN  (4)
#define IMAGECTL_FILL (8)

#define TABLEVIEW_NOICONCOLUMN (1)

#define BTNLIST_HASICON (1)
#define BTNLIST_HASSUBS (2)
#define BTNLIST_BIG     (4)

#define BUTTONDARK BUTTON_SHADOW_COLOR
#define BUTTONMIDD BUTTON_MIDDLE_COLOR
#define BUTTONLITE BUTTON_HILITE_COLOR
#define BUTTONMIDC WINDOW_BACKGD_COLOR

// single click activation
#define LISTVIEW_SINGLECLICK (1)
#define LISTVIEW_NOBORDER    (2)

#define SCROLL_BAR_WIDTH 16

#define LINE_NUM_GAP 56
#define RECT(rect,x,y,w,h) do {\
	rect.left = x, rect.top = y, rect.right = x+w, rect.bottom = y+h;\
} while (0)

/**
 * Gets the OnEvent function corresponding to the widget type.
 */
WidgetEventHandler GetWidgetOnEventFunction (int type);

/**
 * Gets a Control object from its combo ID.
 */
Control* GetControlByComboID(Window* pWindow, int comboID);

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
 * Gets the current minimum progress value for a SCROLLBAR control with a certain comboID.
 */
int GetScrollBarMin (Window *pWindow, int comboID);

/**
 * Gets the current maximum progress value for a SCROLLBAR control with a certain comboID.
 */
int GetScrollBarMax (Window *pWindow, int comboID);

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
 * Gets the selected item index from a ListView component.
 */
int GetSelectedIndexList (Window* pWindow, int comboID);

/**
 * Sets the selected item index to a ListView component.
 */
void SetSelectedIndexList (Window* pWindow, int comboID, int index);

/**
 * Clears the items from a ListView component with a certain comboID.
 */
void ResetList (Window* pWindow, int comboID);

/**
 * Sets the data of a list item in a ListView component.
 */
void SetListItemText(Window* pWindow, int comboID, int index, int icon, const char * pText);

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
 * TEXTEDIT_LINENUMS:  Enables line numbers
 * TEXTEDIT_MULTILINE: Allows user to edit multiple lines at once
 * TEXTEDIT_READONLY:  Prevents the user from editing the content.  Good for crash reporting
 * TEXTEDIT_STYLING:   Enables NanoShell styling of the text.  It uses non-typable characters to format text.
 * TEXTEDIT_SYNTHILT:  Enables syntax highlighting for C.
 */
void CtlTextInputUpdateMode (Control *pControl, Window* pWindow); //internal
void TextInputSetMode (Window *pWindow, int comboID, int mode);

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

/**
 * Zooms an image to fill the entire screen width.
 * Only works if IMAGECTL_ZOOM is enabled
 */
void ImageCtlZoomToFill (Window *pWindow, int comboID);

/**
 * Sets the focused control to the one with this comboid.
 */
void SetFocusedControl(Window *pWindow, int comboId);

/**
 * Sets the control with the comboid as disabled.
 */
void SetDisabledControl(Window *pWindow, int comboId, bool bDisabled);

/**
 * Add an element into the table.
 * The 'pText' array is an array of const char pointers that's <column count> columns wide.
 * An icon can be specified, or ICON_NONE for none.
 */
void AddTableRow (Window* pWindow, int comboID, const char* pText[], int optionalIcon);

/**
 * Add a column into the table.
 */
void AddTableColumn(Window* pWindow, int comboID, const char* pText, int width);

/**
 * Gets the row strings from the table.
 * The 'output' array must be at least 'Column Count' in length.
 * Each of the elements, up until the number of columns minus one, will be filled with a const char pointer to the contents of a single row element.
 */
bool GetRowStringsFromTable(Window* pWindow, int comboID, int index, const char * output[]);

/**
 * Removes a single row from a table.
 */
void RemoveRowFromTable(Window* pWindow, int comboID, int elementIndex);

/**
 * Gets the selected element of a table.
 */
int GetSelectedIndexTable(Window* pWindow, int comboID);

/**
 * Sets the selected element of a table.
 */
void SetSelectedIndexTable(Window* pWindow, int comboID, int selectedIndex);

/**
 * Gets the scroll position of a table. This is to be treated as an
 * opaque-ish value.
 */
int GetScrollTable(Window* pWindow, int comboID);

/**
 * Sets the scroll position of a table. This is to be treated as an
 * opaque-ish value.
 */
void SetScrollTable(Window* pWindow, int comboID, int scroll);

/**
 * Clears a table view widget's data.
 */
void ResetTable(Window* pWindow, int comboID);

/**
 * Adds a new tab to a tab widget.
 */
void TabViewAddTab(Window* pWindow, int comboID, int tabID, const char* pTabText, int tabWidth);

/**
 * Clears the tab list of a tab widget.
 */
void TabViewClearTabs(Window* pWindow, int comboID);

/**
 * Removes a tab with a specific tabID from the tab list of a tab widget.
 */
void TabViewRemoveTab(Window* pWindow, int comboID, int tabID);

/**
 * Sets a control's visibility.
 */
void SetControlVisibility(Window* pWindow, int comboID, bool bVisible);

/**
 * Draws the shape of a button.
 */
void RenderButtonShape(Rectangle rect, unsigned colorDark, unsigned colorLight, unsigned colorMiddle);

/**
 * Draws the shape of a small button.
 */
void RenderButtonShapeSmall(Rectangle rectb, unsigned colorDark, unsigned colorLight, unsigned colorMiddle);

/**
 * Sets the progress field of a progress bar.
 */
void ProgBarSetProgress(Window* pWindow, int comboID, int prog);

/**
 * Sets the maximum progress field of a progress bar.
 */
void ProgBarSetMaxProg(Window* pWindow, int comboID, int max_prog);

/**
 * Sets the font of a text input control.
 */
void TextInputSetFont(Window *pWindow, int comboID, unsigned font);

/**
 * Requests a command from a text input control.
 */
void TextInputRequestCommand(Window *pWindow, int comboID, int command, void* parm);

/**
 * TODO
 */
void ComboBoxAddItem(Window* pWindow, int comboID, const char* item, int itemID, int iconID);

#endif//_WIDGET_H
