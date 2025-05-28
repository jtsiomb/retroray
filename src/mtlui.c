#include <stddef.h>
#include <assert.h>
#include "modui.h"
#include "app.h"
#include "rtk.h"
#include "material.h"
#include "rend.h"
#include "gfxutil.h"

static void draw_colorbn(rtk_widget *w, void *cls);
static void mtlpreview_draw(rtk_widget *w, void *cls);
static void mbn_callback(rtk_widget *w, void *cls);
static void mtx_callback(rtk_widget *w, void *cls);
static void start_color_picker(cgm_vec3 *dest, rtk_widget *updw);
static void colbn_handler(rtk_widget *w, void *cls);
static void colbox_mbutton(rtk_widget *w, int bn, int press, int x, int y);
static void colbox_drag(rtk_widget *w, int dx, int dy, int total_dx, int total_dy);


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

	uint32_t preview_pixels[MTL_PREVIEW_SZ * MTL_PREVIEW_SZ];
	int preview_valid;
};
static struct mtlw mtlw;

static struct material *curmtl;
static int curmtl_idx;


struct colorwidget {
	rtk_widget *huebox, *huebar;
	int rgb[3], hsv[3];
	cgm_vec3 *destcol;	/* where to put selected color */
	rtk_widget *updw;	/* which widget to invalidate on color change */
};
static struct colorwidget colorw;


int create_mtlwin(void)
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
	w = rtk_create_textbox(box, "", mtx_callback);
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
	mtlw.preview_valid = 0;

	rtk_hide(mtlwin);
	return 0;
}


void select_material(int midx)
{
	char buf[64];
	int num_mtl = scn_num_materials(scn);

	if(midx < 0 || midx >= num_mtl) {
		curmtl_idx = -1;
		curmtl = 0;
		return;
	}
	curmtl_idx = midx;
	curmtl = scn->mtl[midx];

	sprintf(buf, "%d/%d", midx + 1, num_mtl);
	rtk_set_text(mtlw.lb_mtlidx, buf);
	rtk_set_text(mtlw.tx_mtlname, curmtl->name);

	rtk_invalidate(mtlwin);
	mtlw.preview_valid = 0;
}

static void mtlpreview_draw(rtk_widget *w, void *cls)
{
	int i, j, r, g, b;
	rtk_rect rect;
	uint32_t *pix, *savpix;
	cgm_vec3 dcol, scol, norm, vdir = {0, 0, 1};
	struct rayhit hit;
	struct object obj;
	struct light lt;

	rtk_get_absrect(w, &rect);

	assert(rect.width == MTL_PREVIEW_SZ);
	assert(rect.height == MTL_PREVIEW_SZ);

	if(!curmtl) {
		gui_fill(&rect, PACK_RGB32(0, 0, 0));
		mtlw.preview_valid = 0;
		return;
	}

	pix = framebuf + rect.y * win_width + rect.x;
	savpix = mtlw.preview_pixels;

	if(mtlw.preview_valid) {
		struct rtk_icon icon = {0, MTL_PREVIEW_SZ, MTL_PREVIEW_SZ, MTL_PREVIEW_SZ};
		icon.pixels = savpix;
		gui_blit(rect.x, rect.y, &icon);
	} else {
		cgm_vcons(&lt.color, 1, 1, 1);
		lt.energy = 1;
		lt.shadows = 0;
		cgm_vcons(&lt.pos, -5, 5, 5);
		obj.mtl = curmtl;

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

				pix[j] = savpix[j] = PACK_RGB32(r, g, b);
			}
			pix += win_width;
			savpix += MTL_PREVIEW_SZ;
		}

		mtlw.preview_valid = 1;
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


static void mtx_callback(rtk_widget *w, void *cls)
{
	const char *newname = rtk_get_text(w);

	if(!curmtl || !newname || !*newname || strcmp(newname, curmtl->name) == 0) {
		return;
	}
	mtl_set_name(curmtl, newname);
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
		if(curmtl) {
			start_color_picker(&curmtl->kd, w);
		}

	} else if(w == mtlw.bn_ks) {
		if(curmtl) {
			start_color_picker(&curmtl->ks, w);
		}

	} else if(w == mtlw.bn_ke) {
		if(curmtl) {
			start_color_picker(&curmtl->ke, w);
		}

	}
}

static void start_color_picker(cgm_vec3 *dest, rtk_widget *updw)
{
	colorw.destcol = dest;
	colorw.updw = updw;

	colorw.rgb[0] = dest->x * 255.0f;
	colorw.rgb[1] = dest->y * 255.0f;
	colorw.rgb[2] = dest->z * 255.0f;
	rgb_to_hsv(colorw.rgb[0], colorw.rgb[1], colorw.rgb[2], colorw.hsv, colorw.hsv + 1, colorw.hsv + 2);

	rtk_show_modal(colordlg);
}



/* color picker widget, and color button */
#define HUEBOX_SZ		128
#define HUEBAR_HEIGHT	20

int create_colordlg(void)
{
	rtk_widget *w;

	if(!(colordlg = rtk_create_window(0, "Color selector", 100, 100, 200, 200,
					RTK_WIN_FRAME | RTK_WIN_MOVABLE))) {
		return -1;
	}
	rtk_win_layout(colordlg, RTK_NONE);
	rtk_add_window(modui, colordlg);

	colorw.huebox = rtk_create_drawbox(colordlg, HUEBOX_SZ, HUEBOX_SZ, draw_huebox);
	rtk_set_mbutton_handler(colorw.huebox, colbox_mbutton);
	rtk_set_drag_handler(colorw.huebox, colbox_drag);
	rtk_move(colorw.huebox, 5, 5);
	colorw.huebar = rtk_create_drawbox(colordlg, HUEBOX_SZ, HUEBAR_HEIGHT, draw_huebar);
	rtk_set_mbutton_handler(colorw.huebar, colbox_mbutton);
	rtk_set_drag_handler(colorw.huebar, colbox_drag);
	rtk_move(colorw.huebar, 5, HUEBOX_SZ + 10);

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


static void colbn_handler(rtk_widget *w, void *cls)
{
	if(cls) {
		hsv_to_rgb(colorw.hsv[0], colorw.hsv[1], colorw.hsv[2], colorw.rgb, colorw.rgb + 1, colorw.rgb + 2);
		colorw.destcol->x = (float)colorw.rgb[0] / 255.0f;
		colorw.destcol->y = (float)colorw.rgb[1] / 255.0f;
		colorw.destcol->z = (float)colorw.rgb[2] / 255.0f;
		rtk_invalidate(colorw.updw);
		mtlw.preview_valid = 0;
	}
	rtk_hide(colordlg);
}

#define SVEQ(a, b)	(abs((a) - (b)) < 256 / HUEBOX_SZ)
#define HEQ(a, b)	(abs((a) - (b)) < 360 / HUEBOX_SZ)

void draw_huebox(rtk_widget *w, void *cls)
{
	static uint32_t pixels[HUEBOX_SZ * HUEBOX_SZ];
	static rtk_icon icon = {0, HUEBOX_SZ, HUEBOX_SZ, HUEBOX_SZ, pixels};
	int i, j, hue, sat, val, r, g, b;
	rtk_rect rect;
	uint32_t *pptr = icon.pixels;

	rtk_get_absrect(w, &rect);

	hue = colorw.hsv[0];

	for(i=0; i<HUEBOX_SZ; i++) {
		val = (HUEBOX_SZ - 1 - i) * 256 / HUEBOX_SZ;
		for(j=0; j<HUEBOX_SZ; j++) {
			sat = j * 256 / HUEBOX_SZ;

			hsv_to_rgb(hue, sat, val, &r, &g, &b);
			if(SVEQ(val, colorw.hsv[2]) || SVEQ(sat, colorw.hsv[1])) {
				r = ~r;
				g = ~g;
				b = ~b;
			}
			pptr[j] = PACK_RGB32(r, g, b);
		}
		pptr += HUEBOX_SZ;
	}

	gui_blit(rect.x, rect.y, &icon);
}

void draw_huebar(rtk_widget *w, void *cls)
{
	static uint32_t pixels[HUEBOX_SZ * HUEBAR_HEIGHT];
	static rtk_icon icon = {0, HUEBOX_SZ, HUEBAR_HEIGHT, HUEBOX_SZ, pixels};
	int i, j, hue, r, g, b;
	rtk_rect rect;
	uint32_t *fbptr, *pptr, col;

	rtk_get_absrect(w, &rect);
	fbptr = pixels;

	for(i=0; i<HUEBOX_SZ; i++) {
		hue = i * 360 / HUEBOX_SZ;
		hsv_to_rgb(hue, 255, 255, &r, &g, &b);
		if(HEQ(hue, colorw.hsv[0])) {
			r = ~r;
			g = ~g;
			b = ~b;
		}
		col = PACK_RGB32(r, g, b);

		pptr = fbptr++;
		for(j=0; j<HUEBAR_HEIGHT / 4; j++) {
			*pptr = col; pptr += HUEBOX_SZ;
			*pptr = col; pptr += HUEBOX_SZ;
			*pptr = col; pptr += HUEBOX_SZ;
			*pptr = col; pptr += HUEBOX_SZ;
		}
	}

	gui_blit(rect.x, rect.y, &icon);
}

static int colbox_pressx, colbox_pressy;

static void colbox_mbutton(rtk_widget *w, int bn, int press, int x, int y)
{
	if(bn != 0 || !press) return;

	if(w == colorw.huebar) {
		colorw.hsv[0] = x * 360 / HUEBOX_SZ;
	} else if(w == colorw.huebox) {
		colorw.hsv[1] = x * 256 / HUEBOX_SZ;
		colorw.hsv[2] = 255 - y * 256 / HUEBOX_SZ;
	}

	colbox_pressx = x;
	colbox_pressy = y;
}

static void colbox_drag(rtk_widget *w, int dx, int dy, int total_dx, int total_dy)
{
	int x = colbox_pressx + total_dx;
	int y = colbox_pressy + total_dy;

	if(x < 0 || y < 0 || x >= HUEBOX_SZ) return;

	if(w == colorw.huebar) {
		if(y >= HUEBAR_HEIGHT) return;
		colorw.hsv[0] = x * 360 / HUEBOX_SZ;

		rtk_invalidate(w);
		rtk_invalidate(colorw.huebox);

	} else if(w == colorw.huebox) {
		colorw.hsv[1] = x * 256 / HUEBOX_SZ;
		colorw.hsv[2] = 255 - y * 256 / HUEBOX_SZ;

		rtk_invalidate(w);
	}
}
