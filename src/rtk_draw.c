#include "app.h"
#include "rtk.h"
#include "rtk_impl.h"
#include "util.h"

rtk_draw_ops rtk_gfx;
#define gfx rtk_gfx

static int fontheight;

enum {
	FRM_SOLID,
	FRM_OUTSET,
	FRM_INSET,
	FRM_FILLBG = 0x100
};

static void widget_rect(rtk_widget *w, rtk_rect *rect);
static void abs_widget_rect(rtk_widget *w, rtk_rect *rect);
static void uicolor(uint32_t col, uint32_t lcol, uint32_t scol);
static void draw_frame(rtk_rect *rect, int type, int sz);

static void draw_window(rtk_widget *w);
static void draw_label(rtk_widget *w);
static void draw_button(rtk_widget *w);
static void draw_checkbox(rtk_widget *w);
static void draw_textbox(rtk_widget *w);
static void draw_slider(rtk_widget *w);
static void draw_separator(rtk_widget *w);
static void draw_drawbox(rtk_widget *w);


#define BEVELSZ		1
#define PAD			2
#define OFFS		(BEVELSZ + PAD)
#define CHKBOXSZ	(BEVELSZ * 2 + 8)

#define WINFRM_SZ	2
#define WINFRM_TBAR	16

void rtk_init_drawing(void)
{
	const char *s = "QI|9g/";
	rtk_rect r;

	gfx.textrect(s, &r);
	fontheight = r.height;
}


void rtk_calc_widget_rect(rtk_widget *w, rtk_rect *rect)
{
	rtk_widget *child;
	rtk_button *bn;
	rtk_rect txrect = {0};

	rect->x = w->x;
	rect->y = w->y;
	rect->width = w->width;
	rect->height = w->height;

	rtk_abs_pos(w, &w->absx, &w->absy);

	if((w->flags & (AUTOWIDTH | AUTOHEIGHT)) == 0) {
		return;
	}

	if(w->text) {
		gfx.textrect(w->text, &txrect);
	}

	switch(w->type) {
	case RTK_WIN:
		if((w->flags & (AUTOWIDTH | AUTOHEIGHT))) {
			rtk_rect subrect;
			rtk_window *win = (rtk_window*)w;
			rtk_null_rect(&subrect);
			child = win->clist;
			while(child) {
				rtk_rect crect;
				widget_rect(child, &crect);
				rtk_rect_union(&subrect, &crect);
				child = child->next;
			}
			if(w->flags & AUTOWIDTH) {
				rect->width = subrect.width + PAD * 2;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = subrect.height + PAD * 2;
			}
		}
		break;

	case RTK_DRAWBOX:
		if(w->flags & AUTOWIDTH) {
			rect->width = w->width;
		}
		if(w->flags & AUTOHEIGHT) {
			rect->height = w->height;
		}
		break;

	case RTK_BUTTON:
		bn = (rtk_button*)w;
		if(bn->icon) {
			if(w->flags & AUTOWIDTH) {
				rect->width = bn->icon->width + OFFS * 2;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = bn->icon->height + OFFS * 2;
			}
		} else {
			if(w->flags & AUTOWIDTH) {
				rect->width = txrect.width + OFFS * 2;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = txrect.height + OFFS * 2;
			}
		}
		break;

	case RTK_CHECKBOX:
		if(w->flags & AUTOWIDTH) {
			rect->width = txrect.width + CHKBOXSZ + OFFS * 2 + PAD;
		}
		if(w->flags & AUTOHEIGHT) {
			rect->height = txrect.height + OFFS * 2;
		}
		break;

	case RTK_LABEL:
		if(w->flags & AUTOWIDTH) {
			rect->width = txrect.width + PAD * 2;
		}
		if(w->flags & AUTOHEIGHT) {
			rect->height = txrect.height + PAD * 2;
		}
		break;

	case RTK_TEXTBOX:
		if(w->flags & AUTOHEIGHT) {
			if(rect->height < fontheight + OFFS * 2) {
				rect->height = fontheight + OFFS * 2;
			}
		}
		break;

	case RTK_SEP:
		if(w->par->layout == RTK_VBOX) {
			if(w->flags & AUTOWIDTH) {
				rect->width = w->par->width - PAD * 2;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = PAD * 4 + BEVELSZ * 2;
			}
		} else if(w->par->layout == RTK_HBOX) {
			if(w->flags & AUTOWIDTH) {
				rect->width = PAD * 4 + BEVELSZ * 2;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = w->par->height - PAD * 2;
			}
		} else {
			if(w->flags & AUTOWIDTH) {
				rect->width = 0;
			}
			if(w->flags & AUTOHEIGHT) {
				rect->height = 0;
			}
		}
		break;

	default:
		if(w->flags & AUTOWIDTH) {
			rect->width = 0;
		}
		if(w->flags & AUTOHEIGHT) {
			rect->height = 0;
		}
	}
}

void rtk_abs_pos(rtk_widget *w, int *xpos, int *ypos)
{
	int x, y, px, py;

	x = w->x;
	y = w->y;

	if(w->par) {
		rtk_abs_pos((rtk_widget*)w->par, &px, &py);
		x += px;
		y += py;
	}

	*xpos = x;
	*ypos = y;
}


int rtk_hittest(rtk_widget *w, int x, int y)
{
	int x0, y0, x1, y1;

	x0 = w->absx;
	y0 = w->absy;
	x1 = x0 + w->width;
	y1 = y0 + w->height;

	if(w->type == RTK_WIN && (w->flags & FRAME)) {
		x0 -= WINFRM_SZ;
		y0 -= WINFRM_SZ + WINFRM_TBAR;
		x1 += WINFRM_SZ;
		y1 += WINFRM_SZ;
	}

	if(x < x0 || y < y0) return 0;
	if(x >= x1 || y >= y1) return 0;
	return 1;
}


void rtk_invalfb(rtk_widget *w)
{
	rtk_rect rect;

	rect.x = w->x;
	rect.y = w->y;
	rect.width = w->width;
	rect.height = w->height;

	if(w->type == RTK_WIN && (w->flags & FRAME)) {
		rect.x -= WINFRM_SZ;
		rect.y -= WINFRM_SZ + WINFRM_TBAR;
		rect.width += WINFRM_SZ * 2;
		rect.height += WINFRM_SZ * 2 + WINFRM_TBAR;
	}

	app_redisplay(rect.x, rect.y, rect.width, rect.height);
}

static int need_relayout(rtk_widget *w)
{
	rtk_widget *c;

	if(w->flags & GEOMCHG) {
		return 1;
	}

	if(w->type == RTK_WIN) {
		c = ((rtk_window*)w)->clist;
		while(c) {
			if(need_relayout(c)) {
				return 1;
			}
			c = c->next;
		}
	}
	return 0;
}

static void calc_layout(rtk_widget *w)
{
	int x, y, dx, dy;
	rtk_widget *c;
	rtk_window *win = (rtk_window*)w;
	rtk_rect rect;

	if(w->type == RTK_WIN && win->layout != RTK_NONE) {
		x = y = PAD;

		c = win->clist;
		while(c) {
			rtk_move(c, x, y);
			calc_layout(c);

			if(win->layout == RTK_VBOX) {
				y += c->height + PAD * 2;
			} else {
				x += c->width + PAD * 2;
			}

			c = c->next;
		}
	}

	rtk_calc_widget_rect(w, &rect);
	dx = rect.width - w->width;
	dy = rect.height - w->height;
	w->width = rect.width;
	w->height = rect.height;

	if((dx | dy) == 0) {
		w->flags &= ~GEOMCHG;
	}
	rtk_invalidate(w);
}

static int calc_substr_width(const char *str, int start, int end)
{
	rtk_rect rect;
	int len;
	char *buf;

	if(end <= start) {
		return 0;
	}
	if(end <= 0) {
		end = strlen(str);
	}

	len = end - start;
	buf = alloca(len + 1);

	memcpy(buf, str + start, len);
	buf[len] = 0;

	gfx.textrect(buf, &rect);
	return rect.width;
}

void rtk_draw_widget(rtk_widget *w)
{
	int dirty;

	if(!(w->flags & VISIBLE)) {
		return;
	}

	if(need_relayout(w)) {
		calc_layout(w);
	}

	dirty = w->flags & DIRTY;
	if(!dirty && w->type != RTK_WIN) {
		return;
	}

	switch(w->type) {
	case RTK_WIN:
		draw_window(w);
		break;

	case RTK_LABEL:
		draw_label(w);
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

	case RTK_DRAWBOX:
		draw_drawbox(w);
		break;

	default:
		break;
	}

	if(w->flags & DBGRECT) {
		rtk_rect r;
		abs_widget_rect(w, &r);
		uicolor(0xffff0000, 0xffff0000, 0xffff0000);
		draw_frame(&r, FRM_SOLID, 1);
	}

	if(dirty) {
		rtk_validate(w);
		rtk_invalfb(w);
	}
}

static void widget_rect(rtk_widget *w, rtk_rect *rect)
{
	rect->x = w->x;
	rect->y = w->y;
	rect->width = w->width;
	rect->height = w->height;
}

static void abs_widget_rect(rtk_widget *w, rtk_rect *rect)
{
	rect->x = w->absx;
	rect->y = w->absy;
	rect->width = w->width;
	rect->height = w->height;
}

#define COL_BG					0xff666666
#define COL_BGHL				0xff808080
#define COL_LBEV				0xffaaaaaa
#define COL_SBEV				0xff222222
#define COL_TEXT				0xff000000
#define COL_WINFRM_FOCUS		0xff6688cc
#define COL_WINFRM_LIT_FOCUS	0xff88aaff
#define COL_WINFRM_SHAD_FOCUS	0xff224466
#define COL_WINFRM				0xff667788
#define COL_WINFRM_LIT			0xff8899aa
#define COL_WINFRM_SHAD			0xff224455
#define COL_TBOX				0xffeeccbb

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

enum {UICOL_BG, UICOL_LBEV, UICOL_SBEV};
static uint32_t uicol[3];

static void uicolor(uint32_t col, uint32_t lcol, uint32_t scol)
{
	uicol[UICOL_BG] = col;
	uicol[UICOL_LBEV] = lcol;
	uicol[UICOL_SBEV] = scol;
}

static void draw_frame(rtk_rect *rect, int type, int sz)
{
	int i, tlcol, brcol, fillbg;
	rtk_rect r = *rect;

	fillbg = type & FRM_FILLBG;
	type &= ~FRM_FILLBG;

	switch(type) {
	case FRM_OUTSET:
		tlcol = uicol[UICOL_LBEV];
		brcol = uicol[UICOL_SBEV];
		break;
	case FRM_INSET:
		tlcol = uicol[UICOL_SBEV];
		brcol = uicol[UICOL_LBEV];
		break;
	case FRM_SOLID:
	default:
		tlcol = brcol = uicol[UICOL_BG];
	}

	for(i=0; i<sz; i++) {
		if(r.width < 2 || r.height < 2) break;

		hline(r.x, r.y, r.width, tlcol);
		vline(r.x, r.y + 1, r.height - 2, tlcol);
		hline(r.x, r.y + r.height - 1, r.width, brcol);
		vline(r.x + r.width - 1, r.y + 1, r.height - 2, brcol);

		r.x++;
		r.y++;
		r.width -= 2;
		r.height -= 2;
	}

	if(fillbg) {
		gfx.fill(&r, uicol[UICOL_BG]);
	}
}

static void draw_window(rtk_widget *w)
{
	rtk_rect rect, frmrect, tbrect;
	rtk_widget *c;
	rtk_window *win = (rtk_window*)w;
	int win_dirty = w->flags & DIRTY;

	if(win_dirty) {
		abs_widget_rect(w, &rect);

		if(w->flags & FRAME) {
			if(w->flags & FOCUS) {
				uicolor(COL_WINFRM_FOCUS, COL_WINFRM_LIT_FOCUS, COL_WINFRM_SHAD_FOCUS);
			} else {
				uicolor(COL_WINFRM, COL_WINFRM_LIT, COL_WINFRM_SHAD);
			}

			frmrect = rect;
			frmrect.width += WINFRM_SZ * 2;
			frmrect.height += WINFRM_SZ * 2 + WINFRM_TBAR;
			frmrect.x -= WINFRM_SZ;
			frmrect.y -= WINFRM_SZ + WINFRM_TBAR;

			tbrect.x = rect.x;
			tbrect.y = rect.y - WINFRM_TBAR;
			tbrect.width = rect.width;
			tbrect.height = WINFRM_TBAR;

			draw_frame(&frmrect, FRM_OUTSET, 1);
			frmrect.x++;
			frmrect.y++;
			frmrect.width -= 2;
			frmrect.height -= 2;
			draw_frame(&frmrect, FRM_INSET, 1);

			draw_frame(&tbrect, FRM_OUTSET | FRM_FILLBG, 1);
			tbrect.x++;
			tbrect.y++;
			tbrect.width -= 2;
			tbrect.height -= 2;

			gfx.drawtext(tbrect.x, tbrect.y + tbrect.height - 1, w->text);
		}

		gfx.fill(&rect, COL_BG);
	}

	c = win->clist;
	while(c) {
		if(win_dirty) {
			rtk_invalidate(c);
		}
		rtk_draw_widget(c);
		c = c->next;
	}
}

static void draw_label(rtk_widget *w)
{
	rtk_rect rect;

	abs_widget_rect(w, &rect);

	gfx.drawtext(rect.x + PAD, rect.y + rect.height - PAD, w->text);
}

static void draw_button(rtk_widget *w)
{
	int pressed;
	rtk_rect rect;
	rtk_button *bn = (rtk_button*)w;

	abs_widget_rect(w, &rect);

	if(bn->mode == RTK_TOGGLEBN) {
		pressed = w->value;
	} else {
		pressed = w->flags & PRESS;
	}

	uicolor(w->flags & HOVER ? COL_BGHL : COL_BG, COL_LBEV, COL_SBEV);

	draw_frame(&rect, (pressed ? FRM_INSET : FRM_OUTSET) | FRM_FILLBG, 1);
	rect.x++;
	rect.y++;
	rect.width -= 2;
	rect.height -= 2;

	if(bn->icon) {
		int offs = w->flags & PRESS ? PAD + 1 : PAD;
		gfx.blit(rect.x + offs, rect.y + offs, bn->icon);
	} else if(w->text) {
		gfx.drawtext(rect.x + PAD, rect.y + rect.height - PAD, w->text);
	}
}

static void draw_checkbox(rtk_widget *w)
{
}

static void draw_textbox(rtk_widget *w)
{
	rtk_rect rect;
	rtk_textbox *tb = (rtk_textbox*)w;
	int curx = 0;

	abs_widget_rect(w, &rect);

	uicolor(COL_TBOX, COL_LBEV, COL_SBEV);

	draw_frame(&rect, FRM_INSET | FRM_FILLBG, w->flags & FOCUS ? 2 : 1);

	rect.x++;
	rect.y++;
	rect.width -= 2;
	rect.height -= 2;

	if(w->text) {
		gfx.drawtext(rect.x + PAD, rect.y + rect.height - PAD, w->text);

		if(w->flags & FOCUS) {
			curx = calc_substr_width(w->text, tb->scroll, tb->cursor);
		}
	}

	/* cursor */
	if(w->flags & FOCUS) {
		int x = rect.x + PAD + curx - 1;
		int y = rect.y + rect.height - PAD - fontheight;
		vline(x, y, fontheight, 0xff000000);
	}

	rtk_invalfb(w);
}

static void draw_slider(rtk_widget *w)
{
}

static void draw_separator(rtk_widget *w)
{
	rtk_window *win = (rtk_window*)w->par;
	rtk_rect rect;

	if(!win) return;

	uicolor(COL_BG, COL_LBEV, COL_SBEV);

	abs_widget_rect(w, &rect);

	switch(win->layout) {
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

	draw_frame(&rect, FRM_INSET, 1);
}

static void draw_drawbox(rtk_widget *w)
{
	if(!w->cbfunc) {
		rtk_rect r;
		abs_widget_rect(w, &r);
		gfx.fill(&r, 0xff000000);
		return;
	}

	w->cbfunc(w, w->cbcls);
}
