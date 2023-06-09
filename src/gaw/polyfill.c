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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "polyfill.h"

/*#define DEBUG_OVERDRAW	PACK_RGB(10, 10, 10)*/

#define FILL_POLY_BITS	0x03


/* mode bits: 00-wire 01-flat 10-gouraud 11-reserved
 *     bit 2: texture
 *     bit 3-4: blend mode: 00-none 01-alpha 10-additive 11-reserved
 *     bit 5: zbuffering
 */
void (*fillfunc[])(struct pvertex*, int) = {
	polyfill_wire,
	polyfill_flat,
	polyfill_gouraud,
	0,
	polyfill_tex_wire,
	polyfill_tex_flat,
	polyfill_tex_gouraud,
	0,
	polyfill_alpha_wire,
	polyfill_alpha_flat,
	polyfill_alpha_gouraud,
	0,
	polyfill_alpha_tex_wire,
	polyfill_alpha_tex_flat,
	polyfill_alpha_tex_gouraud,
	0,
	polyfill_add_wire,
	polyfill_add_flat,
	polyfill_add_gouraud,
	0,
	polyfill_add_tex_wire,
	polyfill_add_tex_flat,
	polyfill_add_tex_gouraud,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	polyfill_wire_zbuf,
	polyfill_flat_zbuf,
	polyfill_gouraud_zbuf,
	0,
	polyfill_tex_wire_zbuf,
	polyfill_tex_flat_zbuf,
	polyfill_tex_gouraud_zbuf,
	0,
	polyfill_alpha_wire_zbuf,
	polyfill_alpha_flat_zbuf,
	polyfill_alpha_gouraud_zbuf,
	0,
	polyfill_alpha_tex_wire_zbuf,
	polyfill_alpha_tex_flat_zbuf,
	polyfill_alpha_tex_gouraud_zbuf,
	0,
	polyfill_add_wire_zbuf,
	polyfill_add_flat_zbuf,
	polyfill_add_gouraud_zbuf,
	0,
	polyfill_add_tex_wire_zbuf,
	polyfill_add_tex_flat_zbuf,
	polyfill_add_tex_gouraud_zbuf,
	0, 0, 0, 0, 0, 0, 0, 0, 0
};

struct pimage pfill_fb, pfill_tex;
uint32_t *pfill_zbuf;

#define EDGEPAD	8
static struct pvertex *edgebuf, *left, *right;
static int edgebuf_size;
static int fbheight;

/*
#define CHECKEDGE(x) \
	do { \
		assert(x >= 0); \
		assert(x < fbheight); \
	} while(0)
*/
#define CHECKEDGE(x)


void polyfill_fbheight(int height)
{
	int newsz = (height * 2 + EDGEPAD * 3) * sizeof *edgebuf;

	if(newsz > edgebuf_size) {
		free(edgebuf);
		if(!(edgebuf = malloc(newsz))) {
			fprintf(stderr, "failed to allocate edge table buffer (%d bytes)\n", newsz);
			abort();
		}
		edgebuf_size = newsz;

		left = edgebuf + EDGEPAD;
		right = edgebuf + height + EDGEPAD * 2;

#ifndef NDEBUG
		memset(edgebuf, 0xaa, EDGEPAD * sizeof *edgebuf);
		memset(edgebuf + height + EDGEPAD, 0xaa, EDGEPAD * sizeof *edgebuf);
		memset(edgebuf + height * 2 + EDGEPAD * 2, 0xaa, EDGEPAD * sizeof *edgebuf);
#endif
	}

	fbheight = height;
}

void polyfill(int mode, struct pvertex *verts, int nverts)
{
#ifndef NDEBUG
	if(!fillfunc[mode]) {
		fprintf(stderr, "polyfill mode %d not implemented\n", mode);
		abort();
	}
#endif

	fillfunc[mode](verts, nverts);
}

void polyfill_wire(struct pvertex *verts, int nverts)
{
	draw_line(verts);
}

void polyfill_tex_wire(struct pvertex *verts, int nverts)
{
	polyfill_wire(verts, nverts);	/* TODO */
}

void polyfill_alpha_wire(struct pvertex *verts, int nverts)
{
	polyfill_wire(verts, nverts);	/* TODO */
}

void polyfill_alpha_tex_wire(struct pvertex *verts, int nverts)
{
	polyfill_wire(verts, nverts);	/* TODO */
}

void polyfill_add_wire(struct pvertex *verts, int nverts)
{
	polyfill_wire(verts, nverts);	/* TODO */
}

void polyfill_add_tex_wire(struct pvertex *verts, int nverts)
{
	polyfill_wire(verts, nverts);	/* TODO */
}

void polyfill_wire_zbuf(struct pvertex *verts, int nverts)
{
	draw_line_zbuf(verts);
}

void polyfill_tex_wire_zbuf(struct pvertex *verts, int nverts)
{
	polyfill_wire_zbuf(verts, nverts);	/* TODO */
}

void polyfill_alpha_wire_zbuf(struct pvertex *verts, int nverts)
{
	polyfill_wire_zbuf(verts, nverts);	/* TODO */
}

void polyfill_alpha_tex_wire_zbuf(struct pvertex *verts, int nverts)
{
	polyfill_wire_zbuf(verts, nverts);	/* TODO */
}

void polyfill_add_wire_zbuf(struct pvertex *verts, int nverts)
{
	polyfill_wire_zbuf(verts, nverts);	/* TODO */
}

void polyfill_add_tex_wire_zbuf(struct pvertex *verts, int nverts)
{
	polyfill_wire_zbuf(verts, nverts);	/* TODO */
}


#define VNEXT(p)	(((p) == vlast) ? varr : (p) + 1)
#define VPREV(p)	((p) == varr ? vlast : (p) - 1)
#define VSUCC(p, side)	((side) == 0 ? VNEXT(p) : VPREV(p))

/* extra bits of precision to use when interpolating colors.
 * try tweaking this if you notice strange quantization artifacts.
 */
#define COLOR_SHIFT	12


#define POLYFILL polyfill_flat
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_gouraud
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_tex_flat
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_tex_gouraud
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_flat
#undef GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_gouraud
#define GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_tex_flat
#undef GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_tex_gouraud
#define GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_flat
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_gouraud
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_tex_flat
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_tex_gouraud
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#undef ZBUF
#include "polytmpl.h"
#undef POLYFILL

/* ---- zbuffer variants ----- */

#define POLYFILL polyfill_flat_zbuf
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_gouraud_zbuf
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_tex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_tex_gouraud_zbuf
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_flat_zbuf
#undef GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_gouraud_zbuf
#define GOURAUD
#undef TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_tex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_alpha_tex_gouraud_zbuf
#define GOURAUD
#define TEXMAP
#define BLEND_ALPHA
#undef BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_flat_zbuf
#undef GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_gouraud_zbuf
#define GOURAUD
#undef TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_tex_flat_zbuf
#undef GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL

#define POLYFILL polyfill_add_tex_gouraud_zbuf
#define GOURAUD
#define TEXMAP
#undef BLEND_ALPHA
#define BLEND_ADD
#define ZBUF
#include "polytmpl.h"
#undef POLYFILL


void draw_line(struct pvertex *verts)
{
	int32_t x0, y0, x1, y1;
	int i, dx, dy, x_inc, y_inc, error;
	uint32_t *fb = pfill_fb.pixels;
	uint32_t color = PACK_RGB(verts[0].r, verts[0].g, verts[0].b);

	x0 = verts[0].x >> 8;
	y0 = verts[0].y >> 8;
	x1 = verts[1].x >> 8;
	y1 = verts[1].y >> 8;

	fb += y0 * pfill_fb.width + x0;

	dx = x1 - x0;
	dy = y1 - y0;

	if(dx >= 0) {
		x_inc = 1;
	} else {
		x_inc = -1;
		dx = -dx;
	}
	if(dy >= 0) {
		y_inc = pfill_fb.width;
	} else {
		y_inc = -pfill_fb.width;
		dy = -dy;
	}

	if(dx > dy) {
		error = dy * 2 - dx;
		for(i=0; i<=dx; i++) {
			*fb = color;
			if(error >= 0) {
				error -= dx * 2;
				fb += y_inc;
			}
			error += dy * 2;
			fb += x_inc;
		}
	} else {
		error = dx * 2 - dy;
		for(i=0; i<=dy; i++) {
			*fb = color;
			if(error >= 0) {
				error -= dy * 2;
				fb += x_inc;
			}
			error += dx * 2;
			fb += y_inc;
		}
	}
}

void draw_line_zbuf(struct pvertex *verts)
{
	int32_t x0, y0, x1, y1, z0, z1, z, dz, zslope;
	int i, dx, dy, x_inc, y_inc, error;
	uint32_t *fb = pfill_fb.pixels;
	uint32_t *zptr = pfill_zbuf;
	uint32_t color = PACK_RGB(verts[0].r, verts[0].g, verts[0].b);

	x0 = verts[0].x >> 8;
	y0 = verts[0].y >> 8;
	x1 = verts[1].x >> 8;
	y1 = verts[1].y >> 8;
	z0 = verts[0].z;
	z1 = verts[1].z;

	fb += y0 * pfill_fb.width + x0;
	zptr += y0 * pfill_fb.width + x0;

	dx = x1 - x0;
	dy = y1 - y0;
	dz = z1 - z0;

	if(dx >= 0) {
		x_inc = 1;
	} else {
		x_inc = -1;
		dx = -dx;
	}
	if(dy >= 0) {
		y_inc = pfill_fb.width;
	} else {
		y_inc = -pfill_fb.width;
		dy = -dy;
	}

	z = z0;

	if(dx > dy) {
		zslope = dx ? (dz << 8) / (verts[1].x - verts[0].x) : 0;
		error = dy * 2 - dx;
		for(i=0; i<=dx; i++) {
			if(z <= *zptr) {
				*fb = color;
				*zptr = z;
			}
			if(error >= 0) {
				error -= dx * 2;
				fb += y_inc;
				zptr += y_inc;
			}
			error += dy * 2;
			fb += x_inc;

			zptr += x_inc;
			z += zslope;
		}
	} else {
		zslope = dy ? (dz << 8) / (verts[1].y - verts[0].y) : 0;
		error = dx * 2 - dy;
		for(i=0; i<=dy; i++) {
			if(z <= *zptr) {
				*fb = color;
				*zptr = z;
			}
			if(error >= 0) {
				error -= dy * 2;
				fb += x_inc;
				zptr += x_inc;
			}
			error += dx * 2;
			fb += y_inc;

			zptr += y_inc;
			z += zslope;
		}
	}
}
