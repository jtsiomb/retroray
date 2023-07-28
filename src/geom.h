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
#ifndef GEOM_H_
#define GEOM_H_

#include "cgmath/cgmath.h"
#include "scene.h"

struct rayhit {
	float t;
	cgm_vec3 pos;
	cgm_vec3 norm;
	cgm_vec2 uv;
	struct object *obj;
};

struct interval {
	struct rayhit a, b;
};

#define MAX_INTERV	32
struct csghit {
	struct interval ivlist[MAX_INTERV];
	int ivcount;
};

int ray_object(const cgm_ray *ray, const struct object *obj, struct rayhit *hit);
int ray_object_csg(const cgm_ray *ray, const struct object *obj, struct csghit *hit);

int ray_sphere(const cgm_ray *ray, const struct sphere *sph, struct csghit *hit);
int ray_box(const cgm_ray *ray, const struct object *box, struct csghit *hit);
int ray_csg(const cgm_ray *ray, const struct csgnode *csg, struct csghit *hit);

float ray_object_dist(const cgm_ray *ray, const struct object *obj);

#endif	/* GEOM_H_ */
