include ../../config.mk

obj = font.o draw.o drawgl.o drawrast.o utf8.o
alib = ../unix/drawtext.a

CFLAGS = $(CFLAGS_extra) $(opt) $(dbg) -DNO_FREETYPE

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
