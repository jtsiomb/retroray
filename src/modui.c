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
rtk_widget *toolbar, *mtlwin, *colordlg;
rtk_widget *tools[NUM_TOOLS];

int selobj;

static int create_toolbar(void);
static int create_mtlwin(void);
static int create_colordlg(void);
static void mtlpreview_draw(rtk_widget *w, void *cls);
static void draw_colorbn(rtk_widget *w, void *cls);
static void draw_huebox(rtk_widget *w, void *cls);
static void draw_huebar(rtk_widget *w, void *cls);
static void mbn_callback(rtk_widget *w, void *cls);
static void select_material(int midx);
static void colbn_handler(rtk_widget *w, void *cls);
static void colbox_mbutton(rtk_widget *w, int bn, int press, int x, int y);
static void colbox_drag(rtk_widget *w, int dx, int dy, int total_dx, int total_dy);

static struct material *curmtl;
static int curmtl_idx;
#define MTL_PREVIEW_SZ 128
static cgm_vec2 mtlsph_uv[MTL_PREVIEW_SZ][MTL_PREVIEW_SZ];
static cgm_vec3 mtlsph_norm[MTL_PREVIEW_SZ][MTL_PREVIEW_SZ];

struct mtlw {
	rtk_widget *lb_mtlidx;
	rtk_widget *tx_mtlname;
	rtk_widget *bn_prev, *bn_next, *bn_add, *bn_del, *bn_dup, *bn_assign;
	rtk_widget *bn_kd, *bn_ks, *bn_ke;
	rtk_widget *slider_shin;
	rtk_widget *preview;
};
static struct mtlw mtlw;

#define HUEBOX_SZ		128
#define HUEBAR_HEIGHT	20
struct colw {
	rtk_widget *huebox, *huebar;
	int rgb[3], hsv[3];
	cgm_vec3 *destcol;	/* where to put selected color */
	rtk_widget *updw;	/* which widget to invalidate on color change */
};
static struct colw colw;


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
	rtk_widget *w, *box, *vbox, *hbox;
	rtk_icon *icon;

	if(!(mtlwin = rtk_create_window(0, "Materials", win_width - 200, 50, 180, 420,
					RTK_WIN_FRAME | RTK_WIN_MOVABLE | RTK_WIN_RESIZABLE))) {
		return -1;
	}
	rtk_add_window(modui, mtlwin);

	box = rtk_create_hbox(mtlwin);

	icon = rtk_define_icon(icons, "leftarrow", 0, 32, 16, 16);
	mtlw.bn_prev = rtk_create_iconbutton(box, icon, mbn_callback);
	mtlw.lb_mtlidx = rtk_create_label(box, "0/0");
	w = rtk_create_textbox(box, "", 0);
	rtk_resize(w, 88, 1);
	mtlw.tx_mtlname = w;
	icon = rtk_define_icon(icons, "rightarrow", 16, 32, 16, 16);
	mtlw.bn_next = rtk_create_iconbutton(box, icon, mbn_callback);

	box = rtk_create_hbox(mtlwin);
	mtlw.preview = rtk_create_drawbox(box, MTL_PREVIEW_SZ, MTL_PREVIEW_SZ, mtlpreview_draw);

	vbox = rtk_create_vbox(box);
	mtlw.bn_add = rtk_create_iconbutton(vbox, tbn_icons[TBN_ADD], mbn_callback);
	mtlw.bn_del = rtk_create_iconbutton(vbox, tbn_icons[TBN_RM], mbn_callback);
	icon = rtk_define_icon(icons, "duplicate", 96, 32, 16, 16);
	mtlw.bn_dup = rtk_create_iconbutton(vbox, icon, mbn_callback);
	icon = rtk_define_icon(icons, "apply", 112, 32, 16, 16);
	mtlw.bn_assign = rtk_create_iconbutton(vbox, icon, mbn_callback);


	rtk_create_separator(mtlwin);

	/* diffuse color */
	hbox = rtk_create_hbox(mtlwin);
	rtk_create_label(hbox, "diffuse ......");
	mtlw.bn_kd = rtk_create_iconbutton(hbox, 0, mbn_callback);
	rtk_set_drawfunc(mtlw.bn_kd, draw_colorbn, (void*)offsetof(struct material, kd));
	rtk_autosize(mtlw.bn_kd, RTK_AUTOSZ_NONE);
	rtk_resize(mtlw.bn_kd, 18, 18);
	/* specular color */
	hbox = rtk_create_hbox(mtlwin);
	rtk_create_label(hbox, "specular ...");
	mtlw.bn_ks = rtk_create_iconbutton(hbox, 0, mbn_callback);
	rtk_set_drawfunc(mtlw.bn_ks, draw_colorbn, (void*)offsetof(struct material, ks));
	rtk_autosize(mtlw.bn_ks, RTK_AUTOSZ_NONE);
	rtk_resize(mtlw.bn_ks, 18, 18);
	/* emissive color */
	hbox = rtk_create_hbox(mtlwin);
	rtk_create_label(hbox, "emissive ...");
	mtlw.bn_ke = rtk_create_iconbutton(hbox, 0, mbn_callback);
	rtk_set_drawfunc(mtlw.bn_ke, draw_colorbn, (void*)offsetof(struct material, ke));
	rtk_autosize(mtlw.bn_ke, RTK_AUTOSZ_NONE);
	rtk_resize(mtlw.bn_ke, 18, 18);

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

static int create_colordlg(void)
{
	rtk_widget *w;

	if(!(colordlg = rtk_create_window(0, "Color selector", 100, 100, 200, 200,
					RTK_WIN_FRAME | RTK_WIN_MOVABLE))) {
		return -1;
	}
	rtk_win_layout(colordlg, RTK_NONE);
	rtk_add_window(modui, colordlg);

	colw.huebox = rtk_create_drawbox(colordlg, HUEBOX_SZ, HUEBOX_SZ, draw_huebox);
	rtk_set_mbutton_handler(colw.huebox, colbox_mbutton);
	rtk_set_drag_handler(colw.huebox, colbox_drag);
	rtk_move(colw.huebox, 5, 5);
	colw.huebar = rtk_create_drawbox(colordlg, HUEBOX_SZ, HUEBAR_HEIGHT, draw_huebar);
	rtk_set_mbutton_handler(colw.huebar, colbox_mbutton);
	rtk_set_drag_handler(colw.huebar, colbox_drag);
	rtk_move(colw.huebar, 5, HUEBOX_SZ + 10);

	w = rtk_create_button(colordlg, "Cancel", 0);
	rtk_set_callback(w, colbn_handler, 0);
	rtk_autosize(w, RTK_AUTOSZ_NONE);
	rtk_resize(w, 50, 20);
	rtk_move(w, 30, HUEBOX_SZ + HUEBAR_HEIGHT + 20);
	w = rtk_create_button(colordlg, "Ok", 0);
	rtk_set_callback(w, colbn_handler, (void*)1);
	rtk_autosize(w, RTK_AUTOSZ_NONE);
	rtk_resize(w, 50, 20);
	rtk_move(w, 90, HUEBOX_SZ + HUEBAR_HEIGHT + 20);

	rtk_hide(colordlg);
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

static void draw_colorbn(rtk_widget *w, void *cls)
{
	int offs = (intptr_t)cls;
	rtk_rect rect;
	cgm_vec3 *color;
	int r, g, b;

	if(!curmtl) {
		return;
	}

	color = (cgm_vec3*)((char*)curmtl + offs);
	r = color->x * 255.0f;
	g = color->y * 255.0f;
	b = color->z * 255.0f;

	rtk_get_absrect(w, &rect);
	rect.x += 3;
	rect.y += 3;
	rect.width -= 6;
	rect.height -= 6;

	gui_fill(&rect, PACK_RGB32(r, g, b));
}

static void hsv_to_rgb(int h, int s, int v, int *rptr, int *gptr, int *bptr)
{
	int r, g, b, c, x, hp, m, frac;

	c = (v * s) >> 8;
	hp = h * 256 / 60;
	frac = hp & 0x1ff;
	x = (c * (256 - abs(frac - 256))) >> 8;

	switch(hp >> 8) {
	case 0:
		r = c; g = x; b = 0;
		break;
	case 1:
		r = x; g = c; b = 0;
		break;
	case 2:
		r = 0; g = c; b = x;
		break;
	case 3:
		r = 0; g = x; b = c;
		break;
	case 4:
		r = x; g = 0; b = c;
		break;
	case 5:
	default:
		r = c; g = 0; b = x;
	}

	m = v - c;
	r += m;
	g += m;
	b += m;

	*rptr = r > 255 ? 255 : r;
	*gptr = g > 255 ? 255 : g;
	*bptr = b > 255 ? 255 : b;
}

static INLINE int min3(int a, int b, int c)
{
	if(a < b) {
		return a < c ? a : c;
	}
	return b < c ? b : c;
}
static INLINE int max3(int a, int b, int c)
{
	if(a > b) {
		return a > c ? a : c;
	}
	return b > c ? b : c;
}

static void rgb_to_hsv(int r, int g, int b, int *hptr, int *sptr, int *vptr)
{
	int h, s, v, xmax, xmin, c;

	xmax = max3(r, g, b);
	xmin = min3(r, g, b);
	c = xmax - xmin;

	v = xmax;
	if(c == 0) {
		h = 0;
	} else if(v == r) {
		h = (60 * ((((g - b) << 8) / c) % 6)) >> 8;
	} else if(v == g) {
		h = (60 * ((b - r) << 8) / c + 2) >> 8;
	} else {	/* v == b */
		h = (60 * (((r - g) << 8) / c + 4)) >> 8;
	}

	s = v == 0 ? 0 : (c << 8) / v;

	*hptr = h;
	*sptr = s;
	*vptr = v;
}

#define SVEQ(a, b)	(abs((a) - (b)) < 256 / HUEBOX_SZ)
#define HEQ(a, b)	(abs((a) - (b)) < 360 / HUEBOX_SZ)

static void draw_huebox(rtk_widget *w, void *cls)
{
	int i, j, hue, sat, val, r, g, b;
	rtk_rect rect;
	uint32_t *pptr;

	rtk_get_absrect(w, &rect);
	pptr = framebuf + rect.y * win_width + rect.x;

	hue = colw.hsv[0];

	for(i=0; i<HUEBOX_SZ; i++) {
		val = (HUEBOX_SZ - 1 - i) * 256 / HUEBOX_SZ;
		for(j=0; j<HUEBOX_SZ; j++) {
			sat = j * 256 / HUEBOX_SZ;

			hsv_to_rgb(hue, sat, val, &r, &g, &b);
			if(SVEQ(val, colw.hsv[2]) || SVEQ(sat, colw.hsv[1])) {
				r = ~r;
				g = ~g;
				b = ~b;
			}
			pptr[j] = PACK_RGB32(r, g, b);
		}
		pptr += win_width;
	}
}

static void draw_huebar(rtk_widget *w, void *cls)
{
	int i, j, hue, r, g, b;
	rtk_rect rect;
	uint32_t *fbptr, *pptr, col;

	rtk_get_absrect(w, &rect);
	fbptr = framebuf + rect.y * win_width + rect.x;

	for(i=0; i<HUEBOX_SZ; i++) {
		hue = i * 360 / HUEBOX_SZ;
		hsv_to_rgb(hue, 255, 255, &r, &g, &b);
		if(HEQ(hue, colw.hsv[0])) {
			r = ~r;
			g = ~g;
			b = ~b;
		}
		col = PACK_RGB32(r, g, b);

		pptr = fbptr++;
		for(j=0; j<HUEBAR_HEIGHT / 4; j++) {
			*pptr = col; pptr += win_width;
			*pptr = col; pptr += win_width;
			*pptr = col; pptr += win_width;
			*pptr = col; pptr += win_width;
		}
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

	} else if(w == mtlw.bn_dup) {
		if(!curmtl) return;
		if(!(mtl = malloc(sizeof *mtl))) {
			errormsg("failed to allocate new material!\n");
			return;
		}
		mtl_clone(mtl, curmtl);
		scn_add_material(scn, mtl);
		select_material(scn_num_materials(scn) - 1);

	} else if(w == mtlw.bn_assign) {
		if(!curmtl || selobj < 0) return;

		scn->objects[selobj]->mtl = curmtl;
		inval_vport();

	} else if(w == mtlw.bn_kd) {
		if(!curmtl) return;

		colw.destcol = &curmtl->kd;
		colw.updw = w;
		colw.rgb[0] = curmtl->kd.x * 255.0f;
		colw.rgb[1] = curmtl->kd.y * 255.0f;
		colw.rgb[2] = curmtl->kd.z * 255.0f;
		rgb_to_hsv(colw.rgb[0], colw.rgb[1], colw.rgb[2], colw.hsv, colw.hsv + 1, colw.hsv + 2);

		rtk_show(colordlg);
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

static void colbn_handler(rtk_widget *w, void *cls)
{
	if(cls) {
		hsv_to_rgb(colw.hsv[0], colw.hsv[1], colw.hsv[2], colw.rgb, colw.rgb + 1, colw.rgb + 2);
		colw.destcol->x = (float)colw.rgb[0] / 255.0f;
		colw.destcol->y = (float)colw.rgb[1] / 255.0f;
		colw.destcol->z = (float)colw.rgb[2] / 255.0f;
		rtk_invalidate(colw.updw);
	}
	rtk_hide(colordlg);
}

static int colbox_pressx, colbox_pressy;

static void colbox_mbutton(rtk_widget *w, int bn, int press, int x, int y)
{
	if(bn != 0 || !press) return;

	if(w == colw.huebar) {
		colw.hsv[0] = x * 360 / HUEBOX_SZ;
	} else if(w == colw.huebox) {
		colw.hsv[1] = x * 256 / HUEBOX_SZ;
		colw.hsv[2] = 255 - y * 256 / HUEBOX_SZ;
	}

	colbox_pressx = x;
	colbox_pressy = y;
}

static void colbox_drag(rtk_widget *w, int dx, int dy, int total_dx, int total_dy)
{
	int x = colbox_pressx + total_dx;
	int y = colbox_pressy + total_dy;

	if(x < 0 || y < 0 || x >= HUEBOX_SZ) return;

	if(w == colw.huebar) {
		if(y >= HUEBAR_HEIGHT) return;
		colw.hsv[0] = x * 360 / HUEBOX_SZ;

		rtk_invalidate(w);
		rtk_invalidate(colw.huebox);

	} else if(w == colw.huebox) {
		colw.hsv[1] = x * 256 / HUEBOX_SZ;
		colw.hsv[2] = 255 - y * 256 / HUEBOX_SZ;

		rtk_invalidate(w);
	}
}
