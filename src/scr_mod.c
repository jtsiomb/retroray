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
#include "geom.h"
#include "cmesh.h"
#include "meshgen.h"
#include "font.h"
#include "rend.h"
#include "modui.h"

static int vpdirty, vpnav;
static rtk_rect totalrend;


static int mdl_init(void);
static void mdl_destroy(void);
static int mdl_start(void);
static void mdl_stop(void);
static void mdl_display(void);
static void mdl_reshape(int x, int y);
static void mdl_keyb(int key, int press);
static void mdl_mouse(int bn, int press, int x, int y);
static void mdl_motion(int x, int y);

static void draw_object(struct object *obj);
static void setup_material(struct material *mtl);
static void draw_grid(void);

static void act_settool(int tidx);
static void act_addobj(void);
static void act_rmobj(void);

static void moveobj(struct object *obj, int px0, int py0, int px1, int py1);

void inval_vport(void);


struct app_screen scr_model = {
	"modeller",
	mdl_init, mdl_destroy,
	mdl_start, mdl_stop,
	mdl_display, mdl_reshape,
	mdl_keyb, mdl_mouse, mdl_motion
};


static struct cmesh *mesh_sph;

static float cam_theta, cam_phi = 20, cam_dist = 8;
static float view_matrix[16], proj_matrix[16];
static float view_matrix_inv[16], proj_matrix_inv[16];
static int viewport[4];
static cgm_ray pickray;

static int cur_tool, prev_tool = -1;
static int selobj = -1;

static rtk_rect rband;
static int rband_valid;

static int rendering;
static rtk_rect rendrect;


static int mdl_init(void)
{
	if(modui_init() == -1) {
		errormsg("failed to initialize modeller UI\n");
		return -1;
	}

	if(!(mesh_sph = cmesh_alloc())) {
		errormsg("failed to allocate sphere vis mesh\n");
		return -1;
	}
	gen_sphere(mesh_sph, 1.0f, 16, 8, 1.0f, 1.0f);

	vpdirty = 1;
	return 0;
}

static void mdl_destroy(void)
{
	cmesh_free(mesh_sph);
	modui_cleanup();
}

static int mdl_start(void)
{
	gaw_clear_color(0.125, 0.125, 0.125, 1);

	gaw_enable(GAW_DEPTH_TEST);
	gaw_enable(GAW_CULL_FACE);
	gaw_enable(GAW_LIGHTING);
	gaw_enable(GAW_LIGHT0);

	rend_pan(0, -TOOLBAR_HEIGHT);
	return 0;
}

static void mdl_stop(void)
{
}

static void mdl_display(void)
{
	int i, num;

	/* viewport */
	if(vpdirty || vpnav) {
		gaw_clear(GAW_COLORBUF | GAW_DEPTHBUF);

		gaw_matrix_mode(GAW_MODELVIEW);
		gaw_load_identity();
		gaw_translate(0, 0, -cam_dist);
		gaw_rotate(cam_phi, 1, 0, 0);
		gaw_rotate(cam_theta, 0, 1, 0);
		gaw_get_modelview(view_matrix);
		cgm_mcopy(view_matrix_inv, view_matrix);
		cgm_minverse(view_matrix_inv);

		draw_grid();

		num = scn_num_objects(scn);
		for(i=0; i<num; i++) {
			setup_material(scn->objects[i]->mtl);

			if(i == selobj) {
				gaw_zoffset(1);
				gaw_enable(GAW_POLYGON_OFFSET);
				draw_object(scn->objects[i]);
				gaw_disable(GAW_POLYGON_OFFSET);

				gaw_save();
				gaw_disable(GAW_LIGHTING);
				gaw_poly_wire();
				gaw_color3f(0, 1, 0);
				draw_object(scn->objects[i]);
				gaw_poly_gouraud();
				gaw_restore();
			} else {
				draw_object(scn->objects[i]);
			}
		}
		vpdirty = 0;

		/* dirty all GUI windows */
		rtk_invalidate(toolbar);
		rtk_invalidate(mtlwin);
	}

	/* render layer */
	if(rendering) {
		if(!render(framebuf)) {
			rendering = 0;
		}
		app_redisplay(rendrect.x, rendrect.y, rendrect.width, rendrect.height);
	}

	/* GUI */
	rtk_draw_widget(toolbar);
	rtk_draw_widget(mtlwin);
}

static void draw_object(struct object *obj)
{
	struct sphere *sph;

	if(!obj->xform_valid) {
		calc_object_matrix(obj);
	}
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

static void setup_material(struct material *mtl)
{
	gaw_mtl_diffuse(mtl->kd.x, mtl->kd.y, mtl->kd.z, 1.0f);
	gaw_mtl_specular(mtl->ks.x, mtl->ks.y, mtl->ks.z, mtl->shin);
	gaw_mtl_emission(mtl->ke.x, mtl->ke.y, mtl->ke.z);
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

	viewport[0] = 0;
	viewport[1] = TOOLBAR_HEIGHT;
	viewport[2] = x;
	viewport[3] = y - TOOLBAR_HEIGHT;
	gaw_viewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	gaw_matrix_mode(GAW_PROJECTION);
	gaw_load_identity();
	gaw_perspective(50, aspect, 0.5, 100.0);
	gaw_get_projection(proj_matrix);
	cgm_mcopy(proj_matrix_inv, proj_matrix);
	cgm_minverse(proj_matrix_inv);

	rtk_resize(toolbar, win_width, TOOLBAR_HEIGHT);

	inval_vport();
}

static void mdl_keyb(int key, int press)
{
	if(rtk_input_key(modui, key, press)) {
		return;
	}

	if(press) {
		switch(key) {
		case 27:
			act_settool(TOOL_SEL);
			break;
		case 'g':
			act_settool(TOOL_MOVE);
			break;
		case 'r':
			act_settool(TOOL_ROT);
			break;
		case 's':
			act_settool(TOOL_SCALE);
			break;

		case KEY_F6:
			act_settool(TOOL_REND_AREA);
			break;

		case KEY_DEL:
			act_rmobj();
			break;

		default:
			break;
		}
	}
}

static int vpdrag;

static void mdl_mouse(int bn, int press, int x, int y)
{
	struct rayhit hit;
	if(!vpdrag && rtk_input_mbutton(modui, bn, press, x, y)) {
		return;
	}

	if(press) {
		rband_valid = 0;
		rband.x = x;
		rband.y = y;
		vpdrag |= (1 << bn);
		if(modkeys) {
			vpnav |= (1 << bn);
			dbgmsg("vpnav on\n");
		}
	} else {
		vpnav &= ~(1 << bn);
		if(!vpnav) {
			dbgmsg("vpnav off\n");
		}
		vpdrag &= ~(1 << bn);

		if(rband_valid) {
			rband_valid = 0;
			app_rband(0, 0, 0, 0);

			if(cur_tool == TOOL_REND_AREA) {
				if(rband.width && rband.height) {
					rendering = 1;
					rend_size(win_width, win_height);
					rtk_fix_rect(&rband);
					rendrect = rband;
					rend_begin(rband.x, rband.y, rband.width, rband.height);
					app_redisplay(rband.x, rband.y, rband.width, rband.height);

					if(totalrend.width) {
						rtk_rect_union(&totalrend, &rband);
					} else {
						totalrend = rband;
					}
				}
			}

		} else if(bn == 0 && x == rband.x && y == rband.y) {
			primray(&pickray, x, y);
			if(scn_intersect(scn, &pickray, &hit)) {
				int newsel = scn_object_index(scn, hit.obj);
				if(newsel != selobj) {
					selobj = newsel;
					inval_vport();
				}
			} else {
				if(selobj != -1) {
					inval_vport();
				}
				selobj = -1;
			}
		}
	}
}

static void mdl_motion(int x, int y)
{
	int dx, dy;

	if(!vpdrag && rtk_input_mmotion(modui, x, y)) {
		return;
	}

	dx = x - mouse_x;
	dy = y - mouse_y;

	if(vpnav) {
		dbgmsg("vpnav %d,%d\n", dx, dy);
		/* navigation */
		if(mouse_state[0]) {
			cam_theta += dx * 0.5f;
			cam_phi += dy * 0.5f;
			if(cam_phi < -90) cam_phi = -90;
			if(cam_phi > 90) cam_phi = 90;
			inval_vport();
		}

		if(mouse_state[2]) {
			cam_dist += dy * 0.1f;
			if(cam_dist < 0) cam_dist = 0;
			inval_vport();
		}
	} else {
		if(mouse_state[0]) {
			switch(cur_tool) {
			case TOOL_SEL:
			case TOOL_REND_AREA:
				if(rband.x != x || rband.y != y) {
					rband.width = x - rband.x;
					rband.height = y - rband.y;
					rband_valid = 1;
					app_rband(rband.x, rband.y, rband.width, rband.height);
				}
				break;

			case TOOL_MOVE:
				if(selobj >= 0) {
					struct object *obj = scn->objects[selobj];
					moveobj(obj, mouse_x, mouse_y, x, y);
				}
				break;

			default:
				break;
			}
		}
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

void tbn_callback(rtk_widget *w, void *cls)
{
	int id = (intptr_t)cls;

	switch(id) {
	case TBN_NEW:
		scn_clear(scn);
		inval_vport();
		break;

	case TBN_SEL:
	case TBN_MOVE:
	case TBN_ROT:
	case TBN_SCALE:
		act_settool(id - TBN_SEL);
		break;
	case TBN_UNION:
	case TBN_ISECT:
	case TBN_DIFF:
		act_settool(id - TBN_UNION + TOOL_UNION);
		break;

	case TBN_MTL:
		if(rtk_visible(mtlwin)) {
			rtk_hide(mtlwin);
			inval_vport();
		} else {
			rtk_show(mtlwin);
		}
		break;

	case TBN_REND_AREA:
		act_settool(TOOL_REND_AREA);
		break;

	case TBN_ADD:
		act_addobj();
		break;

	case TBN_RM:
		act_rmobj();
		break;

	default:
		break;
	}
}

static void act_settool(int tidx)
{
	int i;
	rtk_rect r;

	if(tidx == cur_tool) return;

	if(cur_tool == TOOL_REND_AREA) {
		totalrend.width = 0;
		app_redisplay(totalrend.x, totalrend.y, totalrend.width, totalrend.height);
		inval_vport();
	}

	prev_tool = cur_tool;
	cur_tool = tidx;
	for(i=0; i<NUM_TOOLS; i++) {
		if(i == cur_tool) {
			if(!rtk_get_value(tools[i])) {
				rtk_set_value(tools[i], 1);
				rtk_get_rect(tools[i], &r);
			}
		} else {
			if(rtk_get_value(tools[i])) {
				rtk_set_value(tools[i], 0);
				rtk_get_rect(tools[i], &r);
			}
		}
	}
}

static void act_addobj(void)
{
	int idx = scn_num_objects(scn);
	add_sphere();
	selobj = idx;

	inval_vport();
}

static void act_rmobj(void)
{
	if(selobj >= 0) {
		scn_rm_object(scn, selobj);
		selobj = -1;
		inval_vport();
	}
}


void primray(cgm_ray *ray, int x, int y)
{
	float nx, ny;
	cgm_vec3 npos, farpt;
	float inv_pv[16];

	y = win_height - y;
	nx = (float)(x - viewport[0]) / (float)viewport[2];
	ny = (float)(y - viewport[1]) / (float)viewport[3];

	cgm_mcopy(inv_pv, proj_matrix_inv);
	cgm_mmul(inv_pv, view_matrix_inv);

	cgm_vcons(&npos, nx, ny, 0.0f);
	cgm_unproject(&ray->origin, &npos, inv_pv);
	npos.z = 1.0f;
	cgm_unproject(&farpt, &npos, inv_pv);

	ray->dir.x = farpt.x - ray->origin.x;
	ray->dir.y = farpt.y - ray->origin.y;
	ray->dir.z = farpt.z - ray->origin.z;
}

static void moveobj(struct object *obj, int px0, int py0, int px1, int py1)
{
	cgm_ray ray;
	float dist;
	cgm_vec3 p0, p1;

	primray(&ray, px0, py0);
	cgm_vnormalize(&ray.dir);
	dist = ray_object_dist(&ray, obj);
	cgm_raypos(&p0, &ray, dist);
	primray(&ray, px1, py1);
	cgm_vnormalize(&ray.dir);
	cgm_raypos(&p1, &ray, dist);

	cgm_vsub(&p1, &p0);
	cgm_vadd(&obj->pos, &p1);
	obj->xform_valid = 0;

	inval_vport();
}

void inval_vport(void)
{
	vpdirty = 1;
	app_redisplay(0, 0, 0, 0);
}
