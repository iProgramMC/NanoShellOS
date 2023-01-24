#pragma once

#define TEXT_BOX_HEIGHT (22)

#define DEF_VBUILD_WID 640
#define DEF_VBUILD_HEI (TITLE_BAR_HEIGHT*2 + 6 + TEXT_BOX_HEIGHT + 6)
#define DEF_FDESIGN_WID 500
#define DEF_FDESIGN_HEI 400
#define DEF_CODEWIN_WID 500
#define DEF_CODEWIN_HEI 400
#define DEF_TOOLBOX_WID (24 * 2 + 6)
#define DEF_TOOLBOX_HEI (24 * 4 + 6 + TITLE_BAR_HEIGHT)

#define BUTTONDARK 0x808080
#define BUTTONMIDD BUTTON_MIDDLE_COLOR
#define BUTTONLITE 0xFFFFFF
#define BUTTONMIDC WINDOW_BACKGD_COLOR

#define GRABBER_SIZE (8)
#define GRID_WIDTH   (8)

#define D_OFFSET (50)

enum
{
	TOOL_CURSOR = -100,
	TOOL_SELECT,
};

enum eComboID
{
	CO_MENU_BAR = 1000,
	CO_EDITED_CTL,
	CO_EDITED_CHOOSE,
	CO_COMPILE,
	CO_STOP,
	CO_SHOW_CODE,
	CO_EDITED_FIELD_NAME,
	CO_EDITED_FIELD,
	CO_EDITED_FIELD_CHOOSE,
};

enum eResizeHandle
{
	HANDLE_NONE,
	HANDLE_BR, // bottom right
	HANDLE_BM, // bottom middle
	HANDLE_CR, // center right
	HANDLE_TL, // top left
	HANDLE_COUNT,
};

typedef struct tagDesCtl
{
	struct tagDesCtl *m_pPrev, *m_pNext;
	
	Rectangle m_rect;
	int       m_type;
	char      m_name[128];
	char      m_text[128];
	bool      m_sele;
	int       m_prm1, m_prm2;
	int       m_comboID;
}
DesignerControl;

void VbInitCode();
void VbPreviewWindow();
void VbSelectControlDialog();
void VbCreateMainWindow();
void VbCreateFormDesignerWindow();
void VbCreateToolboxWindow();
void VbCreateCodeWindow();
