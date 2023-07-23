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
static void mbn_callback(rtk_widget *w, void *cls);
static void select_material(int midx);

static struct material *curmtl;
static int curmtl_idx;
#define MTL_PREVIEW_SZ 128
static cgm_vec2 mtlsph_uv[MTL_PREVIEW_SZ][MTL_PREVIEW_SZ];
static cgm_vec3 mtlsph_norm[MTL_PREVIEW_SZ][MTL_PREVIEW_SZ];

struct mtlw {
	rtk_widget *lb_mtlidx;
	rtk_widget *tx_mtlname;
	rtk_widget *bn_prev, *bn_next, *bn_add, *bn_del;
	rtk_widget *bn_kd, *bn_ks;
	rtk_widget *slider_shin;
	rtk_widget *preview;
};
static struct mtlw mtlw;


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

	box = rtk_create_window(mtlwin, "mtlselbox", 0, 0, 192, 8, 0);
	rtk_autosize(box, RTK_AUTOSZ_SIZE);
	rtk_win_layout(box, RTK_HBOX);

	icon = rtk_define_icon(icons, "leftarrow", 0, 32, 16, 16);
	mtlw.bn_prev = rtk_create_iconbutton(box, icon, mbn_callback);
	mtlw.lb_mtlidx = rtk_create_label(box, "0/0");
	w = rtk_create_textbox(box, "", 0);
	rtk_resize(w, 92, 1);
	mtlw.tx_mtlname = w;
	icon = rtk_define_icon(icons, "rightarrow", 16, 32, 16, 16);
	mtlw.bn_next = rtk_create_iconbutton(box, icon, mbn_callback);
	rtk_create_separator(box);
	mtlw.bn_add = rtk_create_iconbutton(box, tbn_icons[TBN_ADD], mbn_callback);
	mtlw.bn_del = rtk_create_iconbutton(box, tbn_icons[TBN_RM], mbn_callback);

	w = rtk_create_drawbox(mtlwin, MTL_PREVIEW_SZ, MTL_PREVIEW_SZ, mtlpreview_draw);
	mtlw.preview = w;

	rtk_create_separator(mtlwin);

	mtlw.bn_kd = rtk_create_button(mtlwin, "diffuse", mbn_callback);
	mtlw.bn_ks = rtk_create_button(mtlwin, "specular", mbn_callback);

	curmtl = 0;
	curmtl_idx = -1;

	/* pre-generate preview sphere */
	for(i=0; i<MTL_PREVIEW_SZ; i++) {
		float y = (1.0f - (float)i * 2.0f / (float)MTL_PREVIEW_SZ) * 1.1f;
		for(j=0; j<MTL_PREVIEW_SZ; j++) {
			float x = ((float)j * 2.0f / (float)MTL_PREVIEW_SZ - 1.0f) * 1.1f;
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
	cgm_vec3 dcol, scol, norm, vdir = {0, 0, 1};
	struct rayhit hit;
	struct object obj;
	struct light lt;

	rtk_get_absrect(w, &rect);

	assert(rect.width == MTL_PREVIEW_SZ);
	assert(rect.height == MTL_PREVIEW_SZ);

	if(!curmtl) {
		gui_fill(&rect, 0xff000000);
		return;
	}

	cgm_vcons(&lt.color, 1, 1, 1);
	lt.energy = 1;
	cgm_vcons(&lt.pos, -5, 5, 5);

	obj.mtl = curmtl;

	pix = framebuf + rect.y * win_width + rect.x;
	for(i=0; i<MTL_PREVIEW_SZ; i++) {
		for(j=0; j<MTL_PREVIEW_SZ; j++) {
			norm = mtlsph_norm[j][i];

			hit.pos = norm;
			hit.norm = norm;
			hit.uv = mtlsph_uv[j][i];
			hit.obj = &obj;

			dcol = curmtl->kd;
			cgm_vscale(&dcol, 0.05);
			scol.x = scol.y = scol.z = 0.0f;
			calc_light(&hit, &lt, &vdir, &dcol, &scol);
			r = (dcol.x + scol.x) * 255.0f;
			g = (dcol.y + scol.y) * 255.0f;
			b = (dcol.z + scol.z) * 255.0f;

			if(r > 255) r = 255;
			if(g > 255) g = 255;
			if(b > 255) b = 255;

			pix[j] = PACK_RGB32(r, g, b);
		}
		pix += win_width;
	}
}

static void mbn_callback(rtk_widget *w, void *cls)
{
	int num;
	struct material *mtl;

	if(w == mtlw.bn_prev) {
		if(!(num = scn_num_materials(scn))) {
			return;
		}
		select_material((curmtl_idx + num - 1) % num);

	} else if(w == mtlw.bn_next) {
		if(!(num = scn_num_materials(scn))) {
			return;
		}
		select_material((curmtl_idx + 1) % num);

	} else if(w == mtlw.bn_add) {
		if(!(mtl = malloc(sizeof *mtl))) {
			errormsg("failed to allocate new material!\n");
			return;
		}
		mtl_init(mtl);
		scn_add_material(scn, mtl);
		select_material(scn_num_materials(scn) - 1);
	}
}

static void select_material(int midx)
{
	char buf[64];
	int num_mtl = scn_num_materials(scn);

	if(midx < 0 || midx >= num_mtl) {
		return;
	}
	curmtl_idx = midx;
	curmtl = scn->mtl[midx];

	sprintf(buf, "%d/%d", midx + 1, num_mtl);
	rtk_set_text(mtlw.lb_mtlidx, buf);
	rtk_set_text(mtlw.tx_mtlname, curmtl->name);

	rtk_invalidate(mtlwin);
}
