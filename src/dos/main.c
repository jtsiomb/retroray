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
#include "gfx.h"
#include "cdpmi.h"
#include "mouse.h"
#include "options.h"

static uint32_t *vmem;
static int quit, disp_pending;

int main(int argc, char **argv)
{
	int i;
	int vmidx;
	int mx, my, bnstate, bndiff;
	static int prev_mx, prev_my, prev_bnstate;

#ifdef __DJGPP__
	__djgpp_nearptr_enable();
#endif

	kb_init(32);

	if(!have_mouse()) {
		fprintf(stderr, "No mouse detected. Make sure the mouse driver is installed\n");
		return 1;
	}
	set_mouse_limits(0, 0, 639, 479);
	set_mouse(320, 240);

	if(init_video() == -1) {
		return 1;
	}

	if((vmidx = match_video_mode(640, 480, 32)) == -1) {
		return 1;
	}
	if(!(vmem = set_video_mode(vmidx, 1))) {
		return 1;
	}

	win_width = 640;
	win_height = 480;
	win_aspect = (float)win_width / (float)win_height;

	if(app_init() == -1) {
		goto break_evloop;
	}
	disp_pending = 1;

	for(;;) {
		int key;
		while((key = kb_getkey()) != -1) {
			app_keyboard(key, 1);
			if(quit) goto break_evloop;
		}

		bnstate = read_mouse(&mx, &my);
		bndiff = bnstate ^ prev_bnstate;

		if(bndiff & 1) app_mouse(0, bnstate & 1, mx, my);
		if(bndiff & 2) app_mouse(1, bnstate & 2, mx, my);
		if(bndiff & 4) app_mouse(3, bnstate & 4, mx, my);

		if(disp_pending) {
			disp_pending = 0;
			app_display();
		}
	}

break_evloop:
	app_shutdown();
	set_text_mode();
	cleanup_video();
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
	blit_frame(framebuf, opt.vsync);
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
