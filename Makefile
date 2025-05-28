include config.mk

src = src/app.c src/cmesh.c src/cpuid.c src/darray.c src/font.c src/geom.c \
	  src/logger.c src/material.c src/meshgen.c src/meshload.c src/modui.c \
	  src/mtlui.c src/options.c src/rbtree.c src/rend.c src/rtk.c \
	  src/rtk_draw.c src/scene.c src/scr_mod.c src/scr_rend.c src/texture.c \
	  src/gfxutil.c src/util.c \
	  src/sys_glut/main.c src/sys_glut/miniglut.c src/gaw/gaw_gl.c

obj = $(src:.c=.o)
bin = retroray

def = -DGFX_GL
inc = -Isrc -Isrc/sys_glut -Ilibs -Ilibs/imago/src -Ilibs/treestor/include -Ilibs/drawtext
libs = libs/unix/imago.a libs/unix/treestor.a libs/unix/drawtext.a

CFLAGS = $(CFLAGS_extra) $(warn) $(dbg) $(opt) $(inc) $(def)
LDFLAGS = $(LDFLAGS_extra) $(libs) -lGL -lGLU -lX11 -lm

$(bin): $(obj) build-libs
	$(CC) -o $@ $(obj) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: cleanall
cleanall: clean clean-libs

.PHONY: build-libs
build-libs:
	cd libs && $(MAKE)

.PHONY: clean-libs
clean-libs:
	cd libs && $(MAKE) clean

.PHONY: data
data:
	tools/procdata
