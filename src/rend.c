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
#include "rend.h"
#include "app.h"
#include "geom.h"
#include "util.h"
#include "gfxutil.h"
#include "scene.h"

struct img_pixmap renderbuf;

int max_ray_depth;
cgm_vec3 ambient;

static int rx, ry, rwidth, rheight;
static int roffs;
static int xstep, ystep;
static int pan_x, pan_y;

static struct light def_light = {0, {0, 0, 0}, {1, 1, 1}, {1, 1, 1}, 1, 1};


int rend_init(void)
{
	img_init(&renderbuf);

	cgm_vcons(&ambient, 0.05, 0.05, 0.05);

	rx = ry = rwidth = rheight = roffs = 0;
	pan_x = pan_y = 0;

	max_ray_depth = 6;
	return 0;
}

void rend_destroy(void)
{
	img_destroy(&renderbuf);
}

void rend_size(int xsz, int ysz)
{
	if(xsz != renderbuf.width || ysz != renderbuf.height) {
		img_set_pixels(&renderbuf, xsz, ysz, IMG_FMT_RGBA32, 0);
	}
}

void rend_pan(int xoffs, int yoffs)
{
	pan_x = xoffs;
	pan_y = yoffs;
}

void rend_begin(int x, int y, int w, int h)
{
	int i;
	uint32_t *ptr;

	if(w == 0 || h == 0) {
		rx = ry = 0;
		rwidth = renderbuf.width;
		rheight = renderbuf.height;
	} else {
		rx = x;
		ry = y;
		rwidth = w;
		rheight = h;
	}
	roffs = ry * renderbuf.width + rx;

	xstep = rwidth;
	ystep = rheight;

	ptr = (uint32_t*)renderbuf.pixels + roffs;
	for(i=0; i<rheight; i++) {
		memset(ptr, 0, rwidth * sizeof *ptr);
		ptr += renderbuf.width;
	}
}

static void fillrect(uint32_t *fb, int x, int y, int w, int h, uint32_t c)
{
	int i, j;

	fb += y * renderbuf.width + x;
	for(i=0; i<h; i++) {
		for(j=0; j<w; j++) {
			fb[j] = c;
		}
		fb += renderbuf.width;
	}
}

int render(uint32_t *fb)
{
	int i, j, w, h, offs, r, g, b;
	uint32_t *dest, pcol;
	cgm_vec3 color;
	cgm_ray ray;

	dest = (uint32_t*)renderbuf.pixels + roffs;
	if(fb) {
		fb += roffs;
	}

	if(xstep < 1) xstep = 1;
	if(ystep < 1) ystep = 1;

	if(scn_num_lights(scn) == 0) {
		primray(&ray, renderbuf.width / 2, renderbuf.height / 2);
		def_light.pos = ray.origin;
	}

	for(i=0; i<rheight; i+=ystep) {
		h = ystep;
		if(i + h > rheight) h = rheight - i;

		for(j=0; j<rwidth; j+=xstep) {
			primray(&ray, rx + j + pan_x, ry + i + pan_y);
			ray_trace(&ray, max_ray_depth, &color);

			if(color.x > 1.0f) color.x = 1.0f;
			if(color.y > 1.0f) color.y = 1.0f;
			if(color.z > 1.0f) color.z = 1.0f;
			r = cround64(color.x * 255.0f);
			g = cround64(color.y * 255.0f);
			b = cround64(color.z * 255.0f);
			pcol = PACK_RGB32(r, g, b);

			offs = i * renderbuf.width + j;
			dest[offs] = pcol;

			if(fb) {
				w = xstep;
				if(j + w > rwidth) w = rwidth - j;

				fillrect(fb, j, i, w, h, pcol);
			}
		}
	}

	xstep >>= 1;
	ystep >>= 1;

	if((xstep | ystep) >= 1) {
		return 1;
	}
	return 0;
}

int ray_trace(const cgm_ray *ray, int maxiter, cgm_vec3 *res)
{
	struct rayhit hit;

	if(!scn_intersect(scn, ray, &hit)) {
		*res = bgcolor(ray);
		return 0;
	}

	*res = shade(ray, &hit, maxiter);
	return 1;
}

cgm_vec3 bgcolor(const cgm_ray *ray)
{
	return cgm_vvec(0, 0, 0);
}

cgm_vec3 shade(const cgm_ray *ray, const struct rayhit *hit, int maxiter)
{
	int i, num_lights;
	cgm_vec3 color, dcol, scol, texel, vdir;
	struct material *mtl;
	struct light *lt;

	dcol = ambient;
	cgm_vcons(&scol, 0, 0, 0);

	mtl = hit->obj->mtl;

	vdir = ray->dir;
	cgm_vneg(&vdir);
	cgm_vnormalize(&vdir);

	if(!(num_lights = scn_num_lights(scn))) {
		calc_light(hit, &def_light, &vdir, &dcol, &scol);
	}
	for(i=0; i<num_lights; i++) {
		lt = scn->lights[i];
		calc_light(hit, lt, &vdir, &dcol, &scol);
	}

	if(mtl->texmap) {
		texel = mtl->texmap->lookup(mtl->texmap, hit);
		cgm_vmul(&dcol, &texel);
	}

	color = dcol;
	cgm_vadd(&color, &scol);
	return color;
}

int calc_light(const struct rayhit *hit, const struct light *lt,
		const cgm_vec3 *vdir, cgm_vec3 *dcol, cgm_vec3 *scol)
{
	float ndotl, ndoth, spec;
	cgm_vec3 norm, ldir, hdir;
	cgm_ray ray;
	struct material *mtl = hit->obj->mtl;

	ldir = lt->pos;
	cgm_vsub(&ldir, &hit->pos);

	ray.origin = hit->pos;
	ray.dir = ldir;

	if(lt->shadows && scn_intersect(scn, &ray, 0)) {
		return 0;	/* in shadow */
	}

	norm = hit->norm;
	cgm_vnormalize(&norm);

	cgm_vnormalize(&ldir);

	hdir = *vdir;
	cgm_vadd(&hdir, &ldir);
	cgm_vnormalize(&hdir);

	ndotl = cgm_vdot(&norm, &ldir);
	if(ndotl < 0.0f) ndotl = 0.0f;
	ndoth = cgm_vdot(&norm, &hdir);
	if(ndoth < 0.0f) ndoth = 0.0f;

	spec = pow(ndoth, mtl->shin);

	dcol->x += mtl->kd.x * ndotl * lt->color.x;
	dcol->y += mtl->kd.y * ndotl * lt->color.y;
	dcol->z += mtl->kd.z * ndotl * lt->color.z;

	scol->x += mtl->ks.x * spec * lt->color.x;
	scol->y += mtl->ks.y * spec * lt->color.y;
	scol->z += mtl->ks.z * spec * lt->color.z;

	return 1;
}
