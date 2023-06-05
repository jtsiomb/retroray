#ifndef RTK_IMPL_H_
#define RTK_IMPL_H_

#include <assert.h>
#include "rtk.h"

typedef struct rtk_any {
	int type;
	int x, y, width, height;
	char *text;
	int visible, enabled;
	union rtk_widget *par, *next;
	rtk_callback cbfunc;
	void *cbcls;
} rtk_any, rtk_label;

typedef struct rtk_window {
	rtk_any any;
	union rtk_widget *clist, *ctail;
	int layout;
} rtk_window;

typedef struct rtk_button {
	rtk_any any;
	struct image *icon;
} rtk_button;

typedef struct rtk_checkbox {
	rtk_any any;
	int chk;
} rtk_checkbox;

typedef union rtk_widget {
	int type;
	rtk_any any;
	rtk_window win;
	rtk_button bn;
	rtk_label lb;
	rtk_checkbox chk;
} rtk_widget;


#define RTK_ASSERT_TYPE(w, t)	assert(w->any.type == t)


#endif	/* RTK_IMPL_H_ */
