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

enum {
	OBJ_NULL,
	OBJ_SPHERE
};

#define OBJ_COMMON_ATTR \
	int type; \
	char *name; \
	cgm_vec3 pos, scale, pivot; \
	cgm_quat rot; \
	float xform[16]

struct object {
	OBJ_COMMON_ATTR;
};

struct sphere {
	OBJ_COMMON_ATTR;
	float rad;
};

struct scene {
	struct object **objects;	/* darr */
};

struct scene *create_scene(void);
void free_scene(struct scene *scn);

int scn_add_object(struct scene *scn, struct object *obj);
int scn_num_objects(struct scene *scn);

struct object *create_object(int type);
void free_object(struct object *obj);

int set_object_name(struct object *obj, const char *name);

void calc_object_matrix(struct object *obj);

#endif	/* SCENE_H_ */
