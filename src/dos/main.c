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

static INLINE int clamp(int x, int a, int b)
{
	if(x < a) return a;
	if(x > b) return b;
	return x;
}

static void draw_cursor(int x, int y);

static uint32_t *vmem;
static int quit, disp_pending;

int main(int argc, char **argv)
{
	int i;
	int vmidx;
	int mx, my, mdx, mdy, prev_mx, prev_my, bnstate, bndiff;
	static int prev_bnstate;

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

	/*add_log_file("retroray.log");*/

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
	disp_pending = 1;

	app_reshape(win_width, win_height);
	mx = win_width / 2;
	my = win_height / 2;

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
			app_keyboard(key, 1);
			if(quit) goto break_evloop;
		}

		bnstate = read_mouse_bn();
		bndiff = bnstate ^ prev_bnstate;
		prev_bnstate = bnstate;

		read_mouse_rel(&mdx, &mdy);
		prev_mx = mx;
		prev_my = my;
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

		if(disp_pending) {
			disp_pending = 0;
			app_display();
		}

		draw_cursor(prev_mx, prev_my);
		draw_cursor(mx, my);

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

void app_redisplay(void)
{
	disp_pending = 1;
}

void app_swap_buffers(void)
{
	if(opt.vsync) {
		vid_vsync();
	}
	vid_blitfb32(framebuf, 0);
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

static void draw_cursor(int x, int y)
{
	int i;
	uint32_t *fbptr = framebuf + y * win_width + x;

	for(i=0; i<3; i++) {
		int offs = i + 1;
		if(y > offs) fbptr[-win_width * offs] ^= 0xffffff;
		if(y < win_height - offs - 1) fbptr[win_width * offs] ^= 0xffffff;
		if(x > offs) fbptr[-offs] ^= 0xffffff;
		if(x < win_width - offs - 1) fbptr[offs] ^= 0xffffff;
	}
}
