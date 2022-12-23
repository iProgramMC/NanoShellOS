/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Widget library: Table view control
******************************************/
#include <widget.h>
#include <keyboard.h>
#include <icon.h>

// NOTE: Damn, this is a pretty heavy control. 144 KB for 34 entries.
// This is probably a sign we need to optimize the memory allocator for sub-page-size allocations!

#define TABLE_WIDGET_TEST

#define TABLE_ITEM_HEIGHT (TITLE_BAR_HEIGHT)

static void TableAddColumn(TableViewData* this, const char * text, int width)
{
	if (this->m_column_count + 1 > this->m_column_capacity)
	{
		// expand
		if (this->m_column_capacity == 0)
			this->m_column_capacity  = 4;
		TableViewColumn * newData = MmReAllocate(this->m_pColumnData, this->m_column_capacity * 2);
		if (!newData)
		{
			SLogMsg("Could not add column '%s' to table.", text);
			return;
		}
		
		this->m_column_capacity *= 2;
		this->m_pColumnData = newData;
	}
	
	TableViewColumn* newColumn = &this->m_pColumnData[this->m_column_count++];
	
	SLogMsg("NewC:%p  CC:%d Cap:%d", newColumn, this->m_column_count, this->m_column_capacity);
	
	memset(newColumn, 0, sizeof *newColumn);
	
	strncpy(newColumn->m_text, text, MAX_COLUMN_LENGTH);
	newColumn->m_text[MAX_COLUMN_LENGTH - 1] = 0;
	
	newColumn->m_sort_order = 0;
	newColumn->m_width      = width;
	
	// For each row, re-allocate its item count.
	for (int i = 0; i < this->m_row_count; i++)
	{
		// TODO we're doing leaps of faith right now, maybe we shouldn't!
		// Ideally you'd set up the columns first, though.
		this->m_pRowData[i].m_items = MmReAllocate(this->m_pRowData[i].m_items, sizeof(char*) * this->m_column_count);
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
		TableViewRow * newData = MmReAllocate(this->m_pRowData, this->m_row_capacity * 2);
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

static void CtlAddTableColumn(Control * this, const char* pText, int width, Window* pWindow)
{
	TableAddColumn(&this->m_tableViewData, pText, width);
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
	CtlUpdateScrollBarSize(this, pWindow);
}

static int CtlGetSelectedIndexTable(Control * this)
{
	return this->m_tableViewData.m_selected_row;
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

void AddTableColumn(Window* pWindow, int comboID, const char* pText, int width)
{
	for (int i = 0; i < pWindow->m_controlArrayLen; i++)
	{
		if (pWindow->m_pControlArray[i].m_comboID == comboID)
		{
			CtlAddTableColumn(&pWindow->m_pControlArray[i], pText, width, pWindow);
			return;
		}
	}
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
		{
			return CtlGetSelectedIndexTable(&pWindow->m_pControlArray[i]);
		}
	}
	return -1;
}

bool WidgetTableView_OnEvent(Control* this, UNUSED int eventType, UNUSED int parm1, UNUSED int parm2, UNUSED Window* pWindow)
{
	switch (eventType)
	{
		case EVENT_DESTROY:
		{
			TableDestroy(&this->m_tableViewData);
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
			
			// Add some elements for testing.
			TableViewData* table = &this->m_tableViewData;
			
			#ifdef TABLE_WIDGET_TEST
			
			TableAddColumn(table, "Name",               120);
			TableAddColumn(table, "Last modified date", 150);
			TableAddColumn(table, "Size",                70);
			
			table->m_selected_row = -1;
			
			// yes.. this is dumb, but it's okay because I'm going to get rid of this soon
			const char * row1[3] = { "limn2600",        "02/12/2022 20:44:46", ""};
			const char * row2[3] = { "test.txt",        "17/12/2022 19:47:23", "11 B" };
			const char * row3[3] = { "about_theme.raw", "21/12/2022 12:29:05", "5,306 KB" };
			const char * row4[3] = { "lma.raw",         "21/12/2022 12:58:57", "57,339 KB" };
			const char * row5[3] = { "Package",         "24/12/2022 20:00:00", "133,700,042 KB" };
			const char * row6[3] = { "kernel.bin",      "23/12/2022 19:11:45", "1,006 KB" };
			const char * row7[3] = { "cinterp.c", "19/01/2022 20:25:26", "69 KB" };
			const char * row8[3] = { "clip.c", "19/01/2022 20:25:26", "69 KB" };
			const char * row9[3] = { "config.c", "19/01/2022 20:25:26", "69 KB" };
			const char * row0[3] = { "console.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowa[3] = { "debug.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowb[3] = { "elf.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowc[3] = { "font.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowd[3] = { "fpu.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowe[3] = { "icon.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowf[3] = { "idt.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowg[3] = { "image.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowh[3] = { "keyboard.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowi[3] = { "main.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowj[3] = { "misc.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowk[3] = { "mouse.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowl[3] = { "mspy.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowm[3] = { "pci.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rown[3] = { "print.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowo[3] = { "process.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowp[3] = { "resource.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowq[3] = { "sb.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowr[3] = { "shell.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rows[3] = { "string.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowt[3] = { "syscall.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowu[3] = { "task.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowv[3] = { "uart.c", "19/01/2022 20:25:26", "69 KB" };
			const char * roww[3] = { "vga.c", "19/01/2022 20:25:26", "69 KB" };
			const char * rowx[3] = { "video.c", "19/01/2022 20:25:26", "69 KB" };
			
			AddTableRow(pWindow, this->m_comboID, row1, ICON_FOLDER);
			AddTableRow(pWindow, this->m_comboID, row2, ICON_TEXT_FILE);
			AddTableRow(pWindow, this->m_comboID, row3, ICON_FILE_IMAGE);
			AddTableRow(pWindow, this->m_comboID, row4, ICON_FLOPPY);
			AddTableRow(pWindow, this->m_comboID, row5, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, row6, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, row7, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, row8, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, row9, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, row0, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowa, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowb, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowc, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowd, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowe, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowf, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowg, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowh, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowi, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowj, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowk, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowl, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowm, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rown, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowo, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowp, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowq, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowr, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rows, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowt, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowu, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowv, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, roww, ICON_PACKAGER);
			AddTableRow(pWindow, this->m_comboID, rowx, ICON_PACKAGER);
			
			#endif
			
			break;
		}
		case EVENT_KEYRAW:
		{
			if (!this->m_bFocused) break;
			
			if (parm1 & 0x80) break;
			
			uint8_t code = (uint8_t)parm1;
			bool bNeedRepaint = false;
			TableViewData* table = &this->m_tableViewData;
			
			switch (code)
			{
				case KEY_ARROW_UP:
				{
					if (table->m_selected_row == -1) break;
					
					int old_row = table->m_selected_row;
					
					table->m_selected_row--;
					if (table->m_selected_row < 0)
						table->m_selected_row = 0;
					
					if (table->m_row_scroll >= table->m_selected_row)
						table->m_row_scroll  = table->m_selected_row;
					
					if (table->m_selected_row != old_row)
						bNeedRepaint = true;
					
					break;
				}
				case KEY_ARROW_DOWN:
				{
					if (table->m_selected_row == -1) break;
					
					int rowsPerSize = (this->m_rect.bottom - this->m_rect.top - (TITLE_BAR_HEIGHT - 2) + TABLE_ITEM_HEIGHT - 1) / TABLE_ITEM_HEIGHT;
					
					int old_row = table->m_selected_row;
					
					table->m_selected_row++;
					if (table->m_selected_row >= table->m_row_count)
						table->m_selected_row  = table->m_row_count - 1;
					
					if (table->m_row_scroll <= table->m_selected_row - rowsPerSize + 2)
						table->m_row_scroll  = table->m_selected_row - rowsPerSize + 2;
					
					if (table->m_selected_row != old_row)
						bNeedRepaint = true;
					
					break;
				}
				case KEY_ENTER:
				{
					if (table->m_selected_row == -1) break;
					
					// well, this behaves like a double click.
					CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, table->m_selected_row);
					
					break;
				}
			}
			
			if (bNeedRepaint)
			{
				WidgetTableView_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
			}
			
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
			
			// Draw the top bar.
			Rectangle topBar, bottomBar;
			topBar = bottomBar = this->m_rect;
			topBar.bottom = topBar.top + TITLE_BAR_HEIGHT - 2;
			
			Rectangle columnBar = topBar;
			columnBar.right = columnBar.left - 1;
			
			// Render each row.
			bottomBar.top = topBar.bottom;
			
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
					table->m_selected_row = index;
					
					WidgetTableView_OnEvent(this, EVENT_PAINT, 0, 0, pWindow);
					
					if (isDoubleClick)
					{
						CallWindowCallback(pWindow, EVENT_COMMAND, this->m_comboID, index);
					}
					
					break;
				}
			}
			
			
			break;
		}
		case EVENT_CLICKCURSOR:
		case EVENT_SCROLLDONE:
		{
			TableViewData* pData = &this->m_tableViewData;
			int pos = GetScrollBarPos(pWindow, -this->m_comboID);
			if (pData->m_row_scroll != pos)
			{
				pData->m_row_scroll  = pos;
			}
			else break;
			// fallthrough intended
		}
		case EVENT_PAINT:
		{
			VidSetClipRect(&this->m_rect);
			VidFillRectangle(WINDOW_TEXT_COLOR_LIGHT, this->m_rect);
			
			TableViewData* table = &this->m_tableViewData;
			
			// Draw the top bar.
			Rectangle topBar, bottomBar;
			topBar = bottomBar = this->m_rect;
			topBar.bottom = topBar.top + TITLE_BAR_HEIGHT - 2;
			
			Rectangle columnBar = topBar;
			columnBar.right = columnBar.left - 1;
			
			int defaultStartX = 22;
			int topOffset = (TABLE_ITEM_HEIGHT - GetLineHeight()) / 2;
			
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
						colWidth = this->m_rect.right - columnBar.right - 1;
					}
				}
				
				columnBar.left  = columnBar.right + 1;
				columnBar.right = columnBar.left + colWidth;
				
				Rectangle columnBarClip = columnBar;
				if (columnBarClip.left  >= this->m_rect.right) break;
				if (columnBarClip.right >= this->m_rect.right) columnBarClip.right = this->m_rect.right;
				
				VidSetClipRect(&columnBarClip);
				
				columnBar.right--;
				
				RenderButtonShapeSmall (columnBar, BUTTONDARK, BUTTONLITE, BUTTONMIDC);
				VidTextOutLimit(text, columnBar.left + 4, columnBar.top + topOffset, WINDOW_TEXT_COLOR, TRANSPARENT, colWidth);
			}
			
			// Render each row.
			bottomBar.top = topBar.bottom;
			
			VidSetClipRect(&bottomBar);
			
			int rowsPerSize = (bottomBar.bottom - bottomBar.top + TABLE_ITEM_HEIGHT - 1) / TABLE_ITEM_HEIGHT;
			
			for (int i = 0, index = table->m_row_scroll; index < table->m_row_count && i < rowsPerSize; i++, index++)
			{
				int startX = bottomBar.left + defaultStartX + 4, offs = bottomBar.top + i * TABLE_ITEM_HEIGHT;
				
				TableViewRow* row = &table->m_pRowData[index];
				
				Rectangle rect = bottomBar;
				
				rect.top += i * TABLE_ITEM_HEIGHT;
				rect.bottom = rect.top + TABLE_ITEM_HEIGHT - 1;
				
				// TODO: Choose better colors.
				if (table->m_selected_row == index)
				{
					VidFillRectangle(0x7F, rect);
				}
				else if (index % 2 == 0)
				{
					VidFillRectangle(BUTTON_MIDDLE_COLOR, rect);
				}
				
				// draw the icon
				RenderIconForceSize(row->m_icon, bottomBar.left + 4, offs + (TABLE_ITEM_HEIGHT - 16) / 2, 16);
				
				// TODO: Allow scrolling via the column count as well.
				for (int j = 0; j < table->m_column_count; j++)
				{
					TableViewColumn* col = &table->m_pColumnData[j];
					
					VidTextOutLimit(row->m_items[j].m_text, startX, offs + topOffset, (table->m_selected_row == index) ? WINDOW_TEXT_COLOR_LIGHT : WINDOW_TEXT_COLOR, TRANSPARENT, col->m_width);
					
					startX += col->m_width;
				}
			}
			
			VidSetClipRect(NULL);
			break;
		}
	}
	
	return false;
}
