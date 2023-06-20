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
#include "texture.h"
#include "geom.h"
#include "imago2.h"
#include "logger.h"
#include "sizeint.h"
#include "util.h"

static cgm_vec3 lookup_pixmap(const struct texture *btex, const struct rayhit *hit);
static cgm_vec3 lookup_chess(const struct texture *btex, const struct rayhit *hit);
static cgm_vec3 lookup_fbm2d(const struct texture *btex, const struct rayhit *hit);
static cgm_vec3 lookup_fbm3d(const struct texture *btex, const struct rayhit *hit);
static cgm_vec3 lookup_marble2d(const struct texture *btex, const struct rayhit *hit);
static cgm_vec3 lookup_marble3d(const struct texture *btex, const struct rayhit *hit);

static cgm_vec3 (*lookup[])(const struct texture*, const struct rayhit*) = {
	lookup_pixmap,
	lookup_chess,
	lookup_fbm2d,
	lookup_fbm3d,
	lookup_marble2d,
	lookup_marble3d
};

static const char *defname_fmt[] = {
	"pixmap%03d", "chess%03d",
	"fbm%03d", "sfbm%03d",
	"marble%03d", "smarble%03d"
};

struct texture *create_texture(int type)
{
	struct texture *tex;
	struct tex_pixmap *tpix;
	struct tex_chess *tchess;
	struct tex_fbm *tfbm;
	static int texidx;
	char namebuf[64];

	switch(type) {
	case TEX_PIXMAP:
		if(!(tpix = malloc(sizeof *tpix))) {
			goto fail;
		}
		tex = (struct texture*)tpix;
		tpix->img = 0;
		break;

	case TEX_CHESS:
		if(!(tchess = malloc(sizeof *tchess))) {
			goto fail;
		}
		tex = (struct texture*)tchess;
		cgm_vcons(tchess->color, 0, 0, 0);
		cgm_vcons(tchess->color + 1, 1, 1, 1);
		break;

	case TEX_FBM2D:
	case TEX_FBM3D:
		if(!(tfbm = malloc(sizeof *tfbm))) {
			goto fail;
		}
		tex = (struct texture*)tfbm;
		tfbm->octaves = 1;
		cgm_vcons(tfbm->color, 0, 0, 0);
		cgm_vcons(tfbm->color + 1, 1, 1, 1);
		break;

	default:
		errormsg("tried to allocate invalid texture type (%d)\n", type);
		return 0;
	}

	tex->type = type;
	tex->name = 0;
	cgm_vcons(&tex->offs, 0, 0, 0);
	cgm_vcons(&tex->scale, 1, 1, 1);
	tex->lookup = lookup[type];

	sprintf(namebuf, defname_fmt[type], texidx++);
	tex_set_name(tex, namebuf);
	return tex;

fail:
	errormsg("failed to allocate texture\n");
	return 0;
}

void free_texture(struct texture *tex)
{
	if(!tex) return;
	free(tex->name);
	free(tex);
}

void tex_set_name(struct texture *tex, const char *name)
{
	char *tmp = strdup(name);
	if(!tmp) return;
	free(tex->name);
	tex->name = tmp;
}

#define XFORM_UV(tex, u, v) \
	do { \
		u *= (tex)->scale.x + (tex)->offs.x; \
		v *= (tex)->scale.y + (tex)->offs.y; \
	} while(0)

#define XFORM_UVW(tex, u, v, w) \
	do { \
		u *= (tex)->scale.x + (tex)->offs.x; \
		v *= (tex)->scale.y + (tex)->offs.y; \
		w *= (tex)->scale.z * (tex)->offs.z; \
	} while(0)

static cgm_vec3 lookup_pixmap(const struct texture *btex, const struct rayhit *hit)
{
	struct tex_pixmap *tex = (struct tex_pixmap*)btex;
	int px, py, twidth, theight;
	int r, g, b;
	uint32_t texel;
	float u = hit->uv.x;
	float v = hit->uv.y;

	twidth = tex->img->width;
	theight = tex->img->height;

	XFORM_UV(tex, u, v);

	px = cround64((float)twidth * u) % twidth;
	py = cround64((float)theight * v) % theight;
	if(px < 0) px += twidth;
	if(py < 0) py += theight;

	texel = ((uint32_t*)tex->img->pixels)[py * twidth + px];
	r = texel & 0xff;
	g = (texel >> 8) & 0xff;
	b = (texel >> 16) & 0xff;

	return cgm_vvec(r / 255.0f, g / 255.0f, b / 255.0f);
}

static cgm_vec3 lookup_chess(const struct texture *btex, const struct rayhit *hit)
{
	struct tex_chess *tex = (struct tex_chess*)btex;
	int cx, cy, chess;
	float u = hit->uv.x;
	float v = hit->uv.y;

	XFORM_UV(tex, u, v);

	cx = cround64(u) & 1;
	cy = cround64(v) & 1;
	chess = cx ^ cy;
	return tex->color[chess];
}

static cgm_vec3 lookup_fbm2d(const struct texture *btex, const struct rayhit *hit)
{
	return cgm_vvec(1, 0, 0);	/* TODO */
}

static cgm_vec3 lookup_fbm3d(const struct texture *btex, const struct rayhit *hit)
{
	return cgm_vvec(1, 0, 0);	/* TODO */
}

static cgm_vec3 lookup_marble2d(const struct texture *btex, const struct rayhit *hit)
{
	return cgm_vvec(1, 0, 0);	/* TODO */
}

static cgm_vec3 lookup_marble3d(const struct texture *btex, const struct rayhit *hit)
{
	return cgm_vvec(1, 0, 0);	/* TODO */
}
