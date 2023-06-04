#ifndef RTK_H_
#define RTK_H_

enum { RTK_ANY, RTK_WIN, RTK_BUTTON, RTK_LABEL, RTK_CHECKBOX, RTK_SLIDER };

typedef struct rtk_any {
	int type;
	int x, y, width, height;
	char *text;
} rtk_any, rtk_label;

typedef union rtk_widget {
	int type;
	rtk_any any;
	rtk_window win;
	rtk_button bn;
	rtk_label lb;
	rtk_checkbox chk;
} rtk_widget;

rtk_widget *rtk_create_window(rtk_widget *par, int x, int y, int w, int h);

#endif	/* RTK_H_ */
