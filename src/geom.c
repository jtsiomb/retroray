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
#include "geom.h"

int ray_object(const cgm_ray *ray, const struct object *obj, struct rayhit *hit)
{
	struct csghit csghit;

	if(!ray_object_csg(ray, obj, &csghit)) {
		return 0;
	}
	if(hit) {
		*hit = csghit.ivlist[0].a;
	}
	return 1;
}

int ray_object_csg(const cgm_ray *ray, const struct object *obj, struct csghit *hit)
{
	int i, res;
	cgm_ray localray = *ray;

	cgm_rmul_mr(&localray, obj->inv_xform);

	switch(obj->type) {
	case OBJ_SPHERE:
		res = ray_sphere(&localray, (const struct sphere*)obj, hit);
		break;

	default:
		res = 0;
	}

	if(res && hit) {
		for(i=0; i<hit->ivcount; i++) {
			cgm_vmul_m4v3(&hit->ivlist[i].a.pos, obj->xform);
			cgm_vmul_m3v3(&hit->ivlist[i].a.norm, obj->xform);
			cgm_vmul_m4v3(&hit->ivlist[i].b.pos, obj->xform);
			cgm_vmul_m3v3(&hit->ivlist[i].b.norm, obj->xform);
		}
	}
	return res;
}

#define EPSILON	1e-5

int ray_sphere(const cgm_ray *ray, const struct sphere *sph, struct csghit *hit)
{
	int i;
	float a, b, c, d, sqrt_d, t1, t2;/*, invrad;*/
	struct rayhit *rhptr;

	a = cgm_vdot(&ray->dir, &ray->dir);
	b = 2.0f * ray->dir.x * ray->origin.x +
		2.0f * ray->dir.y * ray->origin.y +
		2.0f * ray->dir.z * ray->origin.z;
	c = cgm_vdot(&ray->origin, &ray->origin) - sph->rad * sph->rad;

	if((d = b * b - 4.0 * a * c) < 0.0) return 0;

	sqrt_d = sqrt(d);
	t1 = (-b + sqrt_d) / (2.0 * a);
	t2 = (-b - sqrt_d) / (2.0 * a);

	if((t1 < EPSILON && t2 < EPSILON) || (t1 > 1.0f && t2 > 1.0f)) {
		return 0;
	}

	if(hit) {
		if(t1 < EPSILON || t1 > 1.0f) {
			t1 = t2;
		} else if(t2 < EPSILON || t2 > 1.0f) {
			t2 = t1;
		}
		if(t2 < t1) {
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}

		hit->ivcount = 1;
		hit->ivlist[0].a.t = t1;
		hit->ivlist[0].b.t = t2;
		/*invrad = 1.0f / sph->rad;*/

		rhptr = &hit->ivlist[0].a;
		for(i=0; i<2; i++) {
			cgm_raypos(&rhptr->pos, ray, rhptr->t);
			rhptr->norm = rhptr->pos;
			cgm_vnormalize(&rhptr->norm);
			/*rhptr->norm.x = rhptr->pos.x * invrad;
			rhptr->norm.y = rhptr->pos.y * invrad;
			rhptr->norm.z = rhptr->pos.z * invrad;*/
			rhptr->uv.x = (atan2(rhptr->norm.z, rhptr->norm.x) + CGM_PI) / (2.0 * CGM_PI);
			rhptr->uv.y = acos(rhptr->norm.y) / CGM_PI;
			rhptr->obj = (struct object*)sph;
			rhptr = &hit->ivlist[0].b;
		}
	}
	return 1;
}

int ray_csg(const cgm_ray *ray, const struct csgnode *csg, struct csghit *hit)
{
	return 0;
}

float ray_object_dist(const cgm_ray *ray, const struct object *obj)
{
	/*struct rayhit hit;*/
	cgm_vec3 norm, pvec;

	/*if(ray_object(ray, obj, &hit)) {
		return cgm_vdist(&hit.pos, &ray->origin);
	}*/

	/* if we can't hit the object for some reason, fallback to computing the
	 * distance of obj->pos from the plane perpendicular to the ray at the origin
	 */
	norm = ray->dir;
	/*cgm_vnormalize(&norm);*/
	pvec = obj->pos;
	cgm_vsub(&pvec, &ray->origin);
	return cgm_vdot(&pvec, &norm);
}
