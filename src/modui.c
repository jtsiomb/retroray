/*
RetroRay - integrated standalone vintage modeller/renderer
Copyright (C) 2023  John Tsiombikas <nuclear@mutantstargoat.com>

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
#include <assert.h>
#include "modui.h"
#include "app.h"
#include "rtk.h"
#include "gfxutil.h"
#include "rend.h"
#include "cgmath/cgmath.h"

static const char *tbn_icon_name[] = {
	"new", "open", "save", 0,
	"sel", "move", "rot", "scale", 0,
	"add", "remove", 0,
	"union", "isect", "diff", 0,
	"mtl", "rend", "rend-area", "viewrend", 0, "cfg"
};
static int tbn_icon_pos[][2] = {
	{0,0}, {16,0}, {32,0}, {-1,-1},
	{48,0}, {64,0}, {80,0}, {96,0}, {-1,-1},
	{112,0}, {112,16}, {-1,-1},
	{0,16}, {16,16}, {32,16}, {-1,-1},
	{48,16}, {64,16}, {64, 32}, {80,16}, {-1,-1}, {96,16}
};
static int tbn_istool[] = {
	0, 0, 0, 0,
	1, 1, 1, 1, 0,
	0, 0, 0,
	1, 1, 1, 0,
	0, 0, 1, 0, 0, 0
};
static rtk_icon *tbn_icons[NUM_TOOL_BUTTONS];
static rtk_widget *tbn_buttons[NUM_TOOL_BUTTONS];
static rtk_iconsheet *icons;

rtk_screen *modui;
rtk_widget *toolbar, *mtlwin;
rtk_widget *tools[NUM_TOOLS];

static int create_toolbar(void);
static int create_mtlwin(void);
static void mtlpreview_draw(rtk_widget *w, void *cls);

static struct material *curmtl;
static int curmtl_idx;
#define MTL_PREVIEW_SZ 128
static cgm_vec2 mtlsph_uv[MTL_PREVIEW_SZ][MTL_PREVIEW_SZ];
static cgm_vec3 mtlsph_norm[MTL_PREVIEW_SZ][MTL_PREVIEW_SZ];

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
	return 0;
}

static int create_toolbar(void)
{
	int i, toolidx;
	rtk_widget *w;

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
	assert(toolidx == NUM_TOOLS);

	return 0;
}

static int create_mtlwin(void)
{
	int i, j;
	rtk_widget *w, *box;
	rtk_icon *icon;

	if(!(mtlwin = rtk_create_window(0, "Materials", win_width / 2, 64, 256, 380,
					RTK_WIN_FRAME | RTK_WIN_MOVABLE | RTK_WIN_RESIZABLE))) {
		return -1;
	}
	rtk_add_window(modui, mtlwin);

	box = rtk_create_window(mtlwin, "mtlselbox", 0, 0, 192, 64, 0);
	rtk_win_layout(box, RTK_HBOX);

	icon = rtk_define_icon(icons, "leftarrow", xpos, ypos, 16, 16);
	w = rtk_create_iconbutton(box, icon, 0);
	w = rtk_create_textbox(box, "", 0);
	icon = rtk_define_icon(icons, "rightarrow", xpos, ypos, 16, 16);
	w = rtk_create_iconbutton(box, icon, 0);
	rtk_create_separator(box);
	w = rtk_create_iconbutton(box, tbn_icons[TBN_ADD], 0);


	rtk_create_drawbox(mtlwin, MTL_PREVIEW_SZ, MTL_PREVIEW_SZ, mtlpreview_draw);
	w = rtk_create_field(mtlwin, "Name:", 0);
	rtk_resize(w, 40, rtk_get_height(w));

	curmtl = 0;
	curmtl_idx = -1;

	/* pre-generate preview sphere */
	for(i=0; i<MTL_PREVIEW_SZ; i++) {
		float y = (float)i * 2.0f / (float)MTL_PREVIEW_SZ - 1.0f;
		for(j=0; j<MTL_PREVIEW_SZ; j++) {
			float x = (float)j * 2.0f / (float)MTL_PREVIEW_SZ - 1.0f;
			float r = sqrt(x * x + y * y);

			if(r < 1.0f) {
				float z = sqrt(1.0f - x * x - y * y);
				cgm_vcons(&mtlsph_norm[j][i], x, y, z);
			} else {
				cgm_vcons(&mtlsph_norm[j][i], 0, 0, 0);
				mtlsph_uv[j][i].x = mtlsph_uv[j][i].y = 0.0f;
			}
		}
	}

	return 0;
}

void modui_cleanup(void)
{
	rtk_free_iconsheet(icons);
	rtk_free_screen(modui);
}

static void mtlpreview_draw(rtk_widget *w, void *cls)
{
	int i, j, r, g, b;
	rtk_rect rect;
	uint32_t *pix;
	cgm_vec3 col, norm;
	cgm_ray ray;
	struct rayhit hit;
	struct object obj;

	rtk_get_absrect(w, &rect);

	assert(rect.width == MTL_PREVIEW_SZ);
	assert(rect.height == MTL_PREVIEW_SZ);

	if(!curmtl) {
		gui_fill(&rect, 0xff000000);
		return;
	}

	cgm_vcons(&ray.origin, 0, 0, -10);
	obj.mtl = curmtl;

	pix = framebuf + rect.y * win_width + rect.x;
	for(i=0; i<MTL_PREVIEW_SZ; i++) {
		for(j=0; j<MTL_PREVIEW_SZ; j++) {
			norm = mtlsph_norm[j][i];

			ray.dir = norm;
			cgm_vsub(&ray.dir, &ray.origin);

			hit.pos = norm;
			hit.norm = norm;
			hit.uv = mtlsph_uv[j][i];
			hit.obj = &obj;

			col = shade(&ray, &hit, 0);
			r = col.x / 255.0f;
			g = col.y / 255.0f;
			b = col.z / 255.0f;

			pix[j] = PACK_RGB32(r, g, b);
		}
		pix += win_width;
	}
}
