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
#ifndef REND_H_
#define REND_H_

#include "cgmath/cgmath.h"
#include "geom.h"
#include "sizeint.h"
#include "imago2.h"

extern struct img_pixmap renderbuf;
extern int max_ray_depth;
extern cgm_vec3 ambient;

struct scene;

int rend_init(void);
void rend_size(int xsz, int ysz);
void rend_pan(int xoffs, int yoffs);
void rend_begin(int x, int y, int w, int h);
int render(uint32_t *fb);

int ray_trace(const cgm_ray *ray, int maxiter, cgm_vec3 *res);

cgm_vec3 bgcolor(const cgm_ray *ray);
cgm_vec3 shade(const cgm_ray *ray, const struct rayhit *hit, int maxiter);

int calc_light(const struct rayhit *hit, const struct light *lt,
		const cgm_vec3 *vdir, cgm_vec3 *dcol, cgm_vec3 *scol);



#endif	/* REND_H_ */
