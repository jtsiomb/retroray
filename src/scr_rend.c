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
#include "gaw/gaw.h"
#include "app.h"

static int rend_init(void);
static void rend_destroy(void);
static int rend_start(void);
static void rend_stop(void);
static void rend_display(void);
static void rend_reshape(int x, int y);
static void rend_keyb(int key, int press);
static void rend_mouse(int bn, int press, int x, int y);
static void rend_motion(int x, int y);


struct app_screen scr_rend = {
	"renderer",
	rend_init, rend_destroy,
	rend_start, rend_stop,
	rend_display, rend_reshape,
	rend_keyb, rend_mouse, rend_motion
};


static int rend_init(void)
{
	return 0;
}

static void rend_destroy(void)
{
}

static int rend_start(void)
{
	return 0;
}

static void rend_stop(void)
{
}

static void rend_display(void)
{
}

static void rend_reshape(int x, int y)
{
	gaw_matrix_mode(GAW_PROJECTION);
	gaw_load_identity();
	gaw_perspective(50, win_aspect, 0.5, 100.0);
}

static void rend_keyb(int key, int press)
{
}

static void rend_mouse(int bn, int press, int x, int y)
{
}

static void rend_motion(int x, int y)
{
}
