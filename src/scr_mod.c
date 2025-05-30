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
#include "options.h"

static int vpdirty, vpnav, projdirty;
static rtk_rect totalrend;
static float scr_aspect;


static int mdl_init(void);
static void mdl_destroy(void);
static int mdl_start(void);
static void mdl_stop(void);
static void mdl_display(void);
static void mdl_reshape(int x, int y);
static void mdl_keyb(int key, int press);
static void mdl_mouse(int bn, int press, int x, int y);
static void mdl_motion(int x, int y);

static void update_projmat(void);

static void draw_object(struct object *obj);
static void setup_material(struct material *mtl);
static void draw_grid(void);

static void act_settool(int tidx);
static void act_rmobj(void);

static void moveobj(struct object *obj, int px0, int py0, int px1, int py1);
static void rotobj(struct object *obj, int px0, int py0, int px1, int py1);
static void scaleobj(struct object *obj, int px0, int py0, int px1, int py1);

static void act_render(void);
static void act_viewer(void);
static void save_render(void);

void inval_vport(void);

void cancel_op(void);

struct app_screen scr_model = {
	"modeller",
	mdl_init, mdl_destroy,
	mdl_start, mdl_stop,
	mdl_display, mdl_reshape,
	mdl_keyb, mdl_mouse, mdl_motion
};

struct view view = {0, 20, 8};	/* theta, phi, dist, pos */

static struct cmesh *mesh_sph, *mesh_box;

static float view_matrix[16], proj_matrix[16];
static float view_matrix_inv[16], proj_matrix_inv[16];
static int viewport[4];
static cgm_ray pickray;

static int cur_tool;
/*static int prev_tool = -1;*/

static rtk_rect rband;
static int rband_valid;

static int rendering;
static rtk_rect rendrect;

static struct rayhit dbg_hit;


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

	if(!(mesh_box = cmesh_alloc())) {
		errormsg("failed to allocate box vis mesh\n");
		return -1;
	}
	gen_box(mesh_box, 1, 1, 1, 0, 0);

	selobj = -1;
	vpdirty = 1;
	axismask = 0xff;
	return 0;
}

static void mdl_destroy(void)
{
	cmesh_free(mesh_sph);
	cmesh_free(mesh_box);
	modui_cleanup();
}

static int mdl_start(void)
{
	gaw_clear_color(0.125, 0.125, 0.125, 1);

	gaw_enable(GAW_DEPTH_TEST);
	gaw_enable(GAW_CULL_FACE);
	gaw_enable(GAW_LIGHTING);
	gaw_enable(GAW_LIGHT0);
	gaw_enable(GAW_SPECULAR);
	gaw_enable(GAW_NORMALIZE);

	//rend_pan(0, -TOOLBAR_HEIGHT);
	return 0;
}

static void mdl_stop(void)
{
}

static void mdl_display(void)
{
	int i, num;

	/* viewport */
#ifdef GFX_GL
	if(1) {
#else
	if(vpdirty || vpnav) {
#endif
		if(projdirty) {
			update_projmat();
			projdirty = 0;
		}
		gaw_clear(GAW_COLORBUF | GAW_DEPTHBUF);

		gaw_matrix_mode(GAW_MODELVIEW);
		gaw_load_identity();
		gaw_translate(0, 0, -view.dist);
		gaw_rotate(view.phi, 1, 0, 0);
		gaw_rotate(view.theta, 0, 1, 0);
		gaw_translate(-view.pos.x, -view.pos.y, -view.pos.z);
		gaw_get_modelview(view_matrix);
		cgm_mcopy(view_matrix_inv, view_matrix);
		cgm_minverse(view_matrix_inv);
		draw_grid();

		num = scn_num_objects(scn);
		for(i=0; i<num; i++) {
			struct object *obj = scn->objects[i];

			if(obj->type == OBJ_LIGHT) {
				gaw_save();
				gaw_disable(GAW_LIGHTING);
				if(i != selobj) {
					gaw_poly_wire();
					gaw_color3f(0.6, 0.6, 0.3);
				} else {
					gaw_color3f(1, 1, 0);
				}
				draw_object(obj);
				gaw_poly_gouraud();
				gaw_restore();
			} else {
				setup_material(scn->objects[i]->mtl);

				if(i == selobj) {
					gaw_zoffset(0.1);
					gaw_enable(GAW_POLYGON_OFFSET);
					draw_object(obj);
					gaw_disable(GAW_POLYGON_OFFSET);

					gaw_save();
					gaw_disable(GAW_LIGHTING);
					gaw_poly_wire();
					gaw_color3f(0, 1, 0);
					draw_object(obj);
					gaw_poly_gouraud();
					gaw_restore();
				} else {
					draw_object(obj);
				}
			}
		}

		if(dbg_hit.obj) {
			gaw_save();
			gaw_disable(GAW_LIGHTING);
			gaw_begin(GAW_LINES);
			gaw_color3f(0, 1, 0);
			gaw_vertex3f(dbg_hit.pos.x, dbg_hit.pos.y, dbg_hit.pos.z);
			gaw_vertex3f(dbg_hit.pos.x + dbg_hit.norm.x, dbg_hit.pos.y + dbg_hit.norm.y,
					dbg_hit.pos.z + dbg_hit.norm.z);
			gaw_end();
			gaw_restore();
		}
		vpdirty = 0;

		/* dirty all GUI windows */
		rtk_invalidate_screen(modui);
	}

	/* render layer */
	if(rendering) {
		if(!render(framebuf)) {
			rendering = 0;
		}
		app_redisplay(rendrect.x, rendrect.y, rendrect.width, rendrect.height);
	}

	/* GUI */
	rtk_draw_begin();
	rtk_draw_screen(modui);
#ifdef GFX_GL
	if(totalrend.width) {
		static rtk_icon rfb;
		rfb.pixels = framebuf + totalrend.y * win_width + totalrend.x;
		rfb.width = totalrend.width;
		rfb.height = totalrend.height;
		rfb.scanlen = win_width;

		gaw_matrix_mode(GAW_PROJECTION);
		gaw_load_identity();
		gaw_ortho(0, win_width, win_height, 0, -1, 1);
		gaw_viewport(0, 0, win_width * opt.scale, win_height * opt.scale);

		gaw_enable(GAW_ALPHA_TEST);
		gaw_alpha_func(GAW_GREATER, 0.5f);
		gui_blit(totalrend.x, totalrend.y, &rfb);
		gaw_disable(GAW_ALPHA_TEST);
	}
#endif
	rtk_draw_end();
}

static void draw_object(struct object *obj)
{
	if(!obj->xform_valid) {
		calc_object_matrix(obj);
	}
	gaw_push_matrix();
	gaw_mult_matrix(obj->xform);

	switch(obj->type) {
	case OBJ_SPHERE:
	case OBJ_LIGHT:
		cmesh_draw(mesh_sph);
		break;

	case OBJ_BOX:
		/* TODO move to a better mesh, cmesh sucks balls */
		gaw_begin(GAW_QUADS);
		gaw_normal(0, 0, 1);
		gaw_vertex3f(-0.5, -0.5, 0.5);
		gaw_vertex3f(0.5, -0.5, 0.5);
		gaw_vertex3f(0.5, 0.5, 0.5);
		gaw_vertex3f(-0.5, 0.5, 0.5);
		gaw_normal(1, 0, 0);
		gaw_vertex3f(0.5, -0.5, 0.5);
		gaw_vertex3f(0.5, -0.5, -0.5);
		gaw_vertex3f(0.5, 0.5, -0.5);
		gaw_vertex3f(0.5, 0.5, 0.5);
		gaw_normal(0, 0, -1);
		gaw_vertex3f(0.5, -0.5, -0.5);
		gaw_vertex3f(-0.5, -0.5, -0.5);
		gaw_vertex3f(-0.5, 0.5, -0.5);
		gaw_vertex3f(0.5, 0.5, -0.5);
		gaw_normal(-1, 0, 0);
		gaw_vertex3f(-0.5, -0.5, -0.5);
		gaw_vertex3f(-0.5, -0.5, 0.5);
		gaw_vertex3f(-0.5, 0.5, 0.5);
		gaw_vertex3f(-0.5, 0.5, -0.5);
		gaw_normal(0, 1, 0);
		gaw_vertex3f(-0.5, 0.5, 0.5);
		gaw_vertex3f(0.5, 0.5, 0.5);
		gaw_vertex3f(0.5, 0.5, -0.5);
		gaw_vertex3f(-0.5, 0.5, -0.5);
		gaw_normal(0, -1, 0);
		gaw_vertex3f(-0.5, -0.5, -0.5);
		gaw_vertex3f(0.5, -0.5, -0.5);
		gaw_vertex3f(0.5, -0.5, 0.5);
		gaw_vertex3f(-0.5, -0.5, 0.5);
		gaw_end();
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
	int i;

	gaw_save();
	gaw_disable(GAW_LIGHTING);
	gaw_disable(GAW_DEPTH_TEST);

	gaw_begin(GAW_LINES);
	gaw_color3f(0.3, 0.3, 0.3);
	for(i=0; i<10; i++) {
		float offs = (float)((i + 1) * 2);
		gaw_vertex3f(offs, 0, 20);
		gaw_vertex3f(offs, 0, -20);
		gaw_vertex3f(-offs, 0, 20);
		gaw_vertex3f(-offs, 0, -20);

		gaw_vertex3f(20, 0, offs);
		gaw_vertex3f(-20, 0, offs);
		gaw_vertex3f(20, 0, -offs);
		gaw_vertex3f(-20, 0, -offs);
	}

	gaw_color3f(0.5, 0, 0);
	gaw_vertex3f(0, 0, 0);
	gaw_vertex3f(-20, 0, 0);
	gaw_vertex3f(0, 0, 0);
	gaw_vertex3f(20, 0, 0);
	gaw_color3f(0, 0, 0.8);
	gaw_vertex3f(0, 0, 0);
	gaw_vertex3f(0, 0, -20);
	gaw_vertex3f(0, 0, 0);
	gaw_vertex3f(0, 0, 20);
	gaw_end();

	gaw_restore();
}

static void mdl_reshape(int x, int y)
{
	scr_aspect = (float)x / (float)(y - TOOLBAR_HEIGHT);

	viewport[0] = 0;
	viewport[1] = TOOLBAR_HEIGHT;
	viewport[2] = x;
#ifdef GFX_GL
	/* XXX temp render-rect viewport match hack, until I fix it properly */
	viewport[3] = y;
#else
	viewport[3] = y - TOOLBAR_HEIGHT;
#endif
	gaw_viewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	update_projmat();

	rtk_resize(toolbar, win_width, TOOLBAR_HEIGHT);

	inval_vport();
}

static void mdl_keyb(int key, int press)
{
	if(rtk_input_key(modui, key, press)) {
		return;
	}

	switch(key) {
	case 'x':
		if(modkeys & KEY_MOD_SHIFT) {
			set_axismask(press ? ~1 : 0xff);
		} else {
			set_axismask(press ? 1 : 0xff);
		}
		return;
	case 'y':
		if(modkeys & KEY_MOD_SHIFT) {
			set_axismask(press ? ~2 : 0xff);
		} else {
			set_axismask(press ? 2 : 0xff);
		}
		return;
	case 'z':
		if(modkeys & KEY_MOD_SHIFT) {
			set_axismask(press ? ~4 : 0xff);
		} else {
			set_axismask(press ? 4 : 0xff);
		}
		return;
	default:
		break;
	}

	if(press) {
		switch(key) {
		case 27:
			cancel_op();
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

		case KEY_F5:
			act_render();
			break;

		case KEY_F6:
			act_settool(TOOL_REND_AREA);
			break;

		case KEY_F7:
			act_viewer();
			break;

		case KEY_DEL:
			act_rmobj();
			break;

		case KEY_F1:
			act_settool(TOOL_DBG);
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
		if(modkeys & (KEY_MOD_ALT | KEY_MOD_CTRL)) {
			vpnav |= (1 << bn);
		}

		if(bn == 3) {
			view.dist *= 0.75;
			if(view.dist < 0.1) view.dist = 0.1;
			inval_vport();
		} else if(bn == 4) {
			view.dist *= 1.3333333;
			inval_vport();
		}
	} else {
		vpnav &= ~(1 << bn);
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
						memset(framebuf, 0, win_width * win_height * sizeof *framebuf);
					}
				}
			}

		} else if(bn == 0 && x == rband.x && y == rband.y) {
			primray(&pickray, x, y);
			if(scn_pick(scn, &pickray, &hit)) {
				if(cur_tool == TOOL_DBG) {
					dbg_hit = hit;
					cgm_vnormalize(&dbg_hit.norm);
					inval_vport();
				} else {
					int newsel = scn_object_index(scn, hit.obj);
					if(newsel != selobj) {
						selobj = newsel;
						inval_vport();
					}
				}
			} else {
				if(cur_tool == TOOL_DBG) {
					dbg_hit.obj = 0;
					inval_vport();
				} else {
					if(selobj != -1) {
						inval_vport();
					}
					selobj = -1;
				}
			}
		}
	}
}

static void mdl_motion(int x, int y)
{
	int dx, dy;
	cgm_vec3 up, right;
	float viewrot[16], pan_speed;

	if(!vpdrag && rtk_input_mmotion(modui, x, y)) {
		return;
	}

	dx = x - mouse_x;
	dy = y - mouse_y;

	if(vpnav) {
		/* navigation */
		if(mouse_state[0]) {
			view.theta += dx * 0.5f;
			view.phi += dy * 0.5f;
			if(view.phi < -90) view.phi = -90;
			if(view.phi > 90) view.phi = 90;
			inval_vport();
		}

		if(mouse_state[2]) {
			view.dist += dy * 0.1f;
			if(view.dist < 0.1) view.dist = 0.1;
			inval_vport();
			projdirty = 1;
		}

		if(mouse_state[1]) {
			cgm_mrotation_axis(viewrot, 0, -cgm_deg_to_rad(view.phi));
			cgm_mrotate_axis(viewrot, 1, -cgm_deg_to_rad(view.theta));

			cgm_vcons(&right, 1, 0, 0);
			cgm_vcons(&up, 0, 1, 0);

			cgm_vmul_m3v3(&right, viewrot);
			cgm_vmul_m3v3(&up, viewrot);

			pan_speed = view.dist * 0.002 + 0.0025;
			cgm_vadd_scaled(&view.pos, &up, dy * pan_speed);
			cgm_vadd_scaled(&view.pos, &right, dx * -pan_speed);
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

			case TOOL_ROT:
				if(selobj >= 0) {
					struct object *obj = scn->objects[selobj];
					rotobj(obj, mouse_x, mouse_y, x, y);
				}
				break;

			case TOOL_SCALE:
				if(selobj >= 0) {
					struct object *obj = scn->objects[selobj];
					scaleobj(obj, mouse_x, mouse_y, x, y);
				}
				break;

			default:
				break;
			}
		}
	}
}

void tbn_callback(rtk_widget *w, void *cls)
{
	int id = (intptr_t)cls;

	switch(id) {
	case TBN_NEW:
	case TBN_OPEN:
		cancel_op();
		act_settool(TOOL_SEL);
		scn_clear(scn);
		selobj = -1;
		select_material(-1);
		if(id == TBN_OPEN) {
			scn_load(scn, scn_fname ? scn_fname : "foo.rry");
		}
		rtk_invalidate_screen(modui);
		inval_vport();
		break;

	case TBN_SAVE:
		scn_save(scn, scn_fname ? scn_fname : "foo.rry");
		break;

	case TBN_SEL:
	case TBN_MOVE:
	case TBN_ROT:
	case TBN_SCALE:
		act_settool(id - TBN_SEL);
		break;

	case TBN_XYZ:
		rtk_show_modal(xyzmenu);
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

	case TBN_REND:
		act_render();
		break;

	case TBN_REND_AREA:
		act_settool(TOOL_REND_AREA);
		break;

	case TBN_VIEWREND:
		act_viewer();
		break;

	case TBN_ADD:
		rtk_show_modal(objmenu);
		break;

	case TBN_RM:
		act_rmobj();
		break;

	default:
		break;
	}
}


static void update_projmat(void)
{
	float znear = 0.5f;
	if(view.dist < 2.0f) {
		znear = 0.1f;
	}
	gaw_matrix_mode(GAW_PROJECTION);
	gaw_load_identity();
	gaw_perspective(50, scr_aspect, znear, view.dist * 32.0f);
	gaw_get_projection(proj_matrix);
	cgm_mcopy(proj_matrix_inv, proj_matrix);
	cgm_minverse(proj_matrix_inv);
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

	/*prev_tool = cur_tool;*/
	cur_tool = tidx;
	for(i=0; i<NUM_TOOLS; i++) {
		if(!tools[i]) continue;
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

	y = win_height - (y - TOOLBAR_HEIGHT);
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

	/* project px0,py0 to the object plane */
	primray(&ray, px0, py0);
	cgm_vnormalize(&ray.dir);
	dist = ray_object_dist(&ray, obj);
	cgm_raypos(&p0, &ray, dist);

	/* project px1,py1 to the object plane */
	primray(&ray, px1, py1);
	cgm_vnormalize(&ray.dir);
	cgm_raypos(&p1, &ray, dist);

	/* find the vector from p0 to p1 on the plane and translate */
	cgm_vsub(&p1, &p0);

	if(axismask & 1) obj->pos.x += p1.x;
	if(axismask & 2) obj->pos.y += p1.y;
	if(axismask & 4) obj->pos.z += p1.z;

	obj->xform_valid = 0;

	inval_vport();
}

static void rotobj(struct object *obj, int px0, int py0, int px1, int py1)
{
	cgm_ray ray;
	float dist, mag;
	cgm_vec3 p0, p1, axis, up;
	cgm_quat qrot;

	/* project px0,py0 to the object plane */
	primray(&ray, px0, py0);
	cgm_vnormalize(&ray.dir);
	dist = ray_object_dist(&ray, obj);
	cgm_raypos(&p0, &ray, dist);

	/* project px1,py1 to the object plane */
	primray(&ray, px1, py1);
	cgm_vnormalize(&ray.dir);
	cgm_raypos(&p1, &ray, dist);

	/* axis of rotation runs from the center of the object to the view point */
	axis = ray.origin;
	cgm_vsub(&axis, &obj->pos);
	cgm_vnormalize(&axis);

	/* vectors from object center to each projected point */
	cgm_vsub(&p0, &obj->pos);
	cgm_vsub(&p1, &obj->pos);
	cgm_vcross(&up, &p0, &p1);
	mag = cgm_vlength(&up);

	if(cgm_vdot(&up, &axis) < 0) {
		mag = -mag;
	}

	if(axismask == 1 || axismask == 6) {
		cgm_qrotation(&qrot, mag, 1, 0, 0);
	} else if(axismask == 2 || axismask == 5) {
		cgm_qrotation(&qrot, mag, 0, 1, 0);
	} else if(axismask == 4 || axismask == 3) {
		cgm_qrotation(&qrot, mag, 0, 0, 1);
	} else {
		cgm_qrotation(&qrot, mag, axis.x, axis.y, axis.z);
	}

	cgm_qmul(&qrot, &obj->rot);
	obj->rot = qrot;

	obj->xform_valid = 0;

	inval_vport();
}

static void scaleobj(struct object *obj, int px0, int py0, int px1, int py1)
{
	cgm_ray ray;
	float dist, len;
	cgm_vec3 p0, p1;

	/* project px0,py0 to the object plane */
	primray(&ray, px0, py0);
	cgm_vnormalize(&ray.dir);
	dist = ray_object_dist(&ray, obj);
	cgm_raypos(&p0, &ray, dist);

	/* project px1,py1 to the object plane */
	primray(&ray, px1, py1);
	cgm_vnormalize(&ray.dir);
	cgm_raypos(&p1, &ray, dist);

	/* compute the scale factor as a function of the signed length of the
	 * vector from p0 to p1
	 */
	cgm_vsub(&p1, &p0);
	len = cgm_vlength(&p1);

	cgm_vsub(&p0, &obj->pos);
	if(cgm_vdot(&p0, &p1) < 0) {
		len = -len;
	}

	if(axismask & 1) obj->scale.x += len;
	if(axismask & 2) obj->scale.y += len;
	if(axismask & 4) obj->scale.z += len;
	obj->xform_valid = 0;

	inval_vport();
}

static void act_render(void)
{
	rendering = 1;
	rend_size(win_width, win_height);
	rendrect.x = rendrect.y = 0;
	rendrect.width = win_width;
	rendrect.height = win_height;
	rend_begin(rendrect.x, rendrect.y, rendrect.width, rendrect.height);
	app_redisplay(rendrect.x, rendrect.y, rendrect.width, rendrect.height);
	totalrend = rendrect;
	memset(framebuf, 0, win_width * win_height * sizeof *framebuf);
}

static void act_viewer(void)
{
	totalrend.x = totalrend.y = 0;
	totalrend.width = win_width;
	totalrend.height = win_height;
	save_render();
}

#define RENDFILE	"render.png"
static void save_render(void)
{
	if(img_save_pixels(RENDFILE, framebuf, win_width, win_height, IMG_FMT_RGBA32) == -1) {
		fprintf(stderr, "failed to save \"%s\"\n", RENDFILE);
	} else {
		printf("saved render: %s\n", RENDFILE);
	}
}

void inval_vport(void)
{
	vpdirty = 1;
	app_redisplay(0, 0, 0, 0);

	totalrend.width = totalrend.height = 0;
}

void cancel_op(void)
{
	if(rendering) {
		rendering = 0;
	}
	rtk_hide(mtlwin);
	inval_vport();
}

cgm_vec3 get_view_pos(void)
{
	cgm_vec3 p = {0};
	cgm_vmul_m4v3(&p, view_matrix_inv);
	return p;
}
