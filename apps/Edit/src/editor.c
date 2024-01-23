/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
         Command-line text editor

            Editor source file
******************************************/
#include <nanoshell/nanoshell.h>

#define C_MIN_LINE_CAPACITY (32)

// note: this is a HACK and will be removed soon.
void outb(char c)
{
	__asm__("outb %1, %0\n\t"::"d"((short)0xE9),"a"(c));
}

void HackLogMsg(const char * fmt, ...)
{
	va_list lst;
	va_start(lst, fmt);
	
	char buf[8192];
	vsprintf(buf, fmt, lst);
	strcat(buf, "\n");
	
	for (char* ptr = buf; *ptr; ptr++) outb(*ptr);
	
	va_end(lst);
}

Console * GetConsole();

char g_fileName [PATH_MAX];
bool g_bModified;

typedef struct Line
{
	struct Line* prev, *next;
	char* str;
	size_t cap, len;
}
Line;

Line *g_firstLine, *g_lastLine;

Line *g_scrollLine;
int g_line, g_column, g_scrollLineNum;

Line* LineCreate()
{
	Line* ptr = malloc(sizeof(Line));
	memset(ptr, 0, sizeof *ptr);
	return ptr;
}

const char * LineGetCString(Line * line)
{
	if (!line->str)
		return "";
	
	// ensure the terminator is properly there
	line->str[line->len] = 0;
	
	// then doing this will do fine
	return line->str;
}

void LineAppend(Line* line, const char * str)
{
	size_t len = strlen(str);
	
	if (line->str == NULL)
	{
		// build a new line string
		line->str = malloc(C_MIN_LINE_CAPACITY);
		line->cap = C_MIN_LINE_CAPACITY;
		line->len = 0;
	}
	else if (line->len + len + 1 >= line->cap)
	{
		// should expand
		line->cap *= 2;
		line->str = realloc(line->str, line->cap);
	}
	
	memcpy(line->str + line->len, str, len);
	line->len += len;
}

void LineEmpty(Line* line)
{
	free(line->str);
	line->str = NULL;
	line->cap = line->len = 0;
}

void LineConnect(Line* dest, Line* toConnect)
{
	if (dest == NULL)
	{
		// who knows why they would be trying this? probably blindly LineAppend(g_lastLine, ...);
		// well, okay, make sure they're that way
		if (g_firstLine == NULL && g_lastLine == NULL)
		{
			g_firstLine = g_lastLine = toConnect;
			return;
		}
		
		// otherwise, resort to appending to g_lastLine right away
		LineConnect(g_lastLine, toConnect);
		return;
	}
	
	if (dest->next)
	{
		dest->next->prev = toConnect;
		toConnect->next  = dest->next;
	}
	
	toConnect->prev = dest;
	dest->next = toConnect;
	
	if (g_lastLine == dest)
		g_lastLine =  toConnect;
}

int EditorReadFile(const char * filename)
{
	// try to open the file.
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		// oops! Couldn't open the file for reading.
		return fd;
	}
	
	char buffer[4096];
	
	Line* currentLine = LineCreate();
	
	char str[2];
	str[1] = 0;
	
	size_t have_read;
	while ((have_read = read(fd, buffer, sizeof buffer)) > 0)
	{
		// the length of the data read is in have_read.
		for (size_t i = 0; i < have_read; i++)
		{
			if (buffer[i] == '\n')
			{
				// this is the end of the current line. Connect it to the other lines
				LineConnect(g_lastLine, currentLine);
				// reset the current line
				currentLine = LineCreate();
			}
			else
			{
				// just append this character to the current line.
				str[0] = buffer[i];
				LineAppend(currentLine, str);
			}
		}
	}
	
	LineConnect(g_lastLine, currentLine);
	
	close(fd);
	
	strncpy(g_fileName, filename, PATH_MAX - 1);
	
	return 0;
}

int minimum(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}

void EditorMoveCursor()
{
	printf("\e[%d;%dH", g_column, g_line - g_scrollLineNum + 1);
}

void ScreenEditorFullRefresh()
{
	// starting from the current line, and going down to the console height
	int conheight = GetConsole()->height, conwidth = GetConsole()->width;
	
	Line * currentLine = g_scrollLine;
	for (int i = 0; i < conheight - 3; i++)
	{
		int cw = conwidth;
		printf("\e[%d;%dH\e[0K", 1, i+1);
		
		// print each character. If it needs to be escaped, be sure to do so.
		if (currentLine)
		{
			int len = minimum (cw - 1, currentLine->len);
			for (int j = 0; j < len; j++)
			{
				char c = currentLine->str[j];
				
				// If this character needs to be escaped.
				if (c < ' ')
				{
					cw--;
					len = minimum (cw - 1, currentLine->len);
					printf("^%c", c + '@');
				}
				else
				{
					// Just outright print it.
					printf("%c", c);
				}
			}
			
			currentLine = currentLine->next;
		}
	}
	
	// redirect the cursor back where it should be
	EditorMoveCursor();
}

void ScreenSidebarRefresh()
{
	int conheight = GetConsole()->height, conwidth = GetConsole()->width;
	char filename[PATH_MAX];
	int cwopm = conwidth - 1;
	if (cwopm > PATH_MAX - 1)
		cwopm = PATH_MAX - 1;
	
	strncpy(filename, g_fileName, conwidth);
	strcpy(filename + cwopm - 3, "...");
	filename[cwopm] = 0;
	
	// print the help bar
	printf("\e[1;%dH\e[0K", conheight - 2);
	printf("\e[7m\e[0J%s%c\e[0m", filename, g_bModified ? '*' : ' ');
	printf("\e[1;%dH\e[0K", conheight - 1);
	printf(" \e[7m^X\e[0m - Quit   \e[7m^O\e[0m - Open file");
	
	// redirect the cursor back where it should be
	EditorMoveCursor();
}

void ScreenFullRefresh()
{
	// leave 2 rows for the helpful bar
	ScreenEditorFullRefresh();
	ScreenSidebarRefresh();
}

#define CTRL(chr) (chr - '@') // make sure to use CAPITAL letters!

Line* GetCurrentLine()
{
	int offsetFromScrollLine = g_line - g_scrollLineNum;
	
	Line* cl = g_scrollLine;
	while (offsetFromScrollLine < 0)
	{
		if (!cl->prev) return cl;
		cl = cl->prev;
		offsetFromScrollLine--;
	}
	while (offsetFromScrollLine > 0)
	{
		if (!cl->next) return cl;
		cl = cl->next;
		offsetFromScrollLine--;
	}
	return cl;
}

void EditorMovePrevLine()
{
	//int conheight = GetConsole()->height;
	// If the current line is the scroll line
	if (GetCurrentLine() == g_scrollLine)
	{
		// try scrolling up once
		if (!g_scrollLine->prev) return;
		g_scrollLine = g_scrollLine->prev;
		g_scrollLineNum--;
		g_line--;
		
		// ensure column is within bounds
		
		ScreenEditorFullRefresh();
	}
	else
	{
		printf("\e[A");
		g_line--;
	}
}

bool EditorMoveNextLine()
{
	int conheight = GetConsole()->height;
	// Should we do a full on scroll down?
	if (g_line - g_scrollLineNum >= conheight - 5)
	{
		// try scrolling up once
		if (!GetCurrentLine()->next) return false;
		g_scrollLine = g_scrollLine->next;
		g_scrollLineNum++;
		g_line++;
		
		ScreenEditorFullRefresh();
		EditorMoveCursor();
	}
	else
	{
		printf("\e[B");
		g_line++;
	}
	
	return true;
}

void EditorHandleEscapeCode(char c)
{
	switch (c)
	{
		case 'A': // Move Up
		{
			EditorMovePrevLine();
			break;
		}
		case 'B': // Move Down
		{
			EditorMoveNextLine();
			break;
		}
		case 'C': // Move Right
		{
			Line *curLine = GetCurrentLine();
			
			g_column++;
			if (g_column > (int)curLine->len + 1)
			{
				if (EditorMoveNextLine())
				{
					g_column = 1;
					EditorMoveCursor();
				}
			}
			else
			{
				printf("\e[C");
			}
			
			break;
		}
		case 'D': // Move Left
		{
			g_column--;
			if (g_column < 1)
			{
				g_column = 1;
				EditorMovePrevLine();
				EditorMoveCursor();
			}
			else
			{
				printf("\e[D");
			}
			
			break;
		}
	}
}

void EditorHandleBackspace()
{
	
}

void EditorHandleCharacter(UNUSED char c)
{
	
}

// TODO: fix dependency on internal CRT function
char _I_ReadChar();

void EditorRun()
{
	g_scrollLine = g_firstLine;
	g_scrollLineNum = 1;
	g_line   = 1;
	g_column = 1;
	
	ScreenFullRefresh();
	
	bool bRunning = true;
	
	while (bRunning)
	{
		char c;
		c = _I_ReadChar();
		
		switch (c)
		{
			case CTRL('X'): // ^X
			{
				//TODO: Save confirmation, and all that
				printf("\e[2J\e[1;1H");
				bRunning = false;
				break;
			}
			case CTRL('['): // ^[
			{
				// read the [ part (CSI code)
				c = _I_ReadChar();
				
				if (c != '[') break; // wellp
				
				// read one more character
				c = _I_ReadChar();
				
				EditorHandleEscapeCode(c);
				
				break;
			}
			case CTRL('H'): // ^H - Backspace
			{
				EditorHandleBackspace();
			}
			default:
			{
				EditorHandleCharacter(c);
				break;
			}
		}
	}
}
