/*
RetroRay - integrated standalone vintage modeller/renderer
Copyright (C) 2023-2025  John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stddef.h>
#include <assert.h>
#include "modui.h"
#include "app.h"
#include "rtk.h"
#include "gfxutil.h"
#include "rend.h"
#include "cgmath/cgmath.h"
#include "util.h"

static const char *tbn_icon_name[] = {
	"new", "open", "save", 0,
	"sel", "move", "rot", "scale", 0, "xyz", 0,
	"add", "remove", 0,
	"union", "isect", "diff", 0,
	"mtl", "rend", "rend-area", "viewrend", 0, "cfg"
};
static int tbn_icon_pos[][2] = {
	{0,0}, {16,0}, {32,0}, {-1,-1},
	{48,0}, {64,0}, {80,0}, {96,0}, {-1,-1}, {0, 64}, {-1,-1},
	{112,0}, {112,16}, {-1,-1},
	{0,16}, {16,16}, {32,16}, {-1,-1},
	{48,16}, {64,16}, {64, 32}, {80,16}, {-1,-1}, {96,16}
};
static int tbn_istool[] = {
	0, 0, 0, 0,
	1, 1, 1, 1, 0, 0, 0,
	0, 0, 0,
	1, 1, 1, 0,
	0, 0, 1, 0, 0, 0
};
static rtk_widget *tbn_buttons[NUM_TOOL_BUTTONS];
rtk_icon *tbn_icons[NUM_TOOL_BUTTONS];
rtk_iconsheet *icons;

rtk_screen *modui;
rtk_widget *toolbar, *objmenu, *xyzmenu, *mtlwin, *colordlg;
rtk_widget *tools[NUM_TOOLS];

int selobj;
unsigned int axismask;

static int create_toolbar(void);
static void objadd_handler(rtk_widget *w, void *cls);
static void addlight_handler(rtk_widget *w, void *cls);
static void xyz_handler(rtk_widget *w, void *cls);


int modui_init(void)
{
	if(!(icons = rtk_load_iconsheet("data/icons.png"))) {
		errormsg("failed to load iconsheet\n");
		return -1;
	}

	if(!(modui = rtk_create_screen())) {
		return -1;
	}

	if(create_toolbar() == -1) {
		return -1;
	}
	if(create_mtlwin() == -1) {
		return -1;
	}
	if(create_colordlg() == -1) {
		return -1;
	}
	return 0;
}

static int create_toolbar(void)
{
	int i, toolidx;
	rtk_widget *w;
	rtk_icon *icon;

	for(i=0; i<NUM_TOOL_BUTTONS; i++) {
		if(tbn_icon_name[i]) {
			tbn_icons[i] = rtk_define_icon(icons, tbn_icon_name[i],
					tbn_icon_pos[i][0], tbn_icon_pos[i][1], 16, 16);
		} else {
			tbn_icons[i] = 0;
		}
	}

	if(!(toolbar = rtk_create_window(0, "toolbar", 0, 0, win_width, TOOLBAR_HEIGHT, 0))) {
		return -1;
	}
	rtk_add_window(modui, toolbar);
	rtk_win_layout(toolbar, RTK_HBOX);

	toolidx = 0;
	for(i=0; i<NUM_TOOL_BUTTONS; i++) {
		if(!tbn_icons[i]) {
			rtk_create_separator(toolbar);
		} else {
			if(!(w = rtk_create_iconbutton(toolbar, tbn_icons[i], 0))) {
				return -1;
			}
			if(tbn_icon_name[i]) {
				rtk_set_text(w, tbn_icon_name[i]);
			}
			tbn_buttons[i] = w;
			rtk_set_callback(w, tbn_callback, (void*)(intptr_t)i);
			if(tbn_istool[i]) {
				rtk_bn_mode(w, RTK_TOGGLEBN);
				tools[toolidx++] = w;
			}
			if(i == TBN_SEL) {
				rtk_set_value(w, 1);
			}
		}
	}

	/* constraint menu */
	if(!(xyzmenu = rtk_create_window(0, "xyzmenu", TBN_XYZ * 24 - 4, TOOLBAR_HEIGHT, 22, 22 * 7, 0))) {
		return -1;
	}
	rtk_add_window(modui, xyzmenu);
	rtk_win_layout(xyzmenu, RTK_VBOX);
	rtk_padding(xyzmenu, 0);
	for(i=0; i<7; i++) {
		static const char *names[] = {"xyz", "x", "y", "z", "yz", "xz", "xy"};


		if(i == 0) {
			icon = rtk_lookup_icon(icons, "xyz");
		} else {
			icon = rtk_define_icon(icons, names[i], i * 16, 64, 16, 16);
		}
		w = rtk_create_iconbutton(xyzmenu, icon, 0);
		rtk_set_callback(w, xyz_handler, (void*)(intptr_t)i);
		rtk_bn_mode(w, RTK_TOGGLEBN);

		if(i == 0) rtk_set_value(w, 1);
	}

	rtk_hide(xyzmenu);

	/* object creation menu */
	if(!(objmenu = rtk_create_window(0, "objmenu", TBN_ADD * 24 - 4, TOOLBAR_HEIGHT, 22, 22 * 5, 0))) {
		return -1;
	}
	rtk_add_window(modui, objmenu);
	rtk_win_layout(objmenu, RTK_VBOX);
	rtk_padding(objmenu, 0);

	icon = rtk_define_icon(icons, "obj_sphere", 0, 48, 16, 16);
	w = rtk_create_iconbutton(objmenu, icon, 0);
	rtk_set_callback(w, objadd_handler, (void*)OBJ_SPHERE);
	icon = rtk_define_icon(icons, "obj_box", 16, 48, 16, 16);
	w = rtk_create_iconbutton(objmenu, icon, 0);
	rtk_set_callback(w, objadd_handler, (void*)OBJ_BOX);
	icon = rtk_define_icon(icons, "light", 112, 48, 16, 16);
	w = rtk_create_iconbutton(objmenu, icon, 0);
	rtk_set_callback(w, addlight_handler, 0);

	rtk_hide(objmenu);

	return 0;
}

void modui_cleanup(void)
{
	rtk_free_iconsheet(icons);
	rtk_free_screen(modui);
}

void set_axismask(unsigned int mask)
{
	int i, bnidx;
	rtk_widget *w;
	static const int maskidx[] = {-1, 1, 2, 6, 3, 5, 4, 0};

	if(!mask || mask >= 8) {
		mask = 0xff;
		bnidx = 0;
	} else {
		bnidx = maskidx[mask];
	}
	axismask = mask;

	for(i=0; i<7; i++) {
		w = rtk_win_child(xyzmenu, i);
		if(i == bnidx) {
			rtk_bn_set_icon(tbn_buttons[TBN_XYZ], rtk_bn_get_icon(w));
		} else {
			rtk_set_value(w, 0);
		}
	}
}

static void objadd_handler(rtk_widget *w, void *cls)
{
	struct object *obj = 0;
	int type = (intptr_t)cls;
	int newidx = scn_num_objects(scn);

	switch(type) {
	case OBJ_SPHERE:
		obj = create_object(OBJ_SPHERE);
		break;

	case OBJ_BOX:
		obj = create_object(OBJ_BOX);
		break;

	default:
		break;
	}

	rtk_hide(objmenu);

	if(!obj) return;

	scn_add_object(scn, obj);
	selobj = newidx;
	inval_vport();
}

static void addlight_handler(rtk_widget *w, void *cls)
{
	struct light *lt;
	int newidx = scn_num_objects(scn);

	rtk_hide(objmenu);

	if(!(lt = create_light())) {
		return;
	}
	lt->pos = get_view_pos();

	scn_add_light(scn, lt);
	selobj = newidx;
	inval_vport();
}

static void xyz_handler(rtk_widget *w, void *cls)
{
	int bnidx = (intptr_t)cls;
	static const unsigned int mask[] = {0xff, 1, 2, 4, 6, 5, 3};

	rtk_hide(xyzmenu);

	set_axismask(mask[bnidx]);
}
