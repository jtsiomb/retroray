#include <stdlib.h>
#include <string.h>
#include "imago2.h"
#include "rtk_impl.h"

static rtk_draw_ops gfx;

static void calc_widget_rect(rtk_widget *w, rtk_rect *rect);
static void draw_window(rtk_widget *w);
static void draw_button(rtk_widget *w);
static void draw_checkbox(rtk_widget *w);
static void draw_separator(rtk_widget *w);

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
	if(!w) return;

	if(w->type == RTK_WIN) {
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
	w->any.x = x;
	w->any.y = y;
	w->any.flags |= GEOMCHG;
}

void rtk_pos(rtk_widget *w, int *xptr, int *yptr)
{
	*xptr = w->any.x;
	*yptr = w->any.y;
}

void rtk_resize(rtk_widget *w, int xsz, int ysz)
{
	w->any.width = xsz;
	w->any.height = ysz;
	w->any.flags |= GEOMCHG;
}

void rtk_size(rtk_widget *w, int *xptr, int *yptr)
{
	*xptr = w->any.width;
	*yptr = w->any.height;
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
	return 0;
}

const char *rtk_get_text(rtk_widget *w)
{
	return w->any.text;
}

void rtk_set_value(rtk_widget *w, int val)
{
	w->any.value = val;
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

void rtk_bn_set_icon(rtk_widget *w, rtk_icon *icon)
{
	rtk_rect rect;

	RTK_ASSERT_TYPE(w, RTK_BUTTON);
	w->bn.icon = icon;

	calc_widget_rect(w, &rect);
	rtk_resize(w, rect.width, rect.height);
}

rtk_icon *rtk_bn_get_icon(rtk_widget *w)
{
	RTK_ASSERT_TYPE(w, RTK_BUTTON);
	return w->bn.icon;
}

/* --- constructors --- */

rtk_widget *rtk_create_window(rtk_widget *par, const char *title, int x, int y, int width, int height)
{
	rtk_widget *w;

	if(!(w = rtk_create_widget())) {
		return 0;
	}
	w->type = RTK_WIN;
	if(par) rtk_win_add(par, w);
	rtk_set_text(w, title);
	rtk_move(w, x, y);
	rtk_resize(w, width, height);
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

	case RTK_SEP:
		if(w->any.par->win.layout == RTK_VBOX) {
			rect->width = w->any.par->any.width - PAD * 2;
			rect->height = (PAD + BEVELSZ) * 2;
		} else if(w->any.par->win.layout == RTK_HBOX) {
			rect->width = (PAD + BEVELSZ) * 2;
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

	if(w->any.type == RTK_WIN && w->win.layout != RTK_NONE) {
		x = y = PAD;

		c = w->win.clist;
		while(c) {
			rtk_move(c, x, y);
			calc_layout(c);

			if(w->win.layout == RTK_VBOX) {
				y += c->any.height + PAD;
			} else {
				x += c->any.width + PAD;
			}

			c = c->any.next;
		}
	}

	w->any.flags = (w->any.flags & ~GEOMCHG) | DIRTY;
}

void rtk_draw_widget(rtk_widget *w)
{
	if(need_relayout(w)) {
		calc_layout(w);
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

	case RTK_SEP:
		draw_separator(w);
		break;

	default:
		break;
	}

	w->any.flags &= ~DIRTY;
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
#define COL_LBEV	0xffaaaaaa
#define COL_SBEV	0xff333333
#define COL_TEXT	0xff000000

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

static void draw_frame(rtk_rect *rect, int type)
{
	int tlcol, brcol;

	switch(type) {
	case FRM_SOLID:
		tlcol = brcol = 0xff000000;
		break;
	case FRM_OUTSET:
		tlcol = COL_LBEV;
		brcol = COL_SBEV;
		break;
	case FRM_INSET:
		tlcol = COL_SBEV;
		brcol = COL_LBEV;
		break;
	default:
		break;
	}

	hline(rect->x, rect->y, rect->width, tlcol);
	vline(rect->x, rect->y + 1, rect->height - 2, tlcol);
	hline(rect->x, rect->y + rect->height - 1, rect->width, brcol);
	vline(rect->x + rect->width - 1, rect->y + 1, rect->height - 2, brcol);
}

static void draw_window(rtk_widget *w)
{
	rtk_rect rect;
	rtk_widget *c;

	widget_rect(w, &rect);
	gfx.fill(&rect, COL_BG);

	c = w->win.clist;
	while(c) {
		rtk_draw_widget(c);
		c = c->any.next;
	}
}

static void draw_button(rtk_widget *w)
{
	rtk_rect rect;

	widget_rect(w, &rect);
	abs_pos(w, &rect.x, &rect.y);

	if(rect.width > 2 && rect.height > 2) {
		draw_frame(&rect, FRM_OUTSET);

		rect.x++;
		rect.y++;
		rect.width -= 2;
		rect.height -= 2;
	}

	gfx.fill(&rect, COL_BG);
	if(w->bn.icon) {
		gfx.blit(rect.x + OFFS, rect.y + OFFS, w->bn.icon);
	} else {
		gfx.fill(&rect, 0xff802020);
	}
}

static void draw_checkbox(rtk_widget *w)
{
}

static void draw_separator(rtk_widget *w)
{
	rtk_widget *win = w->any.par;
	rtk_rect rect;

	widget_rect(w, &rect);
	abs_pos(w, &rect.x, &rect.y);

	draw_frame(&rect, FRM_INSET);
}
