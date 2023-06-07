#ifndef RTK_IMPL_H_
#define RTK_IMPL_H_

#include <assert.h>
#include "inttypes.h"
#include "rtk.h"

enum {
	VISIBLE		= 0x001,
	ENABLED		= 0x002,
	GEOMCHG		= 0x100,
	DIRTY		= 0x200
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
	rtk_icon *icon;
} rtk_button;

typedef union rtk_widget {
	int type;
	rtk_any any;
	rtk_window win;
	rtk_button bn;
} rtk_widget;

typedef struct rtk_iconsheet {
	int width, height;
	uint32_t *pixels;

	struct rtk_icon *icons;
} rtk_iconsheet;

#define RTK_ASSERT_TYPE(w, t)	assert(w->any.type == t)


#endif	/* RTK_IMPL_H_ */
