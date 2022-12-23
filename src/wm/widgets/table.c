/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

    Widget library: Table view control
******************************************/
#include <widget.h>
#include <icon.h>

// NOTE: Damn, this is a pretty heavy control. 144 KB for 34 entries.
// This is probably a sign we need to optimize the memory allocator for sub-page-size allocations!

#define TABLE_WIDGET_TEST

#define TABLE_ITEM_HEIGHT (TITLE_BAR_HEIGHT)

void TableAddColumn(TableViewData* this, const char * text, int width)
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
	}
}

TableViewRow* TableAddRow(TableViewData* this, int icon, const char * data[])
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

void TableDeleteRow(TableViewData* this, int index)
{
	if (index < 0 || index >= this->m_row_count) return;
	
	TableViewRow row = this->m_pRowData[index];
	this->m_row_count--;
	memmove(&this->m_pRowData[index], &this->m_pRowData[index + 1], sizeof row * (this->m_row_count - index));
	
	MmFree(row.m_items);
	row.m_items = NULL;
}

void TableDeleteColumn(TableViewData* this, int index)
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

void TableClearRows(TableViewData* this)
{
	// Get rid of each row.
	while (this->m_row_count > 0)
	{
		TableDeleteRow(this, this->m_row_count - 1);
	}
	
	MmFree(this->m_pRowData);
	this->m_pRowData = NULL;
}

// note: you can't really clear the columns since they'd eliminate all the data within the rows anyway :)

void TableClearEverything(TableViewData* this)
{
	TableClearRows(this);
	
	// Get rid of each column.
	while (this->m_column_count > 0)
	{
		TableDeleteColumn(this, this->m_column_count - 1);
	}
	
	MmFree(this->m_pColumnData);
	this->m_pColumnData = NULL;
}


void TableDestroy(TableViewData* this)
{
	TableClearEverything(this);
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
			
			TableAddRow(table, ICON_FOLDER,     row1);
			TableAddRow(table, ICON_TEXT_FILE,  row2);
			TableAddRow(table, ICON_FILE_IMAGE, row3);
			TableAddRow(table, ICON_FLOPPY,     row4);
			TableAddRow(table, ICON_PACKAGER,   row5);
			TableAddRow(table, ICON_PACKAGER,   row6);
			TableAddRow(table, ICON_PACKAGER,   row7);
			TableAddRow(table, ICON_PACKAGER,   row8);
			TableAddRow(table, ICON_PACKAGER,   row9);
			TableAddRow(table, ICON_PACKAGER,   row0);
			TableAddRow(table, ICON_PACKAGER,   rowa);
			TableAddRow(table, ICON_PACKAGER,   rowb);
			TableAddRow(table, ICON_PACKAGER,   rowc);
			TableAddRow(table, ICON_PACKAGER,   rowd);
			TableAddRow(table, ICON_PACKAGER,   rowe);
			TableAddRow(table, ICON_PACKAGER,   rowf);
			TableAddRow(table, ICON_PACKAGER,   rowg);
			TableAddRow(table, ICON_PACKAGER,   rowh);
			TableAddRow(table, ICON_PACKAGER,   rowi);
			TableAddRow(table, ICON_PACKAGER,   rowj);
			TableAddRow(table, ICON_PACKAGER,   rowk);
			TableAddRow(table, ICON_PACKAGER,   rowl);
			TableAddRow(table, ICON_PACKAGER,   rowm);
			TableAddRow(table, ICON_PACKAGER,   rown);
			TableAddRow(table, ICON_PACKAGER,   rowo);
			TableAddRow(table, ICON_PACKAGER,   rowp);
			TableAddRow(table, ICON_PACKAGER,   rowq);
			TableAddRow(table, ICON_PACKAGER,   rowr);
			TableAddRow(table, ICON_PACKAGER,   rows);
			TableAddRow(table, ICON_PACKAGER,   rowt);
			TableAddRow(table, ICON_PACKAGER,   rowu);
			TableAddRow(table, ICON_PACKAGER,   rowv);
			TableAddRow(table, ICON_PACKAGER,   roww);
			TableAddRow(table, ICON_PACKAGER,   rowx);
			
			#endif
			
			break;
		}
		case EVENT_RELEASECURSOR:
		{
			Point pt = { GET_X_PARM(parm1), GET_Y_PARM(parm1) };
			
			pt.x -= this->m_rect.left;
			pt.y -= this->m_rect.top;
			
			break;
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
			
			int elementsPerRow = (bottomBar.bottom - bottomBar.top + TABLE_ITEM_HEIGHT - 1) / TABLE_ITEM_HEIGHT;
			
			for (int i = 0, index = table->m_row_scroll; index < table->m_row_count && i < elementsPerRow; i++, index++)
			{
				int startX = bottomBar.left + defaultStartX + 4, offs = bottomBar.top + i * TABLE_ITEM_HEIGHT;
				
				TableViewRow* row = &table->m_pRowData[index];
				
				Rectangle rect = bottomBar;
				
				rect.top += i * TABLE_ITEM_HEIGHT;
				rect.bottom = rect.top + TABLE_ITEM_HEIGHT - 1;
				
				if (table->m_selected_row == index)
				{
					VidFillRectangle(SELECTED_ITEM_COLOR, rect); // TODO: Choose a better color.
				}
				else if (index % 2 == 0)
				{
					VidFillRectangle(BUTTON_MIDDLE_COLOR, rect); // TODO: Choose a better color.
				}
				
				// draw the icon
				RenderIconForceSize(row->m_icon, bottomBar.left + 4, offs + (TABLE_ITEM_HEIGHT - 16) / 2, 16);
				
				// TODO: Allow scrolling via the column count as well.
				for (int j = 0; j < table->m_column_count; j++)
				{
					TableViewColumn* col = &table->m_pColumnData[j];
					
					VidTextOutLimit(row->m_items[j].m_text, startX, offs + topOffset, WINDOW_TEXT_COLOR, TRANSPARENT, col->m_width);
					
					startX += col->m_width;
				}
			}
			
			VidSetClipRect(NULL);
			break;
		}
	}
	
	return false;
}
