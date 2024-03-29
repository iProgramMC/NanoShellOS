/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Widget library: Table view control
******************************************/
#include "../wi.h"

// NOTE: Damn, this is a pretty heavy control. 144 KB for 34 entries.
// This is probably a sign we need to optimize the memory allocator for sub-page-size allocations!

#define TABLE_ITEM_HEIGHT (TITLE_BAR_HEIGHT)

static void TableAddColumn(TableViewData* this, const char * text, int width, bool bHandlesNumbers)
{
	if (this->m_column_count + 1 > this->m_column_capacity)
	{
		// expand
		if (this->m_column_capacity == 0)
			this->m_column_capacity  = 4;
		TableViewColumn * newData = MmReAllocate(this->m_pColumnData, this->m_column_capacity * 2 * sizeof(TableViewColumn));
		if (!newData)
		{
			SLogMsg("Could not add column '%s' to table.", text);
			return;
		}
		
		this->m_column_capacity *= 2;
		this->m_pColumnData = newData;
	}
	
	TableViewColumn* newColumn = &this->m_pColumnData[this->m_column_count++];
	
	memset(newColumn, 0, sizeof *newColumn);
	
	strncpy(newColumn->m_text, text, MAX_COLUMN_LENGTH);
	newColumn->m_text[MAX_COLUMN_LENGTH - 1] = 0;
	
	newColumn->m_sort_order = 0;
	newColumn->m_hovered    = newColumn->m_clicked = false;
	newColumn->m_width      = width;
	newColumn->m_sortMode   = TABLE_SORT_NEUTRAL;
	
	newColumn->m_bNumbers = bHandlesNumbers;
	
	// For each row, re-allocate its item count.
	for (int i = 0; i < this->m_row_count; i++)
	{
		// TODO we're doing leaps of faith right now, maybe we shouldn't!
		// Ideally you'd set up the columns first, though.
		this->m_pRowData[i].m_items = MmReAllocate(this->m_pRowData[i].m_items, sizeof(TableViewItem) * this->m_column_count);
		this->m_pRowData[i].m_items[this->m_column_count - 1].m_text[0] = 0; // reset the text! Don't leave it uninitialised like that
	}
}

static TableViewRow* TableAddRow(TableViewData* this, int icon, const char * data[])
{
	if (this->m_row_count + 1 > this->m_row_capacity)
	{
		// expand
		if (this->m_row_capacity == 0)
			this->m_row_capacity  = 4;
		TableViewRow * newData = MmReAllocate(this->m_pRowData, this->m_row_capacity * 2 * sizeof(TableViewRow));
		if (!newData)
		{
			SLogMsg("Could not add a row to a table.");
			return NULL;
		}
		
		this->m_row_capacity *= 2;
		this->m_pRowData = newData;
	}
	
	TableViewRow* newRow = &this->m_pRowData[this->m_row_count++];
	memset(newRow, 0, sizeof *newRow);
	
	newRow->m_icon = icon;
	
	if (this->m_column_count > 0)
	{
		newRow->m_items = MmAllocate(sizeof(TableViewItem) * this->m_column_count);
		for (int i = 0; i < this->m_column_count; i++)
		{
			strncpy(newRow->m_items[i].m_text, data[i], sizeof newRow->m_items[i].m_text);
			newRow->m_items[i].m_text[sizeof newRow->m_items[i].m_text - 1] = 0;
		}
	}
	
	return newRow;
}

static bool TableGetRow(TableViewData* this, int index, const char * pTextOut[])
{
	if (index < 0 || index >= this->m_row_count) return false;
	
	for (int i = 0; i < this->m_column_count; i++)
	{
		pTextOut[i] = this->m_pRowData[index].m_items[i].m_text;
	}
	
	return true;
}

static void TableDeleteRow(TableViewData* this, int index)
{
	if (index < 0 || index >= this->m_row_count) return;
	
	TableViewRow row = this->m_pRowData[index];
	this->m_row_count--;
	memmove(&this->m_pRowData[index], &this->m_pRowData[index + 1], sizeof row * (this->m_row_count - index));
	
	if (this->m_selected_row == index)
	{
		this->m_selected_row = -1;
	}
	
	MmFree(row.m_items);
	row.m_items = NULL;
}

static void TableDeleteColumn(TableViewData* this, int index)
{
	// This will be a little more complicated since we have to memmove inside each row
	if (index < 0 || index >= this->m_column_count) return;
	
	TableViewColumn col = this->m_pColumnData[index];
	this->m_column_count--;
	memmove(&this->m_pColumnData[index], &this->m_pRowData[index + 1], sizeof col * (this->m_column_count - index));
	
	for (int i = 0; i < this->m_row_count; i++)
	{
		TableViewItem* ptr = this->m_pRowData[i].m_items;
		memmove(&ptr[index], &ptr[index + 1], sizeof (TableViewItem) * (this->m_column_count - index));
		this->m_pRowData[i].m_items = MmReAllocate(ptr, sizeof(TableViewItem) * (this->m_column_count));
	}
}

static void TableClearRows(TableViewData* this)
{
	// Get rid of each row.
	while (this->m_row_count > 0)
	{
		TableDeleteRow(this, this->m_row_count - 1);
	}
	
	this->m_row_capacity = 0;
	
	MmFree(this->m_pRowData);
	this->m_pRowData = NULL;
}

// note: you can't really clear the columns since they'd eliminate all the data within the rows anyway :)

static void TableClearEverything(TableViewData* this)
{
	TableClearRows(this);
	
	// Get rid of each column.
	while (this->m_column_count > 0)
	{
		TableDeleteColumn(this, this->m_column_count - 1);
	}
	
	this->m_column_capacity = 0;
	
	MmFree(this->m_pColumnData);
	this->m_pColumnData = NULL;
}

static void TableDestroy(TableViewData* this)
{
	TableClearEverything(this);
}

static void CtlUpdateScrollBarSize(Control *this, Window * pWindow)
{
	TableViewData * data = &this->m_tableViewData;
	
	int rowsPerSize = (this->m_rect.bottom - this->m_rect.top - (TITLE_BAR_HEIGHT - 2) + TABLE_ITEM_HEIGHT - 1) / TABLE_ITEM_HEIGHT;
	
	int c = data->m_row_count - rowsPerSize + 3;
	if (c <= 1)
		c  = 1;
	
	SetScrollBarMax(pWindow, -this->m_comboID, c);
	CallControlCallback(pWindow, -this->m_comboID, EVENT_PAINT, 0, 0);
}

static void CtlAddTableRow(Control * this, const char* pText[], int optionalIcon, Window* pWindow)
{
	TableAddRow(&this->m_tableViewData, optionalIcon, pText);
	CtlUpdateScrollBarSize(this, pWindow);
}

static void CtlAddTableColumn(Control * this, const char* pText, int width, Window* pWindow, bool bHandlesNumbers)
{
	TableAddColumn(&this->m_tableViewData, pText, width, bHandlesNumbers);
	CtlUpdateScrollBarSize(this, pWindow);
}

static bool CtlGetRowFromTable(Control * this, int index, const char * pTextOut[])
{
	return TableGetRow(&this->m_tableViewData, index, pTextOut);
}

static void CtlRemoveRowFromTable(Control * this, int index, Window* pWindow)
{
	TableDeleteRow(&this->m_tableViewData, index);
	CtlUpdateScrollBarSize(this, pWindow);
}

static void CtlResetTable(Control * this, Window* pWindow)
{
	TableClearEverything(&this->m_tableViewData);
	this->m_tableViewData.m_row_scroll   = 0;
	this->m_tableViewData.m_selected_row = 0;
	CtlUpdateScrollBarSize(this, pWindow);
}

static int CtlGetSelectedIndexTable(Control * this)
{
	return this->m_tableViewData.m_selected_row;
}

static Rectangle WidgetTableView_GetBottomRect(Control* this);
static void WidgetTableView_PaintRow(Control* this, int index, Rectangle clipRect);

static void CtlSetSelectedIndexTable(Control * this, int selectedIndex)
{
	if (selectedIndex < 0 || selectedIndex >= this->m_tableViewData.m_row_count)
		selectedIndex = -1;
	
	int oldSelection = this->m_tableViewData.m_selected_row;
	
	this->m_tableViewData.m_selected_row = selectedIndex;
	
	Rectangle bottomBar = WidgetTableView_GetBottomRect(this);
	VidSetClipRect(&bottomBar);
	WidgetTableView_PaintRow(this, oldSelection, bottomBar);
	WidgetTableView_PaintRow(this, selectedIndex, bottomBar);
	VidSetClipRect(NULL);
}

static int CtlGetScrollTable(Control * this)
{
	return this->m_tableViewData.m_row_scroll;
}

static void CtlSetScrollTable(Control * this, Window * pWindow, int scroll)
{
	if (scroll < 0) scroll = 0;
	
	int max = GetScrollBarMax(pWindow, -this->m_comboID);
	if (scroll >= max)
		scroll = max;
	
	this->m_tableViewData.m_row_scroll = scroll;
	
	SetScrollBarPos(pWindow, -this->m_comboID, scroll);
	
	CallControlCallback(pWindow,  this->m_comboID, EVENT_PAINT, 0, 0);
	CallControlCallback(pWindow, -this->m_comboID, EVENT_PAINT, 0, 0);
}

void AddTableRow (Window* pWindow, int comboID, const char* pText[], int optionalIcon)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlAddTableRow(&pWindow->m_pControlArray[i], pText, optionalIcon, pWindow);
			return;
		}
	}
}

void AddTableColumnEx(Window* pWindow, int comboID, const char* pText, int width, bool bHandlesNumbers)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlAddTableColumn(&pWindow->m_pControlArray[i], pText, width, pWindow, bHandlesNumbers);
			return;
		}
	}
}

void AddTableColumn(Window* pWindow, int comboID, const char* pText, int width)
{
	AddTableColumnEx(pWindow, comboID, pText, width, false);
}

bool GetRowStringsFromTable(Window* pWindow, int comboID, int index, const char * output[])
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			return CtlGetRowFromTable(&pWindow->m_pControlArray[i], index, output);
		}
	}
	return false;
}

void RemoveRowFromTable(Window* pWindow, int comboID, int elementIndex)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlRemoveRowFromTable(&pWindow->m_pControlArray[i], elementIndex, pWindow);
			return;
		}
	}
}

void ResetTable(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlResetTable(&pWindow->m_pControlArray[i], pWindow);
		}
	}
}

int GetSelectedIndexTable(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
			return CtlGetSelectedIndexTable(&pWindow->m_pControlArray[i]);
	}
	return -1;
}

void SetSelectedIndexTable(Window* pWindow, int comboID, int selind)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
			CtlSetSelectedIndexTable(&pWindow->m_pControlArray[i], selind);
	}
}

int GetScrollTable(Window* pWindow, int comboID)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
			return CtlGetScrollTable(&pWindow->m_pControlArray[i]);
	}
	return -1;
}

void SetScrollTable(Window* pWindow, int comboID, int scroll)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
			CtlSetScrollTable(&pWindow->m_pControlArray[i], pWindow, scroll);
	}
}

static Rectangle WidgetTableView_GetBottomRect(Control* this)
{
	Rectangle inRect = this->m_rect;
	
	inRect.left += 2;
	inRect.top  += 2;
	inRect.right  -= 2;
	inRect.bottom -= 2;
	
	// Draw the top bar.
	Rectangle bottomBar;
	bottomBar = inRect;
	// Render each row.
	bottomBar.top += TITLE_BAR_HEIGHT - 2;
	
	return bottomBar;
}

static void WidgetTableView_PaintRow(Control* this, int index, Rectangle clipRect)
{
	Rectangle bottomBar = WidgetTableView_GetBottomRect(this);
	
	int topOffset = (TABLE_ITEM_HEIGHT - GetLineHeight()) / 2;
	
	TableViewData* table = &this->m_tableViewData;
	
	int iYPos = index - table->m_row_scroll;
	int rowsPerSize = (bottomBar.bottom - bottomBar.top + TABLE_ITEM_HEIGHT - 1) / TABLE_ITEM_HEIGHT;
	if (iYPos < 0 || iYPos >= rowsPerSize) return;
	
	int defaultStartX = 22;
	if (this->m_parm2 & TABLEVIEW_NOICONCOLUMN)
		defaultStartX = 0;
	
	int startX = bottomBar.left + defaultStartX + 4, offs = bottomBar.top + iYPos * TABLE_ITEM_HEIGHT;
	
	Rectangle rect = bottomBar;
	
	rect.top += iYPos * TABLE_ITEM_HEIGHT;
	rect.bottom = rect.top + TABLE_ITEM_HEIGHT - 1;
	
	if (rect.bottom < clipRect.top || rect.top > clipRect.bottom) return;
	
	uint32_t bgCol;
	
	// TODO: Choose better colors.
	if (table->m_selected_row == index)
		bgCol = SELECTED_ITEM_COLOR;
	else if (index < 0 || index >= table->m_row_count)
		bgCol = LIST_BACKGD_COLOR;
	else if (index % 2 == 1)
		bgCol = TABLE_VIEW_ALT_ROW_COLOR;
	else
		bgCol = LIST_BACKGD_COLOR;
	
	VidFillRectangle(bgCol, rect);
	
	if (index < 0 || index >= table->m_row_count) return;
	
	TableViewRow* row = &table->m_pRowData[index];
	
	// draw the icon
	RenderIconForceSize(row->m_icon, bottomBar.left + 4, offs + (TABLE_ITEM_HEIGHT - 16) / 2, 16);
	
	// TODO: Allow scrolling via the column count as well.
	for (int j = 0; j < table->m_column_count; j++)
	{
		TableViewColumn* col = &table->m_pColumnData[j];
		
		VidTextOutLimit(row->m_items[j].m_text, startX, offs + topOffset, (table->m_selected_row == index) ? SELECTED_MENU_ITEM_TEXT_COLOR : DESELECTED_MENU_ITEM_TEXT_COLOR, TRANSPARENT, col->m_width);
		
		startX += col->m_width;
	}
}

void WidgetTableView_UpdateBottomView(Control* this, Rectangle bottomBar)
{
	TableViewData* table = &this->m_tableViewData;
	Rectangle actualBottomView = WidgetTableView_GetBottomRect(this);
	
	VidSetClipRect(&bottomBar);
	
	int rowsPerSize = (actualBottomView.bottom - actualBottomView.top + TABLE_ITEM_HEIGHT - 1) / TABLE_ITEM_HEIGHT;
	
	for (int i = 0, index = table->m_row_scroll; i < rowsPerSize; i++, index++)
	{
		WidgetTableView_PaintRow(this, index, bottomBar);
	}
	
	VidSetClipRect(NULL);
}

void WidgetTableView_SetScrollPos(Control* this, int pos)
{
	TableViewData* pData = &this->m_tableViewData;
	if (pData->m_row_scroll == pos) return;

	int oldScroll = pData->m_row_scroll;
	pData->m_row_scroll  = pos;
	
	int yDiff = TABLE_ITEM_HEIGHT * (oldScroll - pos);
	if (yDiff == 0) return; // redundant?
	
	Rectangle rect = WidgetTableView_GetBottomRect(this);
	
	ScrollRect(&rect, 0, yDiff);
	
	Rectangle updateRect = rect;
	if (yDiff > 0)
	{
		updateRect.bottom = updateRect.top + yDiff;
	}
	else
	{
		updateRect.top = updateRect.bottom + yDiff;
	}
	
	WidgetTableView_UpdateBottomView(this, updateRect);
}

bool WidgetTableView_OnEvent(Control* this, UNUSED int eventType, UNUSED long parm1, UNUSED long parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_DESTROY:
		{
			TableDestroy(&this->m_tableViewData);
			RemoveControl(pWindow, -this->m_comboID);
			break;
		}
		case EVENT_CREATE:
		{
			// Add a scroll bar.
			Rectangle r;
			r.right = this->m_rect.right,
			r.top   = this->m_rect.top,
			r.bottom= this->m_rect.bottom,
			r.left  = this->m_rect.right - SCROLL_BAR_WIDTH;
			
			int flags = 0;
			if (this->m_anchorMode & ANCHOR_RIGHT_TO_RIGHT)
				flags |= ANCHOR_RIGHT_TO_RIGHT | ANCHOR_LEFT_TO_RIGHT;
			if (this->m_anchorMode & ANCHOR_BOTTOM_TO_BOTTOM)
				flags |= ANCHOR_BOTTOM_TO_BOTTOM;
			
			AddControlEx (pWindow, CONTROL_VSCROLLBAR, flags, r, NULL, -this->m_comboID, 1, 1);
			
			//shrink our rectangle:
			this->m_rect.right -= SCROLL_BAR_WIDTH + 4;
			
			TableViewData* table = &this->m_tableViewData;
			
			table->m_selected_row = -1;
			
			break;
		}
		case EVENT_KEYRAW:
		{
			if (!this->m_bFocused) break;
			
			if (parm1 & 0x80) break;
			
			uint8_t code = (uint8_t)parm1;
			TableViewData* table = &this->m_tableViewData;
			
			int selectedRow = table->m_selected_row;
			int rowScroll   = table->m_row_scroll;
			
			switch (code)
			{
				case KEY_ARROW_UP:
				{
					if (selectedRow == -1) break;
					
					selectedRow--;
					if (selectedRow < 0)
						selectedRow = 0;
					
					if (rowScroll >= selectedRow)
						rowScroll  = selectedRow;
					
					break;
				}
				case KEY_ARROW_DOWN:
				{
					if (selectedRow == -1) break;
					
					int rowsPerSize = (this->m_rect.bottom - this->m_rect.top - (TITLE_BAR_HEIGHT - 2) + TABLE_ITEM_HEIGHT - 1) / TABLE_ITEM_HEIGHT;
					
					selectedRow++;
					if (selectedRow >= table->m_row_count)
						selectedRow  = table->m_row_count - 1;
					
					if (rowScroll <= selectedRow - rowsPerSize + 2)
						rowScroll  = selectedRow - rowsPerSize + 2;
					
					break;
				}
				case KEY_ENTER:
				{
					if (selectedRow == -1) break;
					
					if (selectedRow < 0 || selectedRow >= this->m_tableViewData.m_row_capacity) break;
					
					// well, this behaves like a double click.
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, selectedRow);
					
					break;
				}
			}
			
			CtlSetSelectedIndexTable(this, selectedRow);
			WidgetTableView_SetScrollPos(this, rowScroll);
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Point pt = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			TableViewData* table = &this->m_tableViewData;
			
			if (!RectangleContains(&this->m_rect, &pt))
			{
				if (this->m_bFocused)
					SetFocusedControl(pWindow, -1);
				break;
			}
			
			SetFocusedControl(pWindow, this->m_comboID);
			
			Rectangle bottomBar = WidgetTableView_GetBottomRect(this);
			
			int rowsPerSize = (bottomBar.bottom - bottomBar.top + TABLE_ITEM_HEIGHT - 1) / TABLE_ITEM_HEIGHT;
			
			for (int i = 0, index = table->m_row_scroll; index < table->m_row_count && i < rowsPerSize; i++, index++)
			{
				Rectangle rect = bottomBar;
				
				rect.top += i * TABLE_ITEM_HEIGHT;
				rect.bottom = rect.top + TABLE_ITEM_HEIGHT - 1;
				
				if (RectangleContains(&rect, &pt))
				{
					// select this!
					bool isDoubleClick = table->m_selected_row == index;
					
					CtlSetSelectedIndexTable(this, index);
					
					if (isDoubleClick)
					{
						CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, index);
					}
					
					break;
				}
			}
			
			break;
		}
		case EVENT_CTLUPDATEVISIBLE:
		{
			SetControlVisibility(pWindow, -this->m_comboID, this->m_bVisible);
			break;
		}
		case EVENT_CLICKCURSOR:
		case EVENT_SCROLLDONE:
		{
			int pos = GetScrollBarPos(pWindow, -this->m_comboID);
			
			WidgetTableView_SetScrollPos(this, pos);
			
			break;
		}
		case EVENT_PAINT:
		{
			VidSetClipRect(&this->m_rect);
			//VidFillRectangle(LIST_BACKGD_COLOR, this->m_rect);
			
			Rectangle inRect = this->m_rect;
			DrawEdge(inRect, DRE_SUNKENINNER | DRE_SUNKENOUTER, 0);
			
			inRect.left += 2;
			inRect.top  += 2;
			inRect.right  -= 2;
			inRect.bottom -= 2;
			
			TableViewData* table = &this->m_tableViewData;
			
			// Draw the top bar.
			Rectangle topBar, bottomBar;
			topBar = bottomBar = inRect;
			topBar.bottom = topBar.top + TITLE_BAR_HEIGHT - 2;
			
			Rectangle columnBar = topBar;
			columnBar.right = columnBar.left;
			
			int defaultStartX = 22;
			if (this->m_parm2 & TABLEVIEW_NOICONCOLUMN)
				defaultStartX = 0;
			
			int topOffset = (TABLE_ITEM_HEIGHT - GetLineHeight()) / 2;
			
			if (!table->m_pColumnData) return false;
			
			for (int i = -1; i < table->m_column_count + 1; i++)
			{
				TableViewColumn * col = NULL;
				int colWidth = defaultStartX; const char * text = "";
				if (i >= 0)
				{
					if (i < table->m_column_count)
					{
						col = &table->m_pColumnData[i];
						colWidth = col->m_width;
						text     = col->m_text;
					}
					else
					{
						colWidth = inRect.right - columnBar.right;
					}
				}
				
				columnBar.left  = columnBar.right;
				columnBar.right = columnBar.left + colWidth;
				
				Rectangle columnBarClip = columnBar;
				if (columnBarClip.left  >= inRect.right) break;
				if (columnBarClip.right >= inRect.right) columnBarClip.right = inRect.right;
				
				VidSetClipRect(&columnBarClip);
				
				DrawEdge(columnBar, DRE_RAISEDINNER | DRE_RAISEDOUTER | DRE_FILLED, BUTTONMIDC);
				
				VidTextOutLimit(text, columnBar.left + 4, columnBar.top + topOffset, WINDOW_TEXT_COLOR, TRANSPARENT, colWidth);
				
				if (col)
					col->m_rect = columnBar;
				
				VidSetClipRect(NULL);
			}
			
			// Render each row.
			bottomBar.top = topBar.bottom;
			
			WidgetTableView_UpdateBottomView(this, bottomBar);
			
			break;
		}
	}
	
	return false;
}
