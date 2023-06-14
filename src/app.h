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
#ifndef APP_H_
#define APP_H_

#include "sizeint.h"
#include "logger.h"
#include "scene.h"

enum {
	KEY_BACKSP = 8,
	KEY_ESC	= 27,
	KEY_DEL	= 127,

	KEY_NUM_0 = 256, KEY_NUM_1, KEY_NUM_2, KEY_NUM_3, KEY_NUM_4,
	KEY_NUM_5, KEY_NUM_6, KEY_NUM_7, KEY_NUM_8, KEY_NUM_9,
	KEY_NUM_DOT, KEY_NUM_DIV, KEY_NUM_MUL, KEY_NUM_MINUS, KEY_NUM_PLUS, KEY_NUM_ENTER, KEY_NUM_EQUALS,
	KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT,
	KEY_INS, KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
	KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
	KEY_F13, KEY_F14, KEY_F15,
	KEY_NUMLK, KEY_CAPSLK, KEY_SCRLK,
	KEY_RSHIFT, KEY_LSHIFT, KEY_RCTRL, KEY_LCTRL, KEY_RALT, KEY_LALT,
	KEY_RMETA, KEY_LMETA, KEY_LSUPER, KEY_RSUPER, KEY_MODE, KEY_COMPOSE,
	KEY_HELP, KEY_PRINT, KEY_SYSRQ, KEY_BREAK
};

#ifndef KEY_ANY
#define KEY_ANY		(-1)
#define KEY_ALT		(-2)
#define KEY_CTRL	(-3)
#define KEY_SHIFT	(-4)
#endif

enum {
	KEY_MOD_SHIFT	= 1,
	KEY_MOD_CTRL	= 4,
	KEY_MOD_ALT	= 8
};


struct app_screen {
	const char *name;

	int (*init)(void);
	void (*destroy)(void);
	int (*start)(void);
	void (*stop)(void);
	void (*display)(void);
	void (*reshape)(int, int);
	void (*keyboard)(int, int);
	void (*mouse)(int, int, int, int);
	void (*motion)(int, int);
	void (*sball_motion)(int, int, int);
	void (*sball_rotate)(int, int, int);
	void (*sball_button)(int, int);
};

extern int mouse_x, mouse_y, mouse_state[3];
extern unsigned int modkeys;
extern int win_width, win_height;
extern float win_aspect;
extern int fullscr;

extern long time_msec;
extern struct app_screen *cur_scr;
extern struct app_screen scr_model, scr_rend;

struct font;
extern struct font *uifont;

extern uint32_t *framebuf;

extern struct scene *scn;


int app_init(void);
void app_shutdown(void);

void app_display(void);
void app_reshape(int x, int y);
void app_keyboard(int key, int press);
void app_mouse(int bn, int st, int x, int y);
void app_motion(int x, int y);
void app_sball_motion(int x, int y, int z);
void app_sball_rotate(int x, int y, int z);
void app_sball_button(int bn, int st);

void app_chscr(struct app_screen *scr);

/* defined in main.c */
long app_getmsec(void);
void app_redisplay(void);
void app_swap_buffers(void);
void app_quit(void);
void app_resize(int x, int y);
void app_fullscreen(int fs);
void app_vsync(int vsync);

#endif	/* APP_H_ */
