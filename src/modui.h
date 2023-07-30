#ifndef MODUI_H_
#define MODUI_H_

#include "cgmath/cgmath.h"
#include "rtk.h"

#define TOOLBAR_HEIGHT	26

/* tools */
enum {
	TOOL_SEL, TOOL_MOVE, TOOL_ROT, TOOL_SCALE,
	TOOL_UNION, TOOL_ISECT, TOOL_DIFF, TOOL_REND_AREA,
	NUM_TOOLS
};

/* toolbar buttons */
enum {
	TBN_NEW, TBN_OPEN, TBN_SAVE, TBN_SEP1,
	TBN_SEL, TBN_MOVE, TBN_ROT, TBN_SCALE, TBN_SEP2, TBN_XYZ, TBN_SEP6,
	TBN_ADD, TBN_RM, TBN_SEP3,
	TBN_UNION, TBN_ISECT, TBN_DIFF, TBN_SEP4,
	TBN_MTL, TBN_REND, TBN_REND_AREA, TBN_VIEWREND, TBN_SEP5, TBN_CFG,

	NUM_TOOL_BUTTONS
};

extern rtk_screen *modui;
extern rtk_widget *toolbar, *objmenu, *xyzmenu, *mtlwin, *colordlg;
extern rtk_widget *tools[NUM_TOOLS];

extern int selobj;
extern unsigned int axismask;

int modui_init(void);
void modui_cleanup(void);

void tbn_callback(rtk_widget *w, void *cls);

void set_axismask(unsigned int mask);

/* scr_mod.c */
void inval_vport(void);
cgm_vec3 get_view_pos(void);

#endif	/* MODUI_H_ */
