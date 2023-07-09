#include "app.h"
#include "rtk.h"
#include "rtk_impl.h"

rtk_draw_ops rtk_gfx;
#define gfx rtk_gfx

static void draw_window(rtk_widget *w);
static void draw_button(rtk_widget *w);
static void draw_checkbox(rtk_widget *w);
static void draw_textbox(rtk_widget *w);
static void draw_slider(rtk_widget *w);
static void draw_separator(rtk_widget *w);


#define BEVELSZ		1
#define PAD			2
#define OFFS		(BEVELSZ + PAD)
#define CHKBOXSZ	(BEVELSZ * 2 + 8)

#define WINFRM_SZ	2
#define WINFRM_TBAR	16


void rtk_calc_widget_rect(rtk_widget *w, rtk_rect *rect)
{
	rtk_button *bn;
	rtk_rect txrect = {0};

	rect->x = w->x;
	rect->y = w->y;

	if(w->text) {
		gfx.textrect(w->text, &txrect);
	}

	switch(w->type) {
	case RTK_WIN:
		rect->width = w->width;
		rect->height = w->height;
		break;

	case RTK_BUTTON:
		bn = (rtk_button*)w;
		if(bn->icon) {
			rect->width = bn->icon->width + OFFS * 2;
			rect->height = bn->icon->height + OFFS * 2;
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
		if(w->par->layout == RTK_VBOX) {
			rect->width = w->par->width - PAD * 2;
			rect->height = PAD * 4 + BEVELSZ * 2;
		} else if(w->par->layout == RTK_HBOX) {
			rect->width = PAD * 4 + BEVELSZ * 2;
			rect->height = w->par->height - PAD * 2;
		} else {
			rect->width = rect->height = 0;
		}
		break;

	default:
		rect->width = rect->height = 0;
	}
}

int rtk_hittest(rtk_widget *w, int x, int y)
{
	int x0 = w->x;
	int y0 = w->y;
	int x1 = w->x + w->width;
	int y1 = w->y + w->height;

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
	int x, y;
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
	w->width = rect.width;
	w->height = rect.height;

	w->flags &= ~GEOMCHG;
	rtk_invalidate(w);
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

static void abs_pos(rtk_widget *w, int *xpos, int *ypos)
{
	int x, y, px, py;

	x = w->x;
	y = w->y;

	if(w->par) {
		abs_pos((rtk_widget*)w->par, &px, &py);
		x += px;
		y += py;
	}

	*xpos = x;
	*ypos = y;
}

#define COL_BG			0xff666666
#define COL_BGHL		0xff808080
#define COL_LBEV		0xffaaaaaa
#define COL_SBEV		0xff222222
#define COL_TEXT		0xff000000
#define COL_WINFRM		0xff6688cc
#define COL_WINFRM_LIT	0xff88aaff
#define COL_WINFRM_SHAD	0xff224466
#define COL_TBOX		0xffeeccbb

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
		tlcol = brcol = 0xff000000;
		break;
		break;
	}

	hline(rect->x, rect->y, rect->width, tlcol);
	vline(rect->x, rect->y + 1, rect->height - 2, tlcol);
	hline(rect->x, rect->y + rect->height - 1, rect->width, brcol);
	vline(rect->x + rect->width - 1, rect->y + 1, rect->height - 2, brcol);
}

static void draw_window(rtk_widget *w)
{
	rtk_rect rect, frmrect, tbrect;
	rtk_widget *c;
	rtk_window *win = (rtk_window*)w;
	int win_dirty = w->flags & DIRTY;

	if(win_dirty) {
		widget_rect(w, &rect);

		if(w->flags & FRAME) {
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

static void draw_button(rtk_widget *w)
{
	int pressed;
	rtk_rect rect;
	rtk_button *bn = (rtk_button*)w;

	widget_rect(w, &rect);
	abs_pos(w, &rect.x, &rect.y);

	if(bn->mode == RTK_TOGGLEBN) {
		pressed = w->value;
	} else {
		pressed = w->flags & PRESS;
	}

	uicolor(COL_BG, COL_LBEV, COL_SBEV);

	if(rect.width > 2 && rect.height > 2) {
		draw_frame(&rect, pressed ? FRM_INSET : FRM_OUTSET);

		rect.x++;
		rect.y++;
		rect.width -= 2;
		rect.height -= 2;
	}

	gfx.fill(&rect, w->flags & HOVER ? COL_BGHL : COL_BG);
	if(bn->icon) {
		int offs = w->flags & PRESS ? PAD + 1 : PAD;
		gfx.blit(rect.x + offs, rect.y + offs, bn->icon);
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
	if(w->text) {
		gfx.drawtext(rect.x, rect.y + rect.height - PAD, w->text);
	}
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

	widget_rect(w, &rect);
	abs_pos(w, &rect.x, &rect.y);

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

	draw_frame(&rect, FRM_INSET);
}

