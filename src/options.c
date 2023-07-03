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
#include <string.h>
#include <errno.h>
#include "options.h"
#include "treestor.h"
#include "logger.h"

#define DEF_XRES		640
#define DEF_YRES		480
#define DEF_BPP			32
#define DEF_VSYNC		1
#define DEF_FULLSCR		0
#define DEF_MOUSE_SPEED	50
#define DEF_SBALL_SPEED	50


struct options opt = {
	DEF_XRES, DEF_YRES, DEF_BPP,
	DEF_VSYNC,
	DEF_FULLSCR,
	DEF_MOUSE_SPEED, DEF_SBALL_SPEED,
};

int load_options(const char *fname)
{
	struct ts_node *cfg;

	if(!(cfg = ts_load(fname))) {
		return -1;
	}
	infomsg("loaded config: %s\n", fname);

	opt.xres = ts_lookup_int(cfg, "options.video.xres", DEF_XRES);
	opt.yres = ts_lookup_int(cfg, "options.video.yres", DEF_YRES);
	opt.bpp = ts_lookup_int(cfg, "options.video.bpp", DEF_BPP);
	opt.vsync = ts_lookup_int(cfg, "options.video.vsync", DEF_VSYNC);
	opt.fullscreen = ts_lookup_int(cfg, "options.video.fullscreen", DEF_FULLSCR);

	opt.mouse_speed = ts_lookup_int(cfg, "options.input.mousespeed", DEF_MOUSE_SPEED);
	opt.sball_speed = ts_lookup_int(cfg, "options.input.sballspeed", DEF_SBALL_SPEED);

	ts_free_tree(cfg);
	return 0;
}

#define WROPT(lvl, fmt, val, defval) \
	do { \
		int i; \
		for(i=0; i<lvl; i++) fputc('\t', fp); \
		if((val) == (defval)) fputc('#', fp); \
		fprintf(fp, fmt "\n", val); \
	} while(0)

int save_options(const char *fname)
{
	FILE *fp;

	printf("writing config: %s\n", fname);

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "failed to save options (%s): %s\n", fname, strerror(errno));
	}
	fprintf(fp, "options {\n");
	fprintf(fp, "\tvideo {\n");
	WROPT(2, "xres = %d", opt.xres, DEF_XRES);
	WROPT(2, "yres = %d", opt.yres, DEF_YRES);
	WROPT(2, "bpp = %d", opt.bpp, DEF_BPP);
	WROPT(2, "vsync = %d", opt.vsync, DEF_VSYNC);
	WROPT(2, "fullscreen = %d", opt.fullscreen, DEF_FULLSCR);
	fprintf(fp, "\t}\n");

	fprintf(fp, "\tinput {\n");
	WROPT(2, "mousespeed = %d", opt.mouse_speed, DEF_MOUSE_SPEED);
	WROPT(2, "sballspeed = %d", opt.sball_speed, DEF_SBALL_SPEED);
	fprintf(fp, "\t}\n");

	fprintf(fp, "}\n");
	fprintf(fp, "# v" "i:ts=4 sts=4 sw=4 noexpandtab:\n");

	fclose(fp);
	return 0;
}
