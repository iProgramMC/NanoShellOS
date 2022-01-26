/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

            Icon listing module
******************************************/
#include <icon.h>

//WORK: Add in your icons here:
#include <icons/cabinet.h>
#include <icons/chip.h>
#include <icons/chip_sq.h>
#include <icons/computer.h>
#include <icons/computer_shutdown.h>
#include <icons/desktop.h>
#include <icons/draw.h>
#include <icons/earth.h>
#include <icons/error.h>
#include <icons/execute_file.h>
#include <icons/file.h>
#include <icons/files.h>
#include <icons/folder.h>
#include <icons/folder_blank.h>
#include <icons/folder_move.h>
#include <icons/folder_parent.h>
#include <icons/folder16_closed.h>
#include <icons/folder16_open.h>
#include <icons/globe.h>
#include <icons/go.h>
#include <icons/hand.h>
#include <icons/help.h>
#include <icons/info.h>
#include <icons/keyboard.h>
#include <icons/keyboard2.h>
#include <icons/laptop.h>
#include <icons/notes.h>
#include <icons/paint.h>
#include <icons/serial.h>
#include <icons/stop.h>
#include <icons/text_file.h>
#include <icons/warning.h>
#include <icons/nanoshell.h>
#include <icons/nanoshell16.h>
#include <icons/nanoshell_shell.h>
#include <icons/bomb.h>
#include <icons/bomb2.h>
#include <icons/nanoshell_shell16.h>
#include <icons/file16.h>
#include <icons/text_file16.h>
#include <icons/execute_file16.h>
#include <icons/folder_parent16.h>
#include <icons/folder_settings.h>
#include <icons/cabinet16.h>
#include <icons/computer16.h>
#include <icons/command.h>
#include <icons/command16.h>
#include <icons/error16.h>

Image * g_iconTable[] = {
	NULL,
	&g_cabinet_icon,
	&g_chip_icon,
	&g_chip_sq_icon,
	&g_computer_icon,
	&g_computer_shutdown_icon,
	&g_desktop_icon,
	&g_draw_icon,
	&g_earth_icon,
	&g_error_icon,
	&g_execute_file_icon,
	&g_file_icon,
	&g_files_icon,
	&g_folder_icon,
	&g_folder_blank_icon,
	&g_folder_move_icon,
	&g_folder_parent_icon,
	&g_folder16_closed_icon,
	&g_folder16_open_icon,
	&g_globe_icon,
	&g_go_icon,
	&g_hand_icon,
	&g_help_icon,
	&g_info_icon,
	&g_keyboard_icon,
	&g_keyboard2_icon,
	&g_laptop_icon,
	&g_notes_icon,
	&g_paint_icon,
	&g_serial_icon,
	&g_stop_icon,
	&g_text_file_icon,
	&g_warning_icon,
	&g_nanoshell_icon,
	&g_nanoshell16_icon,
	&g_nanoshell_shell_icon,
	&g_nanoshell_shell16_icon,
	&g_bomb_icon,
	&g_bomb2_icon,
	&g_file16_icon,
	&g_text_file16_icon,
	&g_execute_file16_icon,
	&g_folder_parent16_icon,
	&g_folder_settings_icon,
	&g_cabinet16_icon,
	&g_computer16_icon,
	&g_command_icon,
	&g_command16_icon,
	&g_error16_icon
};
#if 0
enum 
{
	ICON_NULL,
	ICON_CABINET,
	ICON_CHIP,
	ICON_CHIP_SQ,
	ICON_COMPUTER,
	ICON_COMPUTER_SHUTDOWN,
	ICON_DESKTOP,
	ICON_DRAW,
	ICON_EARTH,
	ICON_ERROR,
	ICON_EXECUTE_FILE,
	ICON_FILE,
	ICON_FILES,
	ICON_FOLDER,
	ICON_FOLDER_BLANK,
	ICON_FOLDER_MOVE,
	ICON_FOLDER_PARENT,
	ICON_FOLDER16_CLOSED,
	ICON_FOLDER16_OPEN,
	ICON_GLOBE,
	ICON_GO,
	ICON_HAND,
	ICON_HELP,
	ICON_INFO,
	ICON_KEYBOARD,
	ICON_KEYBOARD2,
	ICON_LAPTOP,
	ICON_NOTES,
	ICON_PAINT,
	ICON_SERIAL,
	ICON_STOP,
	ICON_TEXT_FILE,
	ICON_WARNING,
	ICON_NANOSHELL_LETTERS,
	ICON_NANOSHELL_LETTERS16,
	ICON_NANOSHELL,
	ICON_NANOSHELL16,
	ICON_BOMB,
	ICON_BOMB_SPIKEY,
	ICON_FILE16,
	ICON_TEXT_FILE16,
	ICON_EXECUTE_FILE16,
	ICON_FOLDER_PARENT16,
	//icons V1.1
	ICON_FOLDER_SETTINGS,
	ICON_CABINET16,
	ICON_COMPUTER16,
	ICON_COMMAND,
	ICON_COMMAND16,
	ICON_ERROR16,
	ICON_COUNT
};
#endif


STATIC_ASSERT(ARRAY_COUNT(g_iconTable) == ICON_COUNT, "Change this array if adding icons.");


void RenderIcon(IconType type, int x, int y)
{
	if (type >= ICON_COUNT || type <= ICON_NULL) return;
	
	Image* p = g_iconTable[type];
	VidBlitImage(p, x, y);
}
void RenderIconForceSize(IconType type, int x, int y, int size)
{
	if (type >= ICON_COUNT || type <= ICON_NULL) return;
	
	if (size == 16)
	{
		// Convert certain icons to their 16x counterparts:
		switch (type)
		{
			#define CASE(typo) case ICON_ ## typo: type = ICON_ ## typo ## 16; break;
			CASE(CABINET)
			CASE(COMPUTER)
			CASE(ERROR)
			case ICON_FOLDER: type = ICON_FOLDER16_CLOSED; break;
			CASE(FOLDER_PARENT)
			CASE(EXECUTE_FILE)
			CASE(FILE)
			CASE(TEXT_FILE)
			CASE(NANOSHELL)
			CASE(NANOSHELL_LETTERS)
			CASE(COMMAND)
			#undef CASE
		}
	}
	
	Image* p = g_iconTable[type];
	VidBlitImageResize(p, x, y, size, size);
}

