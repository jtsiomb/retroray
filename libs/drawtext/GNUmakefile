obj = font.o draw.o drawgl.o drawrast.o utf8.o
alib = ../unix/drawtext.a

include ../../config.mk

def_sw = -DNO_OPENGL
CFLAGS = -g -DNO_FREETYPE $(def_$(build_gfx))

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
