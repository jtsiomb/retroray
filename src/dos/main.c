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
#include <ctype.h>
#include <time.h>
#include "app.h"
#include "keyb.h"
#include "vidsys.h"
#include "cdpmi.h"
#include "mouse.h"
#include "logger.h"
#include "options.h"
#include "cpuid.h"
#include "util.h"
#include "rtk.h"

static INLINE int clamp(int x, int a, int b)
{
	if(x < a) return a;
	if(x > b) return b;
	return x;
}

static void draw_cursor(int x, int y);
static void draw_rband(rtk_rect *r);

static uint32_t *vmem;
static int quit, dirty_valid;
static rtk_rect dirty;
static int mx, my, prev_mx, prev_my;
static rtk_rect rband, prev_rband;


int main(int argc, char **argv)
{
	int i;
	int vmidx;
	int mdx, mdy, bnstate, bndiff;
	static int prev_bnstate;
	char *env;

#ifdef __DJGPP__
	__djgpp_nearptr_enable();
#endif

	init_logger();

	if(read_cpuid(&cpuid) == 0) {
		print_cpuid(&cpuid);
	}

	kb_init();

	if(!have_mouse()) {
		fprintf(stderr, "No mouse detected. Make sure the mouse driver is installed\n");
		return 1;
	}

	if((env = getenv("RRLOG"))) {
		if(tolower(env[0]) == 'c' && tolower(env[1]) == 'o' && tolower(env[2]) == 'm'
				&& isdigit(env[3])) {
			add_log_console(env);
		} else {
			add_log_file(env);
		}
	}

	if(vid_init() == -1) {
		return 1;
	}

	if((vmidx = vid_findmode(640, 480, 32)) == -1) {
		return 1;
	}
	if(!(vmem = vid_setmode(vmidx))) {
		return 1;
	}

	win_width = 640;
	win_height = 480;
	win_aspect = (float)win_width / (float)win_height;

	if(app_init() == -1) {
		goto break_evloop;
	}
	app_redisplay(0, 0, 0, 0);

	app_reshape(win_width, win_height);
	mx = win_width / 2;
	my = win_height / 2;
	prev_mx = prev_my = -1;

	for(;;) {
		int key;

		modkeys = 0;
		if(kb_isdown(KEY_ALT)) {
			modkeys |= KEY_MOD_ALT;
		}
		if(kb_isdown(KEY_CTRL)) {
			modkeys |= KEY_MOD_CTRL;
		}
		if(kb_isdown(KEY_SHIFT)) {
			modkeys |= KEY_MOD_SHIFT;
		}

		while((key = kb_getkey()) != -1) {
			if(key == 'r' && (modkeys & KEY_MOD_CTRL)) {
				app_redisplay(0, 0, 0, 0);
			} else {
				app_keyboard(key, 1);
			}
			if(quit) goto break_evloop;
		}

		bnstate = read_mouse_bn();
		bndiff = bnstate ^ prev_bnstate;
		prev_bnstate = bnstate;

		read_mouse_rel(&mdx, &mdy);
		mx = clamp(mx + mdx, 0, win_width - 1);
		my = clamp(my + mdy, 0, win_height - 1);
		mdx = mx - prev_mx;
		mdy = my - prev_my;

		if(bndiff & 1) app_mouse(0, bnstate & 1, mx, my);
		if(bndiff & 2) app_mouse(1, bnstate & 2, mx, my);
		if(bndiff & 4) app_mouse(3, bnstate & 4, mx, my);

		if((mdx | mdy) != 0) {
			app_motion(mx, my);
		}

		app_display();
		app_swap_buffers();
	}

break_evloop:
	app_shutdown();
	vid_cleanup();
	kb_shutdown();
	return 0;
}

long app_getmsec(void)
{
	return time(0) * 1000;	/* TODO */
}

void app_redisplay(int x, int y, int w, int h)
{
	rtk_rect r;

	if((w | h) == 0) {
		r.x = r.y = 0;
		r.width = win_width;
		r.height = win_height;
	} else {
		r.x = x;
		r.y = y;
		r.width = w;
		r.height = h;
	}

	if(dirty_valid) {
		rtk_rect_union(&dirty, &r);
	} else {
		dirty = r;
	}
	dirty_valid = 1;
}

void app_swap_buffers(void)
{
	if(opt.vsync) {
		vid_vsync();
	}
	if(dirty_valid) {
		if(dirty.width < win_width || dirty.height < win_height) {
			uint32_t *src = framebuf + dirty.y * win_width + dirty.x;
			vid_blit32(dirty.x, dirty.y, dirty.width, dirty.height, src, 0);
		} else {
			vid_blitfb32(framebuf, 0);
		}
		dirty_valid = 0;
	}
	if(prev_mx >= 0) {
		draw_cursor(prev_mx, prev_my);
	}
	draw_cursor(mx, my);
	prev_mx = mx;
	prev_my = my;

	if(prev_rband.width) {
		draw_rband(&prev_rband);
	}
	if(rband.width) {
		draw_rband(&rband);
	}
	prev_rband = rband;
}

void app_quit(void)
{
	quit = 1;
}

void app_resize(int x, int y)
{
}

void app_fullscreen(int fs)
{
}

void app_vsync(int vsync)
{
}

void app_rband(int x, int y, int w, int h)
{
	if(!(w | h)) {
		w = h = 0;
	}

	rband.x = x;
	rband.y = y;
	rband.width = w;
	rband.height = h;
}

static void draw_cursor(int x, int y)
{
	int i;
	uint32_t *fbptr = vmem + y * win_width + x;

	for(i=0; i<3; i++) {
		int offs = i + 1;
		if(y > offs) fbptr[-win_width * offs] ^= 0xffffff;
		if(y < win_height - offs - 1) fbptr[win_width * offs] ^= 0xffffff;
		if(x > offs) fbptr[-offs] ^= 0xffffff;
		if(x < win_width - offs - 1) fbptr[offs] ^= 0xffffff;
	}
}

static void draw_rband(rtk_rect *r)
{
	int i;
	rtk_rect rect;
	uint32_t *fbptr, *bptr;

	rect = *r;
	rtk_fix_rect(&rect);

	if(rect.width <= 0 || rect.height <= 0) {
		return;
	}

	fbptr = vmem + rect.y * win_width + rect.x;
	bptr = fbptr + win_width * (rect.height - 1);

	for(i=0; i<rect.width; i++) {
		fbptr[i] ^= 0xffffff;
		bptr[i] ^= 0xffffff;
	}
	fbptr += win_width;
	for(i=0; i<rect.height-2; i++) {
		fbptr[0] ^= 0xffffff;
		fbptr[rect.width - 1] ^= 0xffffff;
		fbptr += win_width;
	}
}
