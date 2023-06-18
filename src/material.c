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
#include <string.h>
#include "material.h"

void mtl_init(struct material *mtl)
{
	static int mtlidx;
	char namebuf[64];

	cgm_vcons(&mtl->kd, 0.7, 0.7, 0.7);
	cgm_vcons(&mtl->ks, 0.5, 0.5, 0.5);
	cgm_vcons(&mtl->ke, 0, 0, 0);
	mtl->shin = 50.0f;
	mtl->refl = mtl->trans = 0.0f;
	mtl->ior = 1.3333333;
	mtl->texmap = 0;

	sprintf(namebuf, "material%03d", mtlidx++);
	mtl_set_name(mtl, namebuf);
}

void mtl_destroy(struct material *mtl)
{
	if(!mtl) return;
	free(mtl->name);
}

void mtl_set_name(struct material *mtl, const char *name)
{
	char *tmp = strdup(name);
	if(!tmp) return;
	free(mtl->name);
	mtl->name = tmp;
}
