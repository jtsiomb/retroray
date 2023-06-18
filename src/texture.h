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
#ifndef TEXTURE_H_
#define TEXTURE_H_

#include "cgmath/cgmath.h"
#include "imago2.h"

struct rayhit;

enum {
	TEX_PIXMAP,
	TEX_CHESS,
	TEX_FBM2D,
	TEX_FBM3D,
	TEX_MARBLE2D,
	TEX_MARBLE3D
};

#define TEX_COMMON_ATTR	\
	int type; \
	char *name; \
	cgm_vec3 offs, scale; \
	cgm_vec3 (*lookup)(const struct texture*, const struct rayhit*)

struct texture {
	TEX_COMMON_ATTR;
};

struct tex_pixmap {
	TEX_COMMON_ATTR;
	struct img_pixmap *img;
};

struct tex_chess {
	TEX_COMMON_ATTR;
	cgm_vec3 color[2];
};

struct tex_fbm {
	TEX_COMMON_ATTR;
	int octaves;
	cgm_vec3 color[2];
};

struct texture *create_texture(int type);
void free_texture(struct texture *tex);

void tex_set_name(struct texture *tex, const char *name);

#endif	/* TEXTURE_H_ */
