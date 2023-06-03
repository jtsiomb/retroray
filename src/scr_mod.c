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

static int mdl_init(void);
static void mdl_destroy(void);
static int mdl_start(void);
static void mdl_stop(void);
static void mdl_display(void);
static void mdl_reshape(int x, int y);
static void mdl_keyb(int key, int press);
static void mdl_mouse(int bn, int press, int x, int y);
static void mdl_motion(int x, int y);


struct app_screen scr_model = {
	"modeller",
	mdl_init, mdl_destroy,
	mdl_start, mdl_stop,
	mdl_display, mdl_reshape,
	mdl_keyb, mdl_mouse, mdl_motion
};


static int mdl_init(void)
{
	return 0;
}

static void mdl_destroy(void)
{
}

static int mdl_start(void)
{
	return 0;
}

static void mdl_stop(void)
{
}

static void mdl_display(void)
{
}

static void mdl_reshape(int x, int y)
{
	gaw_matrix_mode(GAW_PROJECTION);
	gaw_load_identity();
	gaw_perspective(50, win_aspect, 0.5, 100.0);
}

static void mdl_keyb(int key, int press)
{
}

static void mdl_mouse(int bn, int press, int x, int y)
{
}

static void mdl_motion(int x, int y)
{
}
