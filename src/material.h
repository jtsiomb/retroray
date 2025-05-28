/*
RetroRay - integrated standalone vintage modeller/renderer
Copyright (C) 2023-2025  John Tsiombikas <nuclear@mutantstargoat.com>

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
#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "cgmath/cgmath.h"
#include "texture.h"

struct material {
	char *name;
	cgm_vec3 kd, ks, ke;
	float shin;
	float refl, trans, ior;

	struct texture *texmap;
};

void mtl_init(struct material *mtl);
void mtl_destroy(struct material *mtl);
void mtl_clone(struct material *dest, const struct material *src);

void mtl_set_name(struct material *mtl, const char *name);

#endif	/* MATERIAL_H_ */
