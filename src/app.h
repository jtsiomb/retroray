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

enum {
	KEY_ESC	= 27,
	KEY_DEL	= 127,
	KEY_F1		= 256,
	KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7,
	KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
	KEY_PGUP, KEY_PGDOWN,
	KEY_HOME, KEY_END,
	KEY_INS
};

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
