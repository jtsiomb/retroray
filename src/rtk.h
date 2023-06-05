#ifndef RTK_H_
#define RTK_H_

struct image;

/* widget type */
enum { RTK_ANY, RTK_WIN, RTK_BUTTON, RTK_LABEL, RTK_CHECKBOX, RTK_SLIDER };
/* window layout */
enum { RTK_NONE, RTK_VBOX, RTK_HBOX };

typedef union rtk_widget rtk_widget;

typedef void (*rtk_callback)(rtk_widget*, void*);

rtk_widget *rtk_create_widget(void);
void rtk_free_widget(rtk_widget *w);

int rtk_type(rtk_widget *w);

void rtk_move(rtk_widget *w, int x, int y);
void rtk_pos(rtk_widget *w, int *xptr, int *yptr);
void rtk_resize(rtk_widget *w, int xsz, int ysz);
void rtk_size(rtk_widget *w, int *xptr, int *yptr);

int rtk_set_text(rtk_widget *w, const char *str);
const char *rtk_get_text(rtk_widget *w);

void rtk_set_callback(rtk_widget *w, rtk_callback cbfunc, void *cls);

/* window functions */
void rtk_win_layout(rtk_widget *w, int layout);
void rtk_win_clear(rtk_widget *w);
void rtk_win_add(rtk_widget *par, rtk_widget *child);
void rtk_win_rm(rtk_widget *par, rtk_widget *child);
int rtk_win_has(rtk_widget *par, rtk_widget *child);

rtk_widget *rtk_create_window(rtk_widget *par, const char *title, int x, int y, int w, int h);
rtk_widget *rtk_create_button(rtk_widget *par, const char *str, rtk_callback cbfunc);
rtk_widget *rtk_create_iconbutton(rtk_widget *par, struct image *img, rtk_callback cbfunc);
rtk_widget *rtk_create_label(rtk_widget *par, const char *text);
rtk_widget *rtk_create_checkbox(rtk_widget *par, const char *text, int chk, rtk_callback cbfunc);

#endif	/* RTK_H_ */
