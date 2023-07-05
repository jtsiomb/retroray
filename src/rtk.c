#include <stdlib.h>
#include <string.h>
#include "imago2.h"
#include "rtk_impl.h"
#include "app.h"

static rtk_draw_ops gfx;

static void calc_widget_rect(rtk_widget *w, rtk_rect *rect);
static void draw_window(rtk_widget *w);
static void draw_button(rtk_widget *w);
static void draw_checkbox(rtk_widget *w);
static void draw_textbox(rtk_widget *w);
static void draw_slider(rtk_widget *w);
static void draw_separator(rtk_widget *w);

static void invalfb(rtk_widget *w);
void inval_vport(void);	/* scr_mod.c */


static rtk_widget *hover, *focused, *pressed;
static int prev_mx, prev_my;

#define MAX_WINDOWS		32
static rtk_widget *winlist[MAX_WINDOWS];
static int num_win;


void rtk_setup(rtk_draw_ops *drawop)
{
	gfx = *drawop;
}

rtk_widget *rtk_create_widget(void)
{
	rtk_widget *w;

	if(!(w = calloc(1, sizeof *w))) {
		return 0;
	}
	w->any.flags = VISIBLE | ENABLED | GEOMCHG | DIRTY;
	return w;
}

void rtk_free_widget(rtk_widget *w)
{
	int i;
	if(!w) return;

	if(w->type == RTK_WIN) {
		if(!w->any.par) {
			for(i=0; i<num_win; i++) {
				if(winlist[i] == w) {
					if(i < num_win - 1) {
						winlist[i] = winlist[--num_win];
					} else {
						winlist[i] = 0;
						num_win--;
					}
					break;
				}
			}
		}

		while(w->win.clist) {
			rtk_widget *c = w->win.clist;
			w->win.clist = w->win.clist->any.next;
			rtk_free_widget(c);
		}
	}

	free(w->any.text);
	free(w);
}

int rtk_type(rtk_widget *w)
{
	return w->type;
}

void rtk_move(rtk_widget *w, int x, int y)
{
	if(!w->any.par) {
		invalfb(w);
	}
	w->any.x = x;
	w->any.y = y;
	w->any.flags |= GEOMCHG | DIRTY;
	if(!w->any.par) {
		invalfb(w);
		inval_vport();
	}
}

void rtk_pos(rtk_widget *w, int *xptr, int *yptr)
{
	*xptr = w->any.x;
	*yptr = w->any.y;
}

void rtk_resize(rtk_widget *w, int xsz, int ysz)
{
	if(!w->any.par) {
		invalfb(w);
	}
	w->any.width = xsz;
	w->any.height = ysz;
	w->any.flags |= GEOMCHG | DIRTY;
	if(!w->any.par) {
		invalfb(w);
		inval_vport();
	}
}

void rtk_size(rtk_widget *w, int *xptr, int *yptr)
{
	*xptr = w->any.width;
	*yptr = w->any.height;
}

void rtk_get_rect(rtk_widget *w, rtk_rect *r)
{
	r->x = w->any.x;
	r->y = w->any.y;
	r->width = w->any.width;
	r->height = w->any.height;
}

int rtk_set_text(rtk_widget *w, const char *str)
{
	rtk_rect rect;
	char *s = strdup(str);
	if(!s) return -1;

	free(w->any.text);
	w->any.text = s;

	calc_widget_rect(w, &rect);
	rtk_resize(w, rect.width, rect.height);
	rtk_invalidate(w);
	return 0;
}

const char *rtk_get_text(rtk_widget *w)
{
	return w->any.text;
}

void rtk_set_value(rtk_widget *w, int val)
{
	w->any.value = val;
	rtk_invalidate(w);
}

int rtk_get_value(rtk_widget *w)
{
	return w->any.value;
}

void rtk_set_callback(rtk_widget *w, rtk_callback cbfunc, void *cls)
{
	w->any.cbfunc = cbfunc;
	w->any.cbcls = cls;
}

void rtk_show(rtk_widget *w)
{
	w->any.flags |= VISIBLE;
	rtk_invalidate(w);
}

void rtk_hide(rtk_widget *w)
{
	w->any.flags &= ~VISIBLE;
	invalfb(w);
}

int rtk_visible(const rtk_widget *w)
{
	return w->any.flags & VISIBLE;
}

void rtk_invalidate(rtk_widget *w)
{
	w->any.flags |= DIRTY;
}

void rtk_validate(rtk_widget *w)
{
	w->any.flags &= ~DIRTY;
}

void rtk_win_layout(rtk_widget *w, int layout)
{
	w->win.layout = layout;
}

void rtk_win_clear(rtk_widget *w)
{
	rtk_widget *tmp;

	RTK_ASSERT_TYPE(w, RTK_WIN);

	while(w->win.clist) {
		tmp = w->win.clist;
		w->win.clist = w->win.clist->any.next;
		rtk_free_widget(tmp);
	}

	w->win.clist = w->win.ctail = 0;
	rtk_invalidate(w);
}

void rtk_win_add(rtk_widget *par, rtk_widget *child)
{
	RTK_ASSERT_TYPE(par, RTK_WIN);

	if(rtk_win_has(par, child)) {
		return;
	}

	if(child->any.par) {
		rtk_win_rm(child->any.par, child);
	}

	if(par->win.clist) {
		par->win.ctail->any.next = child;
		par->win.ctail = child;
	} else {
		par->win.clist = par->win.ctail = child;
	}
	child->any.next = 0;

	child->any.par = par;
	rtk_invalidate(par);
}

void rtk_win_rm(rtk_widget *par, rtk_widget *child)
{
	rtk_widget *prev, dummy;

	RTK_ASSERT_TYPE(par, RTK_WIN);

	dummy.any.next = par->win.clist;
	prev = &dummy;
	while(prev->any.next) {
		if(prev->any.next == child) {
			if(!child->any.next) {
				par->win.ctail = prev;
			}
			prev->any.next = child->any.next;
			break;
		}
		prev = prev->any.next;
	}
	par->win.clist = dummy.any.next;
	rtk_invalidate(par);
}

int rtk_win_has(rtk_widget *par, rtk_widget *child)
{
	rtk_widget *w;

	RTK_ASSERT_TYPE(par, RTK_WIN);

	w = par->win.clist;
	while(w) {
		if(w == child) {
			return 1;
		}
		w = w->any.next;
	}
	return 0;
}

/* --- button functions --- */
void rtk_bn_mode(rtk_widget *w, int mode)
{
	RTK_ASSERT_TYPE(w, RTK_BUTTON);
	w->bn.mode = mode;
	rtk_invalidate(w);
}

void rtk_bn_set_icon(rtk_widget *w, rtk_icon *icon)
{
	rtk_rect rect;

	RTK_ASSERT_TYPE(w, RTK_BUTTON);
	w->bn.icon = icon;

	calc_widget_rect(w, &rect);
	rtk_resize(w, rect.width, rect.height);
	rtk_invalidate(w);
}

rtk_icon *rtk_bn_get_icon(rtk_widget *w)
{
	RTK_ASSERT_TYPE(w, RTK_BUTTON);
	return w->bn.icon;
}

/* --- slider functions --- */
void rtk_slider_set_range(rtk_widget *w, int vmin, int vmax)
{
	RTK_ASSERT_TYPE(w, RTK_SLIDER);

	w->slider.vmin = vmin;
	w->slider.vmax = vmax;
}

void rtk_slider_get_range(const rtk_widget *w, int *vmin, int *vmax)
{
	RTK_ASSERT_TYPE(w, RTK_SLIDER);

	*vmin = w->slider.vmin;
	*vmax = w->slider.vmax;
}


/* --- constructors --- */

rtk_widget *rtk_create_window(rtk_widget *par, const char *title, int x, int y,
		int width, int height, unsigned int flags)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget())) {
		return 0;
	}
	w->type = RTK_WIN;
	if(par) {
		rtk_win_add(par, w);
	} else {
		if(num_win < MAX_WINDOWS) {
			winlist[num_win++] = w;
		}
	}

	rtk_set_text(w, title);
	rtk_move(w, x, y);
	rtk_resize(w, width, height);
	rtk_win_layout(w, RTK_VBOX);

	w->any.flags |= flags << 16;
	return w;
}

rtk_widget *rtk_create_button(rtk_widget *par, const char *str, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget())) {
		return 0;
	}
	w->type = RTK_BUTTON;
	if(par) rtk_win_add(par, w);
	rtk_set_text(w, str);
	rtk_set_callback(w, cbfunc, 0);
	return w;
}

rtk_widget *rtk_create_iconbutton(rtk_widget *par, rtk_icon *icon, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget())) {
		return 0;
	}
	w->type = RTK_BUTTON;
	if(par) rtk_win_add(par, w);
	rtk_bn_set_icon(w, icon);
	rtk_set_callback(w, cbfunc, 0);
	return w;
}

rtk_widget *rtk_create_label(rtk_widget *par, const char *text)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget())) {
		return 0;
	}
	w->type = RTK_LABEL;
	if(par) rtk_win_add(par, w);
	rtk_set_text(w, text);
	return w;
}

rtk_widget *rtk_create_checkbox(rtk_widget *par, const char *text, int chk, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget())) {
		return 0;
	}
	w->type = RTK_CHECKBOX;
	if(par) rtk_win_add(par, w);
	rtk_set_text(w, text);
	rtk_set_value(w, chk ? 1 : 0);
	rtk_set_callback(w, cbfunc, 0);
	return w;
}

rtk_widget *rtk_create_textbox(rtk_widget *par, const char *text, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget())) {
		return 0;
	}
	w->type = RTK_TEXTBOX;
	if(par) rtk_win_add(par, w);
	rtk_set_text(w, text);
	rtk_set_callback(w, cbfunc, 0);
	rtk_resize(w, 40, 1);
	return w;
}

rtk_widget *rtk_create_slider(rtk_widget *par, int vmin, int vmax, int val, rtk_callback cbfunc)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget())) {
		return 0;
	}
	w->type = RTK_SLIDER;
	if(par) rtk_win_add(par, w);
	rtk_set_callback(w, cbfunc, 0);
	rtk_slider_set_range(w, vmin, vmax);
	rtk_set_value(w, val);
	return w;
}

rtk_widget *rtk_create_separator(rtk_widget *par)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget())) {
		return 0;
	}
	w->type = RTK_SEP;
	if(par) rtk_win_add(par, w);
	return w;
}


/* --- icon functions --- */
rtk_iconsheet *rtk_load_iconsheet(const char *fname)
{
	 rtk_iconsheet *is;

	if(!(is = malloc(sizeof *is))) {
		return 0;
	}
	is->icons = 0;

	if(!(is->pixels = img_load_pixels(fname, &is->width, &is->height, IMG_FMT_RGBA32))) {
		free(is);
		return 0;
	}
	return is;
}

void rtk_free_iconsheet(rtk_iconsheet *is)
{
	rtk_icon *icon;

	img_free_pixels(is->pixels);

	while(is->icons) {
		icon = is->icons;
		is->icons = is->icons->next;
		free(icon->name);
		free(icon);
	}
	free(is);
}

rtk_icon *rtk_define_icon(rtk_iconsheet *is, const char *name, int x, int y, int w, int h)
{
	rtk_icon *icon;

	if(!(icon = malloc(sizeof *icon))) {
		return 0;
	}
	if(!(icon->name = strdup(name))) {
		free(icon);
		return 0;
	}
	icon->width = w;
	icon->height = h;
	icon->scanlen = is->width;
	icon->pixels = is->pixels + y * is->width + x;
	return icon;
}

#define BEVELSZ		1
#define PAD			2
#define OFFS		(BEVELSZ + PAD)
#define CHKBOXSZ	(BEVELSZ * 2 + 8)

static void calc_widget_rect(rtk_widget *w, rtk_rect *rect)
{
	rtk_rect txrect = {0};

	rect->x = w->any.x;
	rect->y = w->any.y;

	if(w->any.text) {
		gfx.textrect(w->any.text, &txrect);
	}

	switch(w->type) {
	case RTK_WIN:
		rect->width = w->any.width;
		rect->height = w->any.height;
		break;

	case RTK_BUTTON:
		if(w->bn.icon) {
			rect->width = w->bn.icon->width + OFFS * 2;
			rect->height = w->bn.icon->height + OFFS * 2;
		} else {
			rect->width = txrect.width + OFFS * 2;
			rect->height = txrect.height + OFFS * 2;
		}
		break;

	case RTK_CHECKBOX:
		rect->width = txrect.width + CHKBOXSZ + OFFS * 2 + PAD;
		rect->height = txrect.height + OFFS * 2;
		break;

	case RTK_LABEL:
		rect->width = txrect.width + PAD * 2;
		rect->height = txrect.height + PAD * 2;
		break;

	case RTK_TEXTBOX:
		gfx.textrect("Q|I", &txrect);
		if(rect->height < txrect.height + OFFS * 2) {
			rect->height = txrect.height + OFFS * 2;
		}
		break;

	case RTK_SEP:
		if(w->any.par->win.layout == RTK_VBOX) {
			rect->width = w->any.par->any.width - PAD * 2;
			rect->height = PAD * 4 + BEVELSZ * 2;
		} else if(w->any.par->win.layout == RTK_HBOX) {
			rect->width = PAD * 4 + BEVELSZ * 2;
			rect->height = w->any.par->any.height - PAD * 2;
		} else {
			rect->width = rect->height = 0;
		}
		break;

	default:
		rect->width = rect->height = 0;
	}
}

static int need_relayout(rtk_widget *w)
{
	rtk_widget *c;

	if(w->any.flags & GEOMCHG) {
		return 1;
	}

	if(w->any.type == RTK_WIN) {
		c = w->win.clist;
		while(c) {
			if(need_relayout(c)) {
				return 1;
			}
			c = c->any.next;
		}
	}
	return 0;
}

static void calc_layout(rtk_widget *w)
{
	int x, y;
	rtk_widget *c;
	rtk_rect rect;

	if(w->any.type == RTK_WIN && w->win.layout != RTK_NONE) {
		x = y = PAD;

		c = w->win.clist;
		while(c) {
			rtk_move(c, x, y);
			calc_layout(c);

			if(w->win.layout == RTK_VBOX) {
				y += c->any.height + PAD * 2;
			} else {
				x += c->any.width + PAD * 2;
			}

			c = c->any.next;
		}
	}

	calc_widget_rect(w, &rect);
	w->any.width = rect.width;
	w->any.height = rect.height;

	w->any.flags &= ~GEOMCHG;
	rtk_invalidate(w);
}

void rtk_draw_widget(rtk_widget *w)
{
	int dirty;

	if(!(w->any.flags & VISIBLE)) {
		return;
	}

	if(need_relayout(w)) {
		calc_layout(w);
	}

	dirty = w->any.flags & DIRTY;
	if(!dirty && w->any.type != RTK_WIN) {
		return;
	}

	switch(w->any.type) {
	case RTK_WIN:
		draw_window(w);
		break;

	case RTK_BUTTON:
		draw_button(w);
		break;

	case RTK_CHECKBOX:
		draw_checkbox(w);
		break;

	case RTK_TEXTBOX:
		draw_textbox(w);
		break;

	case RTK_SLIDER:
		draw_slider(w);
		break;

	case RTK_SEP:
		draw_separator(w);
		break;

	default:
		break;
	}

	if(dirty) {
		rtk_validate(w);
		invalfb(w);
	}
}

static void widget_rect(rtk_widget *w, rtk_rect *rect)
{
	rect->x = w->any.x;
	rect->y = w->any.y;
	rect->width = w->any.width;
	rect->height = w->any.height;
}

static void abs_pos(rtk_widget *w, int *xpos, int *ypos)
{
	int x, y, px, py;

	x = w->any.x;
	y = w->any.y;

	if(w->any.par) {
		abs_pos(w->any.par, &px, &py);
		x += px;
		y += py;
	}

	*xpos = x;
	*ypos = y;
}

#define COL_BG		0xff666666
#define COL_BGHL	0xff808080
#define COL_LBEV	0xffaaaaaa
#define COL_SBEV	0xff222222
#define COL_TEXT	0xff000000
#define COL_WINFRM	0xff6688cc
#define COL_WINFRM_LIT	0xff88aaff
#define COL_WINFRM_SHAD	0xff224466
#define COL_TBOX	0xffeeccbb

static void hline(int x, int y, int sz, uint32_t col)
{
	rtk_rect rect;
	rect.x = x;
	rect.y = y;
	rect.width = sz;
	rect.height = 1;
	gfx.fill(&rect, col);
}

static void vline(int x, int y, int sz, uint32_t col)
{
	rtk_rect rect;
	rect.x = x;
	rect.y = y;
	rect.width = 1;
	rect.height = sz;
	gfx.fill(&rect, col);
}

enum {FRM_SOLID, FRM_OUTSET, FRM_INSET};

enum {UICOL_BG, UICOL_LBEV, UICOL_SBEV};
static uint32_t uicol[3];

static void uicolor(uint32_t col, uint32_t lcol, uint32_t scol)
{
	uicol[UICOL_BG] = col;
	uicol[UICOL_LBEV] = lcol;
	uicol[UICOL_SBEV] = scol;
}

static void draw_frame(rtk_rect *rect, int type)
{
	int tlcol, brcol;

	switch(type) {
	case FRM_SOLID:
		tlcol = brcol = 0xff000000;
		break;
	case FRM_OUTSET:
		tlcol = uicol[UICOL_LBEV];
		brcol = uicol[UICOL_SBEV];
		break;
	case FRM_INSET:
		tlcol = uicol[UICOL_SBEV];
		brcol = uicol[UICOL_LBEV];
		break;
	default:
		break;
	}

	hline(rect->x, rect->y, rect->width, tlcol);
	vline(rect->x, rect->y + 1, rect->height - 2, tlcol);
	hline(rect->x, rect->y + rect->height - 1, rect->width, brcol);
	vline(rect->x + rect->width - 1, rect->y + 1, rect->height - 2, brcol);
}

#define WINFRM_SZ	2
#define WINFRM_TBAR	16

static void draw_window(rtk_widget *w)
{
	rtk_rect rect, frmrect, tbrect;
	rtk_widget *c;
	int win_dirty = w->any.flags & DIRTY;

	if(win_dirty) {
		widget_rect(w, &rect);

		if(w->any.flags & FRAME) {
			uicolor(COL_WINFRM, COL_WINFRM_LIT, COL_WINFRM_SHAD);

			frmrect = rect;
			frmrect.width += WINFRM_SZ * 2;
			frmrect.height += WINFRM_SZ * 2 + WINFRM_TBAR;
			frmrect.x -= WINFRM_SZ;
			frmrect.y -= WINFRM_SZ + WINFRM_TBAR;

			tbrect.x = rect.x;
			tbrect.y = rect.y - WINFRM_TBAR;
			tbrect.width = rect.width;
			tbrect.height = WINFRM_TBAR;

			draw_frame(&frmrect, FRM_OUTSET);
			frmrect.x++;
			frmrect.y++;
			frmrect.width -= 2;
			frmrect.height -= 2;
			draw_frame(&frmrect, FRM_INSET);

			draw_frame(&tbrect, FRM_OUTSET);
			tbrect.x++;
			tbrect.y++;
			tbrect.width -= 2;
			tbrect.height -= 2;
			gfx.fill(&tbrect, COL_WINFRM);

			gfx.drawtext(tbrect.x, tbrect.y + tbrect.height - 1, w->any.text);
		}

		gfx.fill(&rect, COL_BG);
	}

	c = w->win.clist;
	while(c) {
		if(win_dirty) {
			rtk_invalidate(c);
		}
		rtk_draw_widget(c);
		c = c->any.next;
	}
}

static void draw_button(rtk_widget *w)
{
	int pressed;
	rtk_rect rect;

	widget_rect(w, &rect);
	abs_pos(w, &rect.x, &rect.y);

	if(w->bn.mode == RTK_TOGGLEBN) {
		pressed = w->any.value;
	} else {
		pressed = w->any.flags & PRESS;
	}

	uicolor(COL_BG, COL_LBEV, COL_SBEV);

	if(rect.width > 2 && rect.height > 2) {
		draw_frame(&rect, pressed ? FRM_INSET : FRM_OUTSET);

		rect.x++;
		rect.y++;
		rect.width -= 2;
		rect.height -= 2;
	}

	gfx.fill(&rect, w->any.flags & HOVER ? COL_BGHL : COL_BG);
	if(w->bn.icon) {
		int offs = w->any.flags & PRESS ? PAD + 1 : PAD;
		gfx.blit(rect.x + offs, rect.y + offs, w->bn.icon);
	} else {
		gfx.fill(&rect, 0xff802020);
	}
}

static void draw_checkbox(rtk_widget *w)
{
}

static void draw_textbox(rtk_widget *w)
{
	rtk_rect rect;

	widget_rect(w, &rect);
	abs_pos(w, &rect.x, &rect.y);

	uicolor(COL_TBOX, COL_LBEV, COL_SBEV);

	if(rect.width > 2 && rect.height > 2) {
		draw_frame(&rect, FRM_INSET);

		rect.x++;
		rect.y++;
		rect.width -= 2;
		rect.height -= 2;
	}

	gfx.fill(&rect, COL_TBOX);
	if(w->any.text) {
		gfx.drawtext(rect.x, rect.y + rect.height - PAD, w->any.text);
	}
}

static void draw_slider(rtk_widget *w)
{
}

static void draw_separator(rtk_widget *w)
{
	rtk_widget *win = w->any.par;
	rtk_rect rect;

	if(!win) return;

	uicolor(COL_BG, COL_LBEV, COL_SBEV);

	widget_rect(w, &rect);
	abs_pos(w, &rect.x, &rect.y);

	switch(win->win.layout) {
	case RTK_VBOX:
		rect.y += PAD * 2;
		rect.height = 2;
		break;

	case RTK_HBOX:
		rect.x += PAD * 2;
		rect.width = 2;
		break;

	default:
		break;
	}

	draw_frame(&rect, FRM_INSET);
}


static int hittest(rtk_widget *w, int x, int y)
{
	int x0 = w->any.x;
	int y0 = w->any.y;
	int x1 = w->any.x + w->any.width;
	int y1 = w->any.y + w->any.height;

	if(w->type == RTK_WIN && (w->any.flags & FRAME)) {
		x0 -= WINFRM_SZ;
		y0 -= WINFRM_SZ + WINFRM_TBAR;
		x1 += WINFRM_SZ;
		y1 += WINFRM_SZ;
	}

	if(x < x0 || y < y0) return 0;
	if(x >= x1 || y >= y1) return 0;
	return 1;
}

static void sethover(rtk_widget *w)
{
	if(hover == w) return;

	if(hover) {
		hover->any.flags &= ~HOVER;

		if(hover->type != RTK_WIN) {
			rtk_invalidate(hover);
			invalfb(hover);
		}
	}
	hover = w;
	if(w) {
		w->any.flags |= HOVER;

		if(w->type != RTK_WIN) {
			rtk_invalidate(w);
			invalfb(w);
		}
	}

	/*dbgmsg("hover \"%s\"\n", w ? (w->any.text ? w->any.text : "?") : "-");*/
}

static void setpress(rtk_widget *w)
{
	if(pressed == w) return;

	if(pressed) {
		pressed->any.flags &= ~PRESS;
		rtk_invalidate(pressed);
		invalfb(pressed);
	}
	pressed = w;
	if(w) {
		w->any.flags |= PRESS;
		rtk_invalidate(w);
		invalfb(w);
	}
}

static void click(rtk_widget *w, int x, int y)
{
	switch(w->type) {
	case RTK_BUTTON:
		if(w->bn.mode == RTK_TOGGLEBN) {
	case RTK_CHECKBOX:
			w->any.value ^= 1;
		}
		if(w->any.cbfunc) {
			w->any.cbfunc(w, w->any.cbcls);
		}
		rtk_invalidate(w);
		invalfb(w);
		break;

	default:
		break;
	}
}

int rtk_input_key(rtk_widget *w, int key, int press)
{
	return 0;
}

int rtk_input_mbutton(rtk_widget *w, int bn, int press, int x, int y)
{
	int res = 0;

	if(press) {
		if(hover && hittest(hover, x, y)) {
			setpress(hover);
			res = 1;
		}
		prev_mx = x;
		prev_my = y;
	} else {
		if(pressed && hittest(pressed, x, y)) {
			click(pressed, x, y);
			res = 1;
		}
		setpress(0);
	}

	return res;
}

int rtk_input_mmotion(rtk_widget *w, int x, int y)
{
	int i;
	rtk_widget *c;

	if(w == 0) {
		if(pressed) {
			/* dragging something */
			int dx = x - prev_mx;
			int dy = y - prev_my;
			prev_mx = x;
			prev_my = y;

			switch(pressed->type) {
			case RTK_WIN:
				if(pressed->any.flags & MOVABLE) {
					rtk_move(pressed, pressed->any.x + dx, pressed->any.y + dy);
				}
				break;

			default:
				break;
			}
			return 1;
		}

		for(i=0; i<num_win; i++) {
			w = winlist[i];

			if(rtk_input_mmotion(w, x, y)) {
				return 1;
			}
		}

		sethover(0);
		return 0;
	}

	if(w->type == RTK_WIN) {
		c = w->win.clist;
		while(c) {
			if(hittest(c, x, y)) {
				return rtk_input_mmotion(c, x, y);
			}
			c = c->any.next;
		}
	}

	if(hittest(w, x, y)) {
		sethover(w);
		return 1;
	}
	return 0;
}

void rtk_fix_rect(rtk_rect *rect)
{
	int x, y, w, h;

	x = rect->x;
	y = rect->y;

	if(rect->width < 0) {
		w = -rect->width;
		x += rect->width;
	} else {
		w = rect->width;
	}
	if(rect->height < 0) {
		h = -rect->height;
		y += rect->height;
	} else {
		h = rect->height;
	}

	rect->x = x;
	rect->y = y;
	rect->width = w;
	rect->height = h;
}

void rtk_rect_union(rtk_rect *a, const rtk_rect *b)
{
	int x0, y0, x1, y1;

	x0 = a->x;
	y0 = a->y;
	x1 = a->x + a->width;
	y1 = a->y + a->height;

	if(b->x < x0) x0 = b->x;
	if(b->y < y0) y0 = b->y;
	if(b->x + b->width > x1) x1 = b->x + b->width;
	if(b->y + b->height > y1) y1 = b->y + b->height;

	a->x = x0;
	a->y = y0;
	a->width = x1 - x0;
	a->height = y1 - y0;
}

static void invalfb(rtk_widget *w)
{
	rtk_rect rect;

	rect.x = w->any.x;
	rect.y = w->any.y;
	rect.width = w->any.width;
	rect.height = w->any.height;

	if(w->type == RTK_WIN && (w->any.flags & FRAME)) {
		rect.x -= WINFRM_SZ;
		rect.y -= WINFRM_SZ + WINFRM_TBAR;
		rect.width += WINFRM_SZ * 2;
		rect.height += WINFRM_SZ * 2 + WINFRM_TBAR;
	}

	app_redisplay(rect.x, rect.y, rect.width, rect.height);
}
