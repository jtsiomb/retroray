#include "gfxutil.h"
#include "util.h"


static INLINE int min3(int a, int b, int c)
{
	if(a < b) {
		return a < c ? a : c;
	}
	return b < c ? b : c;
}
static INLINE int max3(int a, int b, int c)
{
	if(a > b) {
		return a > c ? a : c;
	}
	return b > c ? b : c;
}

void rgb_to_hsv(int r, int g, int b, int *hptr, int *sptr, int *vptr)
{
	int h, s, v, xmax, xmin, c;

	xmax = max3(r, g, b);
	xmin = min3(r, g, b);
	c = xmax - xmin;

	v = xmax;
	if(c == 0) {
		h = 0;
	} else if(v == r) {
		h = (60 * (((g - b) << 8) / c + (6 << 8))) >> 8;
		h %= 360;
	} else if(v == g) {
		h = (60 * (((b - r) << 8) / c + (2 << 8))) >> 8;
	} else {	/* v == b */
		h = (60 * (((r - g) << 8) / c + (4 << 8))) >> 8;
	}

	s = v == 0 ? 0 : (c << 8) / v;

	*hptr = h;
	*sptr = s;
	*vptr = v;
}

void hsv_to_rgb(int h, int s, int v, int *rptr, int *gptr, int *bptr)
{
	int r, g, b, c, x, hp, m, frac;

	c = (v * s) >> 8;
	hp = h * 256 / 60;
	frac = hp & 0x1ff;
	x = (c * (256 - abs(frac - 256))) >> 8;

	switch(hp >> 8) {
	case 0:
		r = c; g = x; b = 0;
		break;
	case 1:
		r = x; g = c; b = 0;
		break;
	case 2:
		r = 0; g = c; b = x;
		break;
	case 3:
		r = 0; g = x; b = c;
		break;
	case 4:
		r = x; g = 0; b = c;
		break;
	case 5:
	default:
		r = c; g = 0; b = x;
	}

	m = v - c;
	r += m;
	g += m;
	b += m;

	*rptr = r > 255 ? 255 : r;
	*gptr = g > 255 ? 255 : g;
	*bptr = b > 255 ? 255 : b;
}
