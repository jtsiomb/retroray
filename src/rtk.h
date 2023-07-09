#ifndef RTK_H_
#define RTK_H_

#include "sizeint.h"

/* widget type */
enum {
	RTK_ANY,
	RTK_WIN,
	RTK_BUTTON,
	RTK_LABEL,
	RTK_CHECKBOX,
	RTK_TEXTBOX,
	RTK_SLIDER,
	RTK_SEP
};
/* window layout */
enum { RTK_NONE, RTK_VBOX, RTK_HBOX };
/* window flags */
enum {
	RTK_WIN_FRAME		= 1,
	RTK_WIN_MOVABLE		= 2,
	RTK_WIN_RESIZABLE	= 4
};
/* button mode */
enum { RTK_PUSHBN, RTK_TOGGLEBN };

typedef struct rtk_screen rtk_screen;
typedef struct rtk_widget rtk_widget;
typedef struct rtk_icon rtk_icon;
typedef struct rtk_iconsheet rtk_iconsheet;

typedef struct rtk_rect {
	int x, y, width, height;
} rtk_rect;

typedef struct rtk_icon {
	char *name;
	int width, height, scanlen;
	uint32_t *pixels;

	struct rtk_icon *next;
} rtk_icon;


typedef struct rtk_draw_ops {
	void (*fill)(rtk_rect *rect, uint32_t color);
	void (*blit)(int x, int y, rtk_icon *icon);
	void (*drawtext)(int x, int y, const char *str);
	void (*textrect)(const char *str, rtk_rect *rect);
} rtk_draw_ops;

typedef void (*rtk_callback)(rtk_widget*, void*);

/* global state */
void rtk_setup(rtk_draw_ops *drawop);

/* widget functions */
rtk_widget *rtk_create_widget(int type);
void rtk_free_widget(rtk_widget *w);

int rtk_type(rtk_widget *w);
rtk_widget *rtk_parent(rtk_widget *w);

void rtk_move(rtk_widget *w, int x, int y);
void rtk_pos(rtk_widget *w, int *xptr, int *yptr);
void rtk_resize(rtk_widget *w, int xsz, int ysz);
void rtk_size(rtk_widget *w, int *xptr, int *yptr);
void rtk_get_rect(rtk_widget *w, rtk_rect *r);

int rtk_set_text(rtk_widget *w, const char *str);
const char *rtk_get_text(rtk_widget *w);

void rtk_set_value(rtk_widget *w, int val);
int rtk_get_value(rtk_widget *w);

void rtk_set_callback(rtk_widget *w, rtk_callback cbfunc, void *cls);

void rtk_show(rtk_widget *w);
void rtk_hide(rtk_widget *w);
int rtk_visible(const rtk_widget *w);

void rtk_invalidate(rtk_widget *w);
void rtk_validate(rtk_widget *w);

/* window functions */
void rtk_win_layout(rtk_widget *w, int layout);
void rtk_win_clear(rtk_widget *w);
void rtk_win_add(rtk_widget *par, rtk_widget *child);
void rtk_win_rm(rtk_widget *par, rtk_widget *child);
int rtk_win_has(rtk_widget *par, rtk_widget *child);

/* button functions */
void rtk_bn_mode(rtk_widget *w, int mode);
void rtk_bn_set_icon(rtk_widget *w, rtk_icon *icon);
rtk_icon *rtk_bn_get_icon(rtk_widget *w);

/* slider functions */
void rtk_slider_set_range(rtk_widget *w, int vmin, int vmax);
void rtk_slider_get_range(const rtk_widget *w, int *vmin, int *vmax);

/* basic widgets */
rtk_widget *rtk_create_window(rtk_widget *par, const char *title, int x, int y,
		int width, int height, unsigned int flags);
rtk_widget *rtk_create_button(rtk_widget *par, const char *str, rtk_callback cbfunc);
rtk_widget *rtk_create_iconbutton(rtk_widget *par, rtk_icon *icon, rtk_callback cbfunc);
rtk_widget *rtk_create_label(rtk_widget *par, const char *text);
rtk_widget *rtk_create_checkbox(rtk_widget *par, const char *text, int chk, rtk_callback cbfunc);
rtk_widget *rtk_create_textbox(rtk_widget *par, const char *text, rtk_callback cbfunc);
rtk_widget *rtk_create_slider(rtk_widget *par, int vmin, int vmax, int val, rtk_callback cbfunc);
rtk_widget *rtk_create_separator(rtk_widget *par);

/* compound widgets */
rtk_widget *rtk_create_field(rtk_widget *par, const char *lbtext, rtk_callback cbfunc);

/* icon functions */
rtk_iconsheet *rtk_load_iconsheet(const char *fname);
void rtk_free_iconsheet(rtk_iconsheet *is);

rtk_icon *rtk_define_icon(rtk_iconsheet *is, const char *name, int x, int y, int w, int h);
rtk_icon *rtk_lookup_icon(rtk_iconsheet *is, const char *name);


void rtk_draw_widget(rtk_widget *w);

/* screen functions */
rtk_screen *rtk_create_screen(void);
void rtk_free_screen(rtk_screen *scr);

int rtk_add_window(rtk_screen *scr, rtk_widget *win);

int rtk_input_resize(rtk_screen *scr, int x, int y);
int rtk_input_key(rtk_screen *scr, int key, int press);
int rtk_input_mbutton(rtk_screen *scr, int bn, int press, int x, int y);
int rtk_input_mmotion(rtk_screen *scr, int x, int y);

/* misc */
void rtk_fix_rect(rtk_rect *r);
void rtk_rect_union(rtk_rect *a, const rtk_rect *b);

#endif	/* RTK_H_ */
