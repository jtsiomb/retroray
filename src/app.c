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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gaw/gaw.h"
#include "app.h"
#include "options.h"
#include "font.h"
#include "util.h"

static void txdraw(struct dtx_vertex *v, int vcount, struct dtx_pixmap *pixmap, void *cls);

int mouse_x, mouse_y, mouse_state[3];
unsigned int modkeys;
int win_width, win_height;
float win_aspect;
int fullscr;

long time_msec;

struct app_screen *cur_scr;

struct font *uifont;

/* available screens */
#define MAX_SCREENS	8
static struct app_screen *screens[MAX_SCREENS];
static int num_screens;


int app_init(void)
{
	int i;
	char *start_scr_name;

#if !defined(NDEBUG) && defined(DBG_FPEXCEPT)
	printf("floating point exceptions enabled\n");
	enable_fpexcept();
#endif

	load_options("retroray.cfg");
	app_resize(opt.xres, opt.yres);
	app_vsync(opt.vsync);
	if(opt.fullscreen) {
		app_fullscreen(1);
	}

	dtx_target_user(txdraw, 0);

	uifont = malloc_nf(sizeof *uifont);
	if(load_font(uifont, "data/uifont12.gmp") == -1) {
		free(uifont);
		return -1;
	}

	/* initialize screens */
	screens[num_screens++] = &scr_model;
	screens[num_screens++] = &scr_rend;

	start_scr_name = getenv("START_SCREEN");

	for(i=0; i<num_screens; i++) {
		if(screens[i]->init() == -1) {
			return -1;
		}
	}

	time_msec = app_getmsec();

	for(i=0; i<num_screens; i++) {
		if(screens[i]->name && start_scr_name && strcmp(screens[i]->name, start_scr_name) == 0) {
			app_chscr(screens[i]);
			break;
		}
	}
	if(!cur_scr) {
		app_chscr(&scr_model);
	}

	return 0;
}

void app_shutdown(void)
{
	int i;

	putchar('\n');

	save_options("retroray.cfg");

	for(i=0; i<num_screens; i++) {
		if(screens[i]->destroy) {
			screens[i]->destroy();
		}
	}

	destroy_font(uifont);
	free(uifont);
}

void app_display(void)
{
	time_msec = app_getmsec();

	cur_scr->display();

	app_swap_buffers();
}

void app_reshape(int x, int y)
{
	win_width = x;
	win_height = y;
	win_aspect = (float)x / (float)y;
	gaw_viewport(0, 0, x, y);

	if(cur_scr && cur_scr->reshape) {
		cur_scr->reshape(x, y);
	}
}

void app_keyboard(int key, int press)
{
	if(press) {
		switch(key) {
#ifdef DBG_ESCQUIT
		case 27:
			app_quit();
			return;
#endif

		case '\n':
		case '\r':
			if(modkeys & KEY_MOD_ALT) {
		case KEY_F11:
				app_fullscreen(-1);
				return;
			}
			break;
		}
	}

	if(cur_scr && cur_scr->keyboard) {
		cur_scr->keyboard(key, press);
	}
}

void app_mouse(int bn, int st, int x, int y)
{
	mouse_x = x;
	mouse_y = y;
	if(bn < 3) {
		mouse_state[bn] = st;
	}

	if(cur_scr && cur_scr->mouse) {
		cur_scr->mouse(bn, st, x, y);
	}
}

void app_motion(int x, int y)
{
	if(cur_scr && cur_scr->motion) {
		cur_scr->motion(x, y);
	}
	mouse_x = x;
	mouse_y = y;
}

void app_sball_motion(int x, int y, int z)
{
	if(cur_scr->sball_motion) {
		cur_scr->sball_motion(x, y, z);
	}
}

void app_sball_rotate(int x, int y, int z)
{
	if(cur_scr->sball_rotate) {
		cur_scr->sball_rotate(x, y, z);
	}
}

void app_sball_button(int bn, int st)
{
	if(cur_scr->sball_button) {
		cur_scr->sball_button(bn, st);
	}
}

void app_chscr(struct app_screen *scr)
{
	struct app_screen *prev = cur_scr;

	if(!scr) return;

	if(scr->start && scr->start() == -1) {
		return;
	}
	if(scr->reshape) {
		scr->reshape(win_width, win_height);
	}

	if(prev && prev->stop) {
		prev->stop();
	}
	cur_scr = scr;
}

static void txdraw(struct dtx_vertex *v, int vcount, struct dtx_pixmap *pixmap, void *cls)
{
	/*
	int i, aref, npix;
	unsigned char *src, *dest;
	struct texture *tex = pixmap->udata;

	if(!tex) {
		struct img_pixmap *img = img_create();
		img_set_pixels(img, pixmap->width, pixmap->height, IMG_FMT_RGBA32, 0);

		npix = pixmap->width * pixmap->height;
		src = pixmap->pixels;
		dest = img->pixels;
		for(i=0; i<npix; i++) {
			dest[0] = dest[1] = dest[2] = 0xff;
			dest[3] = *src++;
			dest += 4;
		}

		if(!(tex = tex_image(img))) {
			return;
		}
		pixmap->udata = tex;
	}

	gaw_save();
	if(dtx_get(DTX_GL_BLEND)) {
		gaw_enable(GAW_BLEND);
		gaw_blend_func(GAW_SRC_ALPHA, GAW_ONE_MINUS_SRC_ALPHA);
	} else {
		gaw_disable(GAW_BLEND);
	}
	if((aref = dtx_get(DTX_GL_ALPHATEST))) {
		gaw_enable(GAW_ALPHA_TEST);
		gaw_alpha_func(GAW_GREATER, aref);
	} else {
		gaw_disable(GAW_ALPHA_TEST);
	}

	gaw_set_tex2d(tex->texid);

	gaw_begin(GAW_TRIANGLES);
	for(i=0; i<vcount; i++) {
		gaw_texcoord2f(v->s, v->t);
		gaw_vertex2f(v->x, v->y);
		v++;
	}
	gaw_end();

	gaw_restore();
	gaw_set_tex2d(0);
	*/
}
