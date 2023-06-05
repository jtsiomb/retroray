#include <stdlib.h>
#include <string.h>
#include "rtk_impl.h"

rtk_widget *rtk_create_widget(void)
{
	rtk_widget *w;

	if(!(w = calloc(1, sizeof *w))) {
		return 0;
	}
	w->any.visible = w->any.enabled = 1;
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
}

void rtk_size(rtk_widget *w, int *xptr, int *yptr)
{
	*xptr = w->any.width;
	*yptr = w->any.height;
}

int rtk_set_text(rtk_widget *w, const char *str)
{
	char *s = strdup(str);
	if(!s) return -1;

	free(w->any.text);
	w->any.text = s;
	return 0;
}

const char *rtk_get_text(rtk_widget *w)
{
	return w->any.text;
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

rtk_widget *rtk_create_window(rtk_widget *par, const char *title, int x, int y, int w, int h)
{
	return 0;
}

rtk_widget *rtk_create_button(rtk_widget *par, const char *str, rtk_callback cbfunc)
{
	return 0;
}

rtk_widget *rtk_create_iconbutton(rtk_widget *par, struct image *img, rtk_callback cbfunc)
{
	return 0;
}

rtk_widget *rtk_create_label(rtk_widget *par, const char *text)
{
	return 0;
}

rtk_widget *rtk_create_checkbox(rtk_widget *par, const char *text, int chk, rtk_callback cbfunc)
{
	return 0;
}
