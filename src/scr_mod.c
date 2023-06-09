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
#include "gaw/gaw.h"
#include "app.h"
#include "rtk.h"
#include "scene.h"
#include "cmesh.h"
#include "meshgen.h"

enum {
	TBN_NEW, TBN_OPEN, TBN_SAVE, TBN_SEP1,
	TBN_SEL, TBN_MOVE, TBN_ROT, TBN_SCALE, TBN_SEP2,
	TBN_ADD, TBN_RM, TBN_SEP3,
	TBN_MTL, TBN_REND, TBN_VIEWREND, TBN_SEP4, TBN_CFG,

	NUM_TOOL_BUTTONS
};
static const char *tbn_icon_name[] = {
	"new", "open", "save", 0,
	"sel", "move", "rot", "scale", 0,
	"add", "remove", 0,
	"mtl", "rend", "viewrend", 0, "cfg"
};
static int tbn_icon_pos[][2] = {
	{0,0}, {16,0}, {32,0}, {-1,-1},
	{48,0}, {64,0}, {80,0}, {96,0}, {-1,-1},
	{112,0}, {112,16}, {-1,-1},
	{48,16}, {64,16}, {80,16}, {-1,-1}, {96,16}
};
static rtk_icon *tbn_icons[NUM_TOOL_BUTTONS];

#define TOOLBAR_HEIGHT	26


static int mdl_init(void);
static void mdl_destroy(void);
static int mdl_start(void);
static void mdl_stop(void);
static void mdl_display(void);
static void mdl_reshape(int x, int y);
static void mdl_keyb(int key, int press);
static void mdl_mouse(int bn, int press, int x, int y);
static void mdl_motion(int x, int y);

static void draw_grid(void);
static void tbn_callback(rtk_widget *w, void *cls);

struct app_screen scr_model = {
	"modeller",
	mdl_init, mdl_destroy,
	mdl_start, mdl_stop,
	mdl_display, mdl_reshape,
	mdl_keyb, mdl_mouse, mdl_motion
};

static rtk_widget *toolbar;
static rtk_iconsheet *icons;

static struct cmesh *mesh_sph;

static float cam_theta, cam_phi = 20, cam_dist = 8;


static int mdl_init(void)
{
	int i;
	rtk_widget *w;

	if(!(icons = rtk_load_iconsheet("data/icons.png"))) {
		errormsg("failed to load iconsheet\n");
		return -1;
	}
	for(i=0; i<NUM_TOOL_BUTTONS; i++) {
		if(tbn_icon_name[i]) {
			tbn_icons[i] = rtk_define_icon(icons, tbn_icon_name[i],
					tbn_icon_pos[i][0], tbn_icon_pos[i][1], 16, 16);
		} else {
			tbn_icons[i] = 0;
		}
	}

	if(!(toolbar = rtk_create_window(0, "toolbar", 0, 0, win_width, TOOLBAR_HEIGHT))) {
		return -1;
	}
	rtk_win_layout(toolbar, RTK_HBOX);

	for(i=0; i<NUM_TOOL_BUTTONS; i++) {
		if(!tbn_icons[i]) {
			rtk_create_separator(toolbar);
		} else {
			if(!(w = rtk_create_iconbutton(toolbar, tbn_icons[i], 0))) {
				return -1;
			}
			rtk_set_callback(w, tbn_callback, (void*)i);
		}
	}

	if(!(mesh_sph = cmesh_alloc())) {
		errormsg("failed to allocate sphere vis mesh\n");
		return -1;
	}
	gen_sphere(mesh_sph, 1.0f, 16, 8, 1.0f, 1.0f);
	return 0;
}

static void mdl_destroy(void)
{
	cmesh_free(mesh_sph);
	rtk_free_iconsheet(icons);
}

static int mdl_start(void)
{
	gaw_clear_color(0.125, 0.125, 0.125, 1);

	gaw_enable(GAW_DEPTH_TEST);
	gaw_enable(GAW_CULL_FACE);
	gaw_enable(GAW_LIGHTING);
	gaw_enable(GAW_LIGHT0);
	return 0;
}

static void mdl_stop(void)
{
}

static void mdl_display(void)
{
	int i, num;

	gaw_clear(GAW_COLORBUF | GAW_DEPTHBUF);

	rtk_draw_widget(toolbar);

	gaw_viewport(0, TOOLBAR_HEIGHT, win_width, win_height - TOOLBAR_HEIGHT);

	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_load_identity();
	gaw_translate(0, 0, -cam_dist);
	gaw_rotate(cam_phi, 1, 0, 0);
	gaw_rotate(cam_theta, 0, 1, 0);

	draw_grid();

	gaw_poly_wire();

	num = scn_num_objects(scn);
	for(i=0; i<num; i++) {
		struct object *obj = scn->objects[i];
		struct sphere *sph;

		calc_object_matrix(obj);
		gaw_push_matrix();
		gaw_mult_matrix(obj->xform);

		switch(obj->type) {
		case OBJ_SPHERE:
			sph = (struct sphere*)obj;
			gaw_scale(sph->rad, sph->rad, sph->rad);
			cmesh_draw(mesh_sph);
			break;

		default:
			break;
		}

		gaw_pop_matrix();
	}

	gaw_poly_gouraud();

	gaw_viewport(0, 0, win_width, win_height);
}

static void draw_grid(void)
{
	gaw_save();
	gaw_disable(GAW_LIGHTING);

	gaw_begin(GAW_LINES);
	gaw_color3f(0.5, 0, 0);
	gaw_vertex3f(0, 0, 0);
	gaw_vertex3f(-100, 0, 0);
	gaw_vertex3f(0, 0, 0);
	gaw_vertex3f(100, 0, 0);
	gaw_color3f(0, 0.5, 0);
	gaw_vertex3f(0, 0, 0);
	gaw_vertex3f(0, 0, -100);
	gaw_vertex3f(0, 0, 0);
	gaw_vertex3f(0, 0, 100);
	gaw_end();

	gaw_restore();
}

static void mdl_reshape(int x, int y)
{
	float aspect = (float)x / (float)(y - TOOLBAR_HEIGHT);

	gaw_matrix_mode(GAW_PROJECTION);
	gaw_load_identity();
	gaw_perspective(50, aspect, 0.5, 100.0);

	rtk_resize(toolbar, win_width, TOOLBAR_HEIGHT);
}

static void mdl_keyb(int key, int press)
{
	if(rtk_input_key(toolbar, key, press)) {
		app_redisplay();
		return;
	}
}

static int vpdrag;

static void mdl_mouse(int bn, int press, int x, int y)
{
	if(!vpdrag && rtk_input_mbutton(toolbar, bn, press, x, y)) {
		app_redisplay();
		return;
	}

	if(press) {
		vpdrag |= (1 << bn);
	} else {
		vpdrag &= ~(1 << bn);
	}
}

static void mdl_motion(int x, int y)
{
	int dx, dy;

	if(!vpdrag && rtk_input_mmotion(toolbar, x, y)) {
		app_redisplay();
		return;
	}

	dx = x - mouse_x;
	dy = y - mouse_y;

	if((dx | dy) == 0) return;

	if(mouse_state[0]) {
		cam_theta += dx * 0.5f;
		cam_phi += dy * 0.5f;
		if(cam_phi < -90) cam_phi = -90;
		if(cam_phi > 90) cam_phi = 90;
		app_redisplay();
	}

	if(mouse_state[2]) {
		cam_dist += dy * 0.1f;
		if(cam_dist < 0) cam_dist = 0;
		app_redisplay();
	}
}

static void add_sphere(void)
{
	struct object *obj;

	if(!(obj = create_object(OBJ_SPHERE))) {
		return;
	}
	scn_add_object(scn, obj);
}

static void tbn_callback(rtk_widget *w, void *cls)
{
	int id = (intptr_t)cls;

	switch(id) {
	case TBN_ADD:
		add_sphere();
		break;

	default:
		break;
	}
}
