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

static int rx, ry, rwidth, rheight;
static int roffs;
static int xstep, ystep;

int rend_init(void)
{
	img_init(&renderbuf);

	rx = ry = rwidth = rheight = roffs = 0;

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
	if(fb) fb += roffs;

	if(xstep < 1) xstep = 1;
	if(ystep < 1) ystep = 1;

	for(i=0; i<rheight; i+=ystep) {
		h = ystep;
		if(i + h > rheight) h = rheight - i;

		for(j=0; j<rwidth; j+=xstep) {
			primray(&ray, rx + j, ry + i);
			ray_trace(&ray, max_ray_depth, &color);

			r = cround64(color.x * 255.0f);
			g = cround64(color.y * 255.0f);
			b = cround64(color.z * 255.0f);

			if(r > 255) r = 255;
			if(g > 255) g = 255;
			if(b > 255) b = 255;

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
	return cgm_vvec(1, 0, 0);
}
