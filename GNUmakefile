-include config.mk

# --- default build option values ---
build_gfx ?= gl
build_opt ?= false
build_dbg ?= true
# -----------------------------------

gawsrc_gl = src/gaw/gaw_gl.c
gawsrc_sw = src/gaw/gaw_sw.c src/gaw/gawswtnl.c src/gaw/polyfill.c src/gaw/polyclip.c

gawdef_gl = -DGFX_GL
gawdef_sw = -DGFX_SW

src = $(wildcard src/*.c) $(wildcard src/modern/*.c) $(gawsrc_$(build_gfx))
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = retroray

warn = -pedantic -Wall
ifeq ($(build_opt), true)
	opt = -O3
endif
ifeq ($(build_dbg), true)
	dbg = -g
endif
def = $(gawdef_$(build_gfx))
inc = -Isrc -Isrc/modern -Ilibs -Ilibs/imago/src -Ilibs/treestor/include -Ilibs/drawtext
libs = libs/unix/imago.a libs/unix/treestor.a libs/unix/drawtext.a

CFLAGS = $(warn) $(dbg) $(opt) $(inc) $(def) $(cflags_$(rend)) -MMD
LDFLAGS = $(ldsys_pre) $(libs) $(ldsys)

sys := $(shell uname -s | sed 's/MINGW.*/mingw/')
ifeq ($(sys), mingw)
	bin = retroray.exe

	ldsys = -lopengl32 -lglu32 -lgdi32 -lwinmm
	ldsys_pre = -static-libgcc -lmingw32 -mconsole
else
	ldsys = -lGL -lGLU -lX11 -lm
endif

$(bin): $(obj) libs
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: libs
libs:
	$(MAKE) -C libs

.PHONY: cleanlibs
cleanlibs:
	$(MAKE) -C libs clean

.PHONY: cleanall
cleanall: clean cleanlibs cleandep

.PHONY: data
data:
	tools/procdata

.PHONY: crosswin
crosswin:
	$(MAKE) CC=i686-w64-mingw32-gcc sys=mingw

.PHONY: crosswin-clean
crosswin-clean:
	$(MAKE) CC=i686-w64-mingw32-gcc sys=mingw clean

.PHONY: crosswin-cleandep
crosswin-cleandep:
	$(MAKE) CC=i686-w64-mingw32-gcc sys=mingw cleandep
