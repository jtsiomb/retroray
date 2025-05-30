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
#ifndef SCENE_H_
#define SCENE_H_

#include "cgmath/cgmath.h"
#include "material.h"

enum {
	OBJ_NULL,
	OBJ_SPHERE,
	OBJ_BOX,
	OBJ_CSG,

	OBJ_LIGHT,

	NUM_OBJ_TYPES
};

#define OBJ_COMMON_ATTR \
	int type; \
	char *name; \
	cgm_vec3 pos, scale, pivot; \
	cgm_quat rot; \
	float xform[16], dir_xform[16], inv_xform[16]; \
	int xform_valid; \
	struct material *mtl

struct object {
	OBJ_COMMON_ATTR;
};

struct csgnode {
	OBJ_COMMON_ATTR;
	int op;
	struct object *subobj;	/* darr */
};

struct light {
	OBJ_COMMON_ATTR;
	cgm_vec3 color, orig_color;
	float energy;
	int shadows;
};

struct scene {
	struct object **objects;	/* darr */
	struct light **lights;
	struct material **mtl;		/* darr */
};

struct rayhit;	/* declared in rt.h */

struct scene *create_scene(void);
void free_scene(struct scene *scn);

void scn_clear(struct scene *scn);

int scn_load(struct scene *scn, const char *fname);
int scn_save(struct scene *scn, const char *fname);

int scn_add_object(struct scene *scn, struct object *obj);
int scn_rm_object(struct scene *scn, int idx);
int scn_num_objects(const struct scene *scn);
int scn_object_index(const struct scene *scn, const struct object *obj);

int scn_add_material(struct scene *scn, struct material *mtl);
int scn_rm_material(struct scene *scn, struct material *mtl);
int scn_num_materials(const struct scene *scn);
int scn_material_index(const struct scene *scn, const struct material *mtl);
struct material *scn_find_material(const struct scene *scn, const char *mname);

int scn_add_light(struct scene *scn, struct light *mtl);
int scn_rm_light(struct scene *scn, struct light *mtl);
int scn_num_lights(const struct scene *scn);
int scn_light_index(const struct scene *scn, const struct light *mtl);
struct light *scn_find_light(const struct scene *scn, const char *mname);

int scn_intersect(const struct scene *scn, const cgm_ray *ray, struct rayhit *hit);
int scn_pick(const struct scene *scn, const cgm_ray *ray, struct rayhit *hit);

/* --- objects --- */
struct object *create_object(int type);
void free_object(struct object *obj);

int set_object_name(struct object *obj, const char *name);

void calc_object_matrix(struct object *obj);

/* --- lights --- */
struct light *create_light(void);
void free_light(struct light *lt);

int set_light_name(struct light *lt, const char *name);
void set_light_color(struct light *lt, float r, float g, float b);
void set_light_energy(struct light *lt, float e);

#endif	/* SCENE_H_ */
