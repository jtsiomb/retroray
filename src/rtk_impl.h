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
#ifndef RTK_IMPL_H_
#define RTK_IMPL_H_

#include <assert.h>
#include "sizeint.h"
#include "rtk.h"

enum {
	VISIBLE		= 0x0001,
	ENABLED		= 0x0002,
	HOVER		= 0x0010,
	PRESS		= 0x0020,
	FOCUS		= 0x0040,
	GEOMCHG		= 0x0100,
	DIRTY		= 0x0200,
	CANFOCUS	= 0x0400,
	AUTOWIDTH	= 0x1000,
	AUTOHEIGHT	= 0x2000,

	/* window flags */
	FRAME		= RTK_WIN_FRAME << 16,
	MOVABLE		= RTK_WIN_MOVABLE << 16,
	RESIZABLE	= RTK_WIN_RESIZABLE << 16,

	DBGRECT		= 0x40000000
};

#define WIDGET_COMMON \
	int type; \
	int x, y, width, height; \
	int absx, absy; \
	int pad; \
	char *text; \
	int value; \
	unsigned int flags; \
	struct rtk_window *par; \
	rtk_widget *next; \
	rtk_callback cbfunc, drawcb; \
	void *cbcls, *drawcls; \
	void *udata; \
	rtk_key_callback on_key; \
	rtk_mbutton_callback on_mbutton; \
	rtk_click_callback on_click; \
	rtk_drag_callback on_drag; \
	rtk_drop_callback on_drop; \
	rtk_screen *scr

typedef struct rtk_widget {
	WIDGET_COMMON;
} rtk_widget;

typedef struct rtk_window {
	WIDGET_COMMON;
	rtk_widget *clist, *ctail;
	int layout;
} rtk_window;

typedef struct rtk_button {
	WIDGET_COMMON;
	int mode;
	rtk_icon *icon;
} rtk_button;

typedef struct rtk_textbox {
	WIDGET_COMMON;
	int cursor, scroll;
	int len, bufsz;
} rtk_textbox;

typedef struct rtk_slider {
	WIDGET_COMMON;
	int vmin, vmax;
	int dragging, prev_val;
} rtk_slider;

typedef struct rtk_iconsheet {
	int width, height;
	uint32_t *pixels;

	struct rtk_icon *icons;
} rtk_iconsheet;

#define MAX_WINDOWS	64

typedef struct rtk_screen {
	rtk_widget *winlist[MAX_WINDOWS];
	int num_win;
	rtk_widget *hover, *focus;
	rtk_window *focuswin;
	int prev_mx, prev_my;

	rtk_widget *press;					/* currently pressed widget */
	int press_x, press_y;				/* position of last mouse press */

	rtk_widget *modal;		/* which window is currently modal (null if none) */
} rtk_screen;

#define RTK_ASSERT_TYPE(w, t)	assert(w->type == t)

extern rtk_draw_ops rtk_gfx;

void rtk_init_drawing(void);
void rtk_calc_widget_rect(rtk_widget *w, rtk_rect *rect);
void rtk_abs_pos(rtk_widget *w, int *xpos, int *ypos);
int rtk_hittest(rtk_widget *w, int x, int y);
void rtk_invalfb(rtk_widget *w);
void rtk_clearfb(rtk_widget *w);

#define SLIDER_HANDLE_SZ	8
void rtk_slider_handle_rect(rtk_widget *w, rtk_rect *hr);

#endif	/* RTK_IMPL_H_ */
