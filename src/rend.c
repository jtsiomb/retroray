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
#include "rt.h"

struct img_pixmap renderbuf;
struct img_pixmap dbgimg;

static int rx, ry, rwidth, rheight;
static int roffs;
static int xstep, ystep;

int rend_init(void)
{
	img_init(&renderbuf);

	img_init(&dbgimg);
	img_load(&dbgimg, "data/foo.jpg");
	img_convert(&dbgimg, IMG_FMT_RGBA32);

	rx = ry = rwidth = rheight = roffs = 0;
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
	int i, j, offs;
	uint32_t *src, *dest;

	src = (uint32_t*)dbgimg.pixels + roffs;
	dest = (uint32_t*)renderbuf.pixels + roffs;
	if(fb) fb += roffs;

	for(i=0; i<rheight; i+=ystep) {
		for(j=0; j<rwidth; j+=xstep) {
			offs = i * renderbuf.width + j;
			dest[offs] = src[offs];
			if(fb) {
				fb[offs] = src[offs];
				fillrect(fb, j, i, xstep, ystep, src[offs]);
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
