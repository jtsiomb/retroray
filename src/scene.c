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
#include "scene.h"
#include "darray.h"
#include "logger.h"

struct scene *create_scene(void)
{
	struct scene *scn;

	if(!(scn = malloc(sizeof *scn))) {
		errormsg("failed to allocate scene\n");
		return 0;
	}
	scn->objects = darr_alloc(0, sizeof *scn->objects);

	return scn;
}

void free_scene(struct scene *scn)
{
	int i;

	if(!scn) return;

	for(i=0; i<darr_size(scn->objects); i++) {
		free_object(scn->objects[i]);
	}
	darr_free(scn->objects);
	free(scn);
}

int scn_add_object(struct scene *scn, struct object *obj)
{
	darr_push(scn->objects, &obj);
	return 0;
}

int scn_num_objects(struct scene *scn)
{
	return darr_size(scn->objects);
}

struct object *create_object(int type)
{
	struct object *obj;
	struct sphere *sph;
	char buf[32];
	static int objid;

	if(!(obj = malloc(sizeof *obj))) {
		errormsg("failed to allocate object\n");
		return 0;
	}
	obj->type = type;

	cgm_vcons(&obj->pos, 0, 0, 0);
	cgm_qcons(&obj->rot, 0, 0, 0, 1);
	cgm_vcons(&obj->scale, 1, 1, 1);
	cgm_vcons(&obj->pivot, 0, 0, 0);
	cgm_midentity(obj->xform);

	switch(type) {
	case OBJ_SPHERE:
		sph = (struct sphere*)obj;
		sph->rad = 1.0f;
		sprintf(buf, "sphere%03d", objid);
		break;

	default:
		sprintf(buf, "object%03d", objid);
		break;
	}

	set_object_name(obj, buf);
	objid++;
	return obj;
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
}
