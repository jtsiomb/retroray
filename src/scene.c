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
#include <stdlib.h>
#include <float.h>
#include "scene.h"
#include "geom.h"
#include "darray.h"
#include "logger.h"
#include "treestor.h"

static struct material *default_material(void);

struct scene *create_scene(void)
{
	struct scene *scn;

	if(!(scn = malloc(sizeof *scn))) {
		errormsg("failed to allocate scene\n");
		return 0;
	}
	scn->objects = darr_alloc(0, sizeof *scn->objects);
	scn->lights = darr_alloc(0, sizeof *scn->lights);
	scn->mtl = darr_alloc(0, sizeof *scn->mtl);
	return scn;
}

void free_scene(struct scene *scn)
{
	if(!scn) return;

	scn_clear(scn);

	darr_free(scn->objects);
	darr_free(scn->lights);
	darr_free(scn->mtl);
	free(scn);
}

void scn_clear(struct scene *scn)
{
	int i;

	for(i=0; i<darr_size(scn->objects); i++) {
		free_object(scn->objects[i]);
	}
	darr_clear(scn->objects);

	for(i=0; i<darr_size(scn->lights); i++) {
		free_light(scn->lights[i]);
	}
	darr_clear(scn->lights);

	for(i=0; i<darr_size(scn->mtl); i++) {
		mtl_destroy(scn->mtl[i]);
		free(scn->mtl[i]);
	}
	darr_clear(scn->mtl);
}

static struct material *read_material(struct ts_node *tsmtl)
{
	struct material *mtl;

	if(!(mtl = malloc(sizeof *mtl))) {
		errormsg("failed to allocate new material!\n");
		return 0;
	}
	mtl_init(mtl);

	return mtl;
}

static struct object *read_object(struct scene *scn, struct ts_node *tsobj)
{
	return 0;
}

static struct light *read_light(struct ts_node *tslt)
{
	return 0;
}

int scn_load(struct scene *scn, const char *fname)
{
	int res = -1;
	struct ts_node *ts, *tsn;
	struct material *mtl;
	struct object *obj;
	struct light *lt;

	if(!(ts = ts_load(fname)) || strcmp(ts->name, "rrscene") != 0) {
		errormsg("failed to load: %s\n", fname);
		return -1;
	}

	scn_clear(scn);

	tsn = ts->child_list;
	while(tsn) {
		if(strcmp(tsn->name, "material") == 0) {
			if(!(mtl = read_material(tsn))) {
				goto end;
			}
			scn_add_material(scn, mtl);

		} else if(strcmp(tsn->name, "object") == 0) {
			if(!(obj = read_object(scn, tsn))) {
				goto end;
			}
			scn_add_object(scn, obj);

		} else if(strcmp(tsn->name, "light") == 0) {
			if(!(lt = read_light(tsn))) {
				goto end;
			}
			scn_add_light(scn, lt);
		}
		tsn = tsn->next;
	}

	res = 0;
end:
	ts_free_tree(ts);
	return res;
}

int scn_save(struct scene *scn, const char *fname)
{
	return -1;
}

int scn_add_object(struct scene *scn, struct object *obj)
{
	darr_push(scn->objects, &obj);
	return 0;
}

int scn_rm_object(struct scene *scn, int idx)
{
	int numobj = darr_size(scn->objects);

	if(idx < 0 || idx >= numobj) {
		return -1;
	}

	free_object(scn->objects[idx]);

	if(idx < numobj - 1) {
		scn->objects[idx] = scn->objects[numobj - 1];
	}
	darr_pop(scn->objects);
	return 0;
}

int scn_num_objects(const struct scene *scn)
{
	return darr_size(scn->objects);
}

int scn_object_index(const struct scene *scn, const struct object *obj)
{
	int i, num = darr_size(scn->objects);
	for(i=0; i<num; i++) {
		if(scn->objects[i] == obj) {
			return i;
		}
	}
	return -1;
}



int scn_add_material(struct scene *scn, struct material *mtl)
{
	darr_push(scn->mtl, &mtl);
	return 0;
}

int scn_rm_material(struct scene *scn, struct material *mtl)
{
	int idx, num_mtl;

	if((idx = scn_material_index(scn, mtl)) == -1) {
		return -1;
	}

	num_mtl = darr_size(scn->mtl);

	if(idx < num_mtl - 1) {
		scn->mtl[idx] = scn->mtl[num_mtl - 1];
	}
	darr_pop(scn->mtl);
	return 0;
}

int scn_num_materials(const struct scene *scn)
{
	return darr_size(scn->mtl);
}

int scn_material_index(const struct scene *scn, const struct material *mtl)
{
	int i, num_mtl;

	num_mtl = darr_size(scn->mtl);
	for(i=0; i<num_mtl; i++) {
		if(scn->mtl[i] == mtl) {
			return i;
		}
	}
	return -1;
}

struct material *scn_find_material(const struct scene *scn, const char *mname)
{
	int i, num_mtl;

	num_mtl = darr_size(scn->mtl);
	for(i=0; i<num_mtl; i++) {
		if(strcmp(scn->mtl[i]->name, mname) == 0) {
			return scn->mtl[i];
		}
	}
	return 0;
}

/* manage lights */

int scn_add_light(struct scene *scn, struct light *light)
{
	darr_push(scn->lights, &light);
	return 0;
}

int scn_rm_light(struct scene *scn, struct light *light)
{
	int idx, num_lights;

	if((idx = scn_light_index(scn, light)) == -1) {
		return -1;
	}

	num_lights = darr_size(scn->lights);

	if(idx < num_lights - 1) {
		scn->lights[idx] = scn->lights[num_lights - 1];
	}
	darr_pop(scn->lights);
	return 0;
}

int scn_num_lights(const struct scene *scn)
{
	return darr_size(scn->lights);
}

int scn_light_index(const struct scene *scn, const struct light *light)
{
	int i, num_lights;

	num_lights = darr_size(scn->lights);
	for(i=0; i<num_lights; i++) {
		if(scn->lights[i] == light) {
			return i;
		}
	}
	return -1;
}

struct light *scn_find_light(const struct scene *scn, const char *mname)
{
	int i, num_lights;

	num_lights = darr_size(scn->lights);
	for(i=0; i<num_lights; i++) {
		if(strcmp(scn->lights[i]->name, mname) == 0) {
			return scn->lights[i];
		}
	}
	return 0;
}


int scn_intersect(const struct scene *scn, const cgm_ray *ray, struct rayhit *hit)
{
	int i, numobj;
	struct rayhit hit0, tmphit;

	hit0.t = FLT_MAX;
	hit0.obj = 0;

	numobj = darr_size(scn->objects);
	for(i=0; i<numobj; i++) {
		if(ray_object(ray, scn->objects[i], &tmphit) && tmphit.t < hit0.t) {
			hit0 = tmphit;
		}
	}

	if(hit0.obj) {
		if(hit) *hit = hit0;
		return 1;
	}
	return 0;
}


/* --- object functions --- */

struct object *create_object(int type)
{
	struct object *obj;
	char buf[32];
	static int objid;

	switch(type) {
	case OBJ_SPHERE:
		if(!(obj = calloc(1, sizeof *obj))) {
			goto err;
		}
		sprintf(buf, "sphere%03d", objid);
		break;

	case OBJ_BOX:
		if(!(obj = calloc(1, sizeof *obj))) {
			goto err;
		}
		sprintf(buf, "box%03d", objid);
		break;

	default:
		if(!(obj = calloc(1, sizeof *obj))) {
			goto err;
		}
		sprintf(buf, "object%03d", objid);
		break;
	}

	obj->type = type;

	cgm_vcons(&obj->pos, 0, 0, 0);
	cgm_qcons(&obj->rot, 0, 0, 0, 1);
	cgm_vcons(&obj->scale, 1, 1, 1);
	cgm_vcons(&obj->pivot, 0, 0, 0);
	cgm_midentity(obj->xform);
	cgm_midentity(obj->dir_xform);
	cgm_midentity(obj->inv_xform);
	obj->xform_valid = 1;
	obj->mtl = default_material();

	set_object_name(obj, buf);
	objid++;
	return obj;

err:
	errormsg("failed to allocate object\n");
	return 0;
}

void free_object(struct object *obj)
{
	if(!obj) return;

	free(obj->name);
	free(obj);
}

int set_object_name(struct object *obj, const char *name)
{
	char *str = strdup(name);
	if(!str) return -1;

	free(obj->name);
	obj->name = str;
	return 0;
}

void calc_object_matrix(struct object *obj)
{
	int i;
	float rmat[16];
	float *mat = obj->xform;

	cgm_mtranslation(mat, obj->pivot.x, obj->pivot.y, obj->pivot.z);
	cgm_mrotation_quat(rmat, &obj->rot);

	for(i=0; i<3; i++) {
		mat[i] = rmat[i];
		mat[4 + i] = rmat[4 + i];
		mat[8 + i] = rmat[8 + i];
	}

	mat[0] *= obj->scale.x; mat[4] *= obj->scale.y; mat[8] *= obj->scale.z; mat[12] += obj->pos.x;
	mat[1] *= obj->scale.x; mat[5] *= obj->scale.y; mat[9] *= obj->scale.z; mat[13] += obj->pos.y;
	mat[2] *= obj->scale.x; mat[6] *= obj->scale.y; mat[10] *= obj->scale.z; mat[14] += obj->pos.z;

	cgm_mpretranslate(mat, -obj->pivot.x, -obj->pivot.y, -obj->pivot.z);
	/* that's basically: pivot * rotation * translation * scaling * -pivot */

	/* dir_xform is the inverse/transpose upper 3x3 of the matrix */
	cgm_mcopy(obj->dir_xform, mat);
	/*obj->dir_xform[3] = obj->dir_xform[7] = obj->dir_xform[11] = 0.0f;
	obj->dir_xform[12] = obj->dir_xform[13] = obj->dir_xform[14] = 0.0f;
	obj->dir_xform[15] = 1.0f;*/
	cgm_minverse(obj->dir_xform);
	cgm_mtranspose(obj->dir_xform);

	cgm_mcopy(obj->inv_xform, mat);
	cgm_minverse(obj->inv_xform);

	obj->xform_valid = 1;
}

/* --- lights --- */
struct light *create_light(void)
{
	struct light *lt;
	static int ltidx;
	char buf[64];

	if(!(lt = calloc(1, sizeof *lt))) {
		return 0;
	}
	cgm_vcons(&lt->pos, 0, 0, 0);

	set_light_color(lt, 1, 1, 1);
	set_light_energy(lt, 1);

	sprintf(buf, "light%03d", ltidx++);
	set_light_name(lt, buf);

	lt->shadows = 1;
	return lt;
}

void free_light(struct light *lt)
{
	if(!lt) return;
	free(lt->name);
	free(lt);
}

int set_light_name(struct light *lt, const char *name)
{
	char *tmp = strdup(name);
	if(!tmp) return -1;
	free(lt->name);
	lt->name = tmp;
	return 0;
}

void set_light_color(struct light *lt, float r, float g, float b)
{
	cgm_vcons(&lt->orig_color, r, g, b);
	lt->color = lt->orig_color;
	cgm_vscale(&lt->color, lt->energy);
}

void set_light_energy(struct light *lt, float e)
{
	lt->energy = e;
	lt->color = lt->orig_color;
	cgm_vscale(&lt->color, e);
}

static struct material *default_material(void)
{
	static struct material defmtl;

	if(!defmtl.name) {
		mtl_init(&defmtl);
		mtl_set_name(&defmtl, "default_mtl");
	}
	return &defmtl;
}
