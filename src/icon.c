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
};

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
	
	Image* p = g_iconTable[type];
	VidBlitImageResize(p, x, y, size, size);
}

