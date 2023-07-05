#ifndef RTK_IMPL_H_
#define RTK_IMPL_H_

#include <assert.h>
#include "sizeint.h"
#include "rtk.h"

enum {
	VISIBLE		= 0x001,
	ENABLED		= 0x002,
	HOVER		= 0x010,
	PRESS		= 0x020,
	GEOMCHG		= 0x100,
	DIRTY		= 0x200,

	/* window flags */
	FRAME		= RTK_WIN_FRAME << 16,
	MOVABLE		= RTK_WIN_MOVABLE << 16,
	RESIZABLE	= RTK_WIN_RESIZABLE << 16
};

typedef struct rtk_any {
	int type;
	int x, y, width, height;
	char *text;
	int value;
	unsigned int flags;
	union rtk_widget *par, *next;
	rtk_callback cbfunc;
	void *cbcls;
} rtk_any;

typedef struct rtk_window {
	rtk_any any;
	union rtk_widget *clist, *ctail;
	int layout;
} rtk_window;

typedef struct rtk_button {
	rtk_any any;
	int mode;
	rtk_icon *icon;
} rtk_button;

typedef struct rtk_textbox {
	rtk_any any;
	int cursor;
} rtk_textbox;

typedef struct rtk_slider {
	rtk_any any;
	int vmin, vmax;
} rtk_slider;

typedef union rtk_widget {
	int type;
	rtk_any any;
	rtk_window win;
	rtk_button bn;
	rtk_textbox tbox;
	rtk_slider slider;
} rtk_widget;

typedef struct rtk_iconsheet {
	int width, height;
	uint32_t *pixels;

	struct rtk_icon *icons;
} rtk_iconsheet;

#define RTK_ASSERT_TYPE(w, t)	assert(w->any.type == t)


#endif	/* RTK_IMPL_H_ */
