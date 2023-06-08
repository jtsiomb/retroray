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
#ifndef POLYFILL_H_
#define POLYFILL_H_

#include "../util.h"
#include "gaw.h"

#define POLYFILL_MODE_MASK	0x03
#define POLYFILL_TEX_BIT	0x04
#define POLYFILL_ALPHA_BIT	0x08
#define POLYFILL_ADD_BIT	0x10
#define POLYFILL_ZBUF_BIT	0x20

enum {
	POLYFILL_WIRE			= 0,
	POLYFILL_FLAT,
	POLYFILL_GOURAUD,

	POLYFILL_TEX_WIRE		= 4,
	POLYFILL_TEX_FLAT,
	POLYFILL_TEX_GOURAUD,

	POLYFILL_ALPHA_WIRE		= 8,
	POLYFILL_ALPHA_FLAT,
	POLYFILL_ALPHA_GOURAUD,

	POLYFILL_ALPHA_TEX_WIRE	= 12,
	POLYFILL_ALPHA_TEX_FLAT,
	POLYFILL_ALPHA_TEX_GOURAUD,

	POLYFILL_ADD_WIRE		= 16,
	POLYFILL_ADD_FLAT,
	POLYFILL_ADD_GOURAUD,

	POLYFILL_ADD_TEX_WIRE	= 20,
	POLYFILL_ADD_TEX_FLAT,
	POLYFILL_ADD_TEX_GOURAUD,


	POLYFILL_WIRE_ZBUF			= 32,
	POLYFILL_FLAT_ZBUF,
	POLYFILL_GOURAUD_ZBUF,

	POLYFILL_TEX_WIRE_ZBUF		= 36,
	POLYFILL_TEX_FLAT_ZBUF,
	POLYFILL_TEX_GOURAUD_ZBUF,

	POLYFILL_ALPHA_WIRE_ZBUF	= 40,
	POLYFILL_ALPHA_FLAT_ZBUF,
	POLYFILL_ALPHA_GOURAUD_ZBUF,

	POLYFILL_ALPHA_TEX_WIRE_ZBUF = 44,
	POLYFILL_ALPHA_TEX_FLAT_ZBUF,
	POLYFILL_ALPHA_TEX_GOURAUD_ZBUF,

	POLYFILL_ADD_WIRE_ZBUF		= 48,
	POLYFILL_ADD_FLAT_ZBUF,
	POLYFILL_ADD_GOURAUD_ZBUF,

	POLYFILL_ADD_TEX_WIRE_ZBUF	= 52,
	POLYFILL_ADD_TEX_FLAT_ZBUF,
	POLYFILL_ADD_TEX_GOURAUD_ZBUF
};

typedef uint32_t gaw_pixel;

#define PACK_RGB(r, g, b) \
	(0xff000000 | ((r) << 16) | ((g) << 8) | (b))
#define PACK_RGBA(r, g, b, a) \
	(((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define UNPACK_R(pix)	((pix) & 0xff)
#define UNPACK_G(pix)	(((pix) >> 8) & 0xff)
#define UNPACK_B(pix)	(((pix) >> 16) & 0xff)
#define UNPACK_A(pix)	((pix) >> 24)

struct vertex {
	float x, y, z, w;
	float nx, ny, nz;
	float u, v;
	int r, g, b, a;
};

/* projected vertices for the rasterizer */
struct pvertex {
	int32_t x, y; /* 24.8 fixed point */
	int32_t u, v; /* 16.16 fixed point */
	int32_t r, g, b, a;  /* int 0-255 */
	int32_t z;	/* 0-65535 */
};

struct pimage {
	gaw_pixel *pixels;
	int width, height;

	int xshift, yshift;
	unsigned int xmask, ymask;
};

extern struct pimage pfill_fb;
extern struct pimage pfill_tex;
extern uint32_t *pfill_zbuf;

void polyfill_fbheight(int height);

void polyfill(int mode, struct pvertex *verts, int nverts);

void polyfill_wire(struct pvertex *verts, int nverts);
void polyfill_flat(struct pvertex *verts, int nverts);
void polyfill_gouraud(struct pvertex *verts, int nverts);
void polyfill_tex_wire(struct pvertex *verts, int nverts);
void polyfill_tex_flat(struct pvertex *verts, int nverts);
void polyfill_tex_gouraud(struct pvertex *verts, int nverts);
void polyfill_alpha_wire(struct pvertex *verts, int nverts);
void polyfill_alpha_flat(struct pvertex *verts, int nverts);
void polyfill_alpha_gouraud(struct pvertex *verts, int nverts);
void polyfill_alpha_tex_wire(struct pvertex *verts, int nverts);
void polyfill_alpha_tex_flat(struct pvertex *verts, int nverts);
void polyfill_alpha_tex_gouraud(struct pvertex *verts, int nverts);
void polyfill_add_wire(struct pvertex *verts, int nverts);
void polyfill_add_flat(struct pvertex *verts, int nverts);
void polyfill_add_gouraud(struct pvertex *verts, int nverts);
void polyfill_add_tex_wire(struct pvertex *verts, int nverts);
void polyfill_add_tex_flat(struct pvertex *verts, int nverts);
void polyfill_add_tex_gouraud(struct pvertex *verts, int nverts);
void polyfill_flat_zbuf(struct pvertex *verts, int nverts);
void polyfill_gouraud_zbuf(struct pvertex *verts, int nverts);
void polyfill_tex_flat_zbuf(struct pvertex *verts, int nverts);
void polyfill_tex_gouraud_zbuf(struct pvertex *verts, int nverts);
void polyfill_alpha_flat_zbuf(struct pvertex *verts, int nverts);
void polyfill_alpha_gouraud_zbuf(struct pvertex *verts, int nverts);
void polyfill_alpha_tex_flat_zbuf(struct pvertex *verts, int nverts);
void polyfill_alpha_tex_gouraud_zbuf(struct pvertex *verts, int nverts);
void polyfill_add_flat_zbuf(struct pvertex *verts, int nverts);
void polyfill_add_gouraud_zbuf(struct pvertex *verts, int nverts);
void polyfill_add_tex_flat_zbuf(struct pvertex *verts, int nverts);
void polyfill_add_tex_gouraud_zbuf(struct pvertex *verts, int nverts);

void draw_line(struct pvertex *verts);

#endif	/* POLYFILL_H_ */
