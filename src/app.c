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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gaw/gaw.h"
#include "app.h"
#include "timer.h"
#include "rend.h"
#include "options.h"
#include "font.h"
#include "util.h"
#include "gfxutil.h"
#include "rtk.h"

#ifdef GFX_SW
#include "gaw/gaw_sw.h"
#endif

int mouse_x, mouse_y, mouse_state[3];
unsigned int modkeys;
int win_width, win_height;
float win_aspect;
int fullscr;

long time_msec;

struct app_screen *cur_scr;

struct font *uifont;

uint32_t *framebuf;

struct scene *scn;
const char *scn_fname;

/* available screens */
#define MAX_SCREENS	8
static struct app_screen *screens[MAX_SCREENS];
static int num_screens;


int app_init(void)
{
	int i;
	char *start_scr_name;
	static rtk_draw_ops guigfx = {gui_begin, gui_end, gui_fill, gui_blit,
		gui_drawtext, gui_textrect};

#if !defined(NDEBUG) && defined(DBG_FPEXCEPT)
	infomsg("floating point exceptions enabled\n");
	enable_fpexcept();
#endif

#ifdef GFX_SW
	gaw_sw_init();
#endif
	rend_init();

	app_vsync(opt.vsync);
	if(opt.fullscreen) {
		app_fullscreen(1);
	}

#ifdef GFX_SW
	/*dtx_target_user(txdraw, 0);*/
	dtx_target_raster((unsigned char*)framebuf, win_width, win_height);
	dtx_set(DTX_RASTER_THRESHOLD, 127);
#endif

	uifont = malloc_nf(sizeof *uifont);
	if(load_font(uifont, "data/uifont14.gmp") == -1) {
		free(uifont);
		return -1;
	}

	rtk_setup(&guigfx);

	if(!(scn = create_scene())) {
		return -1;
	}
	if(scn_fname && scn_load(scn, scn_fname) == -1) {
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

	time_msec = get_msec();

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

#ifdef GFX_SW
	gaw_sw_destroy();
#endif

	free_scene(scn);

	cleanup_logger();
}

void app_display(void)
{
	time_msec = get_msec();

	cur_scr->display();
}

void app_reshape(int x, int y)
{
	int numpix = x * y;
	int prev_numpix = win_width * win_height;

	if(!framebuf || numpix > prev_numpix) {
		void *tmp;
		if(!(tmp = realloc(framebuf, numpix * sizeof *framebuf))) {
			errormsg("failed to resize framebuffer to %dx%d\n", x, y);
			return;
		}
		framebuf = tmp;
	}
#ifdef GFX_SW
	gaw_sw_framebuffer(x, y, framebuf);
	dtx_target_raster((unsigned char*)framebuf, x, y);
#endif

	win_width = x;
	win_height = y;
	win_aspect = (float)x / (float)y;
	gaw_viewport(0, 0, x, y);

	if(cur_scr && cur_scr->reshape) {
		cur_scr->reshape(x, y);
	}

	app_redisplay(0, 0, 0, 0);
}

void app_keyboard(int key, int press)
{
	long msec;
	static long prev_esc;

	if(press) {
		switch(key) {
#ifdef DBG_ESCQUIT
		case 27:
			msec = get_msec();
			if(msec - prev_esc < 1000) {
				app_quit();
				return;
			}
			prev_esc = msec;
			break;
#endif

		case 'q':
			if(modkeys & KEY_MOD_CTRL) {
				app_quit();
				return;
			}
			break;

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


void gui_begin(void)
{
#ifdef GFX_GL
	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_push_matrix();
	gaw_load_identity();
	gaw_matrix_mode(GAW_PROJECTION);
	gaw_push_matrix();
	gaw_load_identity();
	gaw_ortho(0, win_width, win_height, 0, -1, 1);

	gaw_save();
	gaw_disable(GAW_DEPTH_TEST);
	gaw_disable(GAW_CULL_FACE);
	gaw_disable(GAW_LIGHTING);
#endif
}

void gui_end(void)
{
#ifdef GFX_GL
	gaw_restore();

	gaw_matrix_mode(GAW_PROJECTION);
	gaw_pop_matrix();
	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_pop_matrix();
#endif
}

void gui_fill(rtk_rect *rect, uint32_t color)
{
#ifdef GFX_GL
	float r = UNP_RED(color) / 255.0f;
	float g = UNP_GREEN(color) / 255.0f;
	float b = UNP_BLUE(color) / 255.0f;
	gaw_color3f(r, g, b);
	gaw_rect(rect->x, rect->y, rect->x + rect->width, rect->y + rect->height);
#else
	int i, j;
	uint32_t *fb;

	if(rect->x < 0) {
		rect->width += rect->x;
		rect->x = 0;
	}
	if(rect->y < 0) {
		rect->height += rect->y;
		rect->y = 0;
	}
	if(rect->x + rect->width >= win_width) {
		rect->width = win_width - rect->x;
	}
	if(rect->y + rect->height >= win_height) {
		rect->height = win_height - rect->y;
	}

	fb = framebuf + rect->y * win_width + rect->x;
	for(i=0; i<rect->height; i++) {
		for(j=0; j<rect->width; j++) {
			fb[j] = color;
		}
		fb += win_width;
	}
#endif
}

void gui_blit(int x, int y, rtk_icon *icon)
{
#ifdef GFX_GL
	gaw_pixelzoom(opt.scale, opt.scale);
	gaw_drawpix(x, y, icon->width, icon->height, icon->scanlen, GAW_RGBA, icon->pixels);
#else
	int i, j;
	uint32_t *dest, *src;

	dest = framebuf + y * win_width + x;
	src = icon->pixels;

	for(i=0; i<icon->height; i++) {
		for(j=0; j<icon->width; j++) {
			int r = src[j] & 0xff;
			int g = (src[j] >> 8) & 0xff;
			int b = (src[j] >> 16) & 0xff;
			dest[j] = src[j];
		}
		dest += win_width;
		src += icon->scanlen;
	}
#endif
}

void gui_drawtext(int x, int y, const char *str)
{
#ifdef GFX_GL
	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_push_matrix();
	gaw_translate(x, y, 0);
	gaw_scale(1, -1, 1);
#endif

	use_font(uifont);
#ifdef GFX_GL
	gaw_color4f(0, 0, 0, 1);
#else
	dtx_position(x, y);
	dtx_color(0, 0, 0, 1);
#endif
	dtx_string(str);
	dtx_flush();

#ifdef GFX_GL
	gaw_matrix_mode(GAW_MODELVIEW);
	gaw_pop_matrix();
#endif
}

void gui_textrect(const char *str, rtk_rect *rect)
{
	struct dtx_box dbox;

	use_font(uifont);
	dtx_string_box(str, &dbox);

	rect->x = dbox.x;
	rect->y = dbox.y;
	rect->width = dbox.width;
	rect->height = dbox.height;
}
