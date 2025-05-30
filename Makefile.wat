!ifdef __UNIX__
dosobj = src/sys_dos/main.obj src/sys_dos/keyb.obj src/sys_dos/mouse.obj src/sys_dos/timer.obj &
	src/sys_dos/cdpmi.obj src/sys_dos/vidsys.obj src/sys_dos/drv_vga.obj src/sys_dos/drv_vbe.obj &
	src/sys_dos/drv_s3.obj
appobj = src/app.obj src/cmesh.obj src/darray.obj src/font.obj src/logger.obj &
	src/meshgen.obj src/meshload.obj src/options.obj src/rbtree.obj src/geom.obj &
	src/rend.obj src/rtk.obj src/rtk_draw.obj src/scene.obj src/scr_mod.obj &
	src/modui.obj src/mtlui.obj src/scr_rend.obj src/texture.obj src/material.obj &
	src/gfxutil.obj src/util.obj src/util_s.obj src/cpuid.obj src/cpuid_s.obj
gawobj = src/gaw/gaw_sw.obj src/gaw/gawswtnl.obj src/gaw/polyclip.obj src/gaw/polyfill.obj

incpath = -Isrc -Isrc/sys_dos -Ilibs -Ilibs/imago/src -Ilibs/treestor/include -Ilibs/drawtext
libpath = libpath libs/dos
!else
dosobj = src\sys_dos\main.obj src\sys_dos\keyb.obj src\sys_dos\mouse.obj src\sys_dos\timer.obj &
	src\sys_dos\cdpmi.obj src\sys_dos\vidsys.obj src\sys_dos\drv_vga.obj src\sys_dos\drv_vbe.obj &
	src\sys_dos\drv_s3.obj
appobj = src\app.obj src\cmesh.obj src\darray.obj src\font.obj src\logger.obj &
	src\meshgen.obj src\meshload.obj src\options.obj src\rbtree.obj src\geom.obj &
	src\rend.obj src\rtk.obj src\rtk_draw.obj src\scene.obj src\scr_mod.obj &
	src\modui.obj src\mtlui.obj src\scr_rend.obj src\texture.obj src\material.obj &
	src\gfxutil.obj src\util.obj src\util_s.obj src\cpuid.obj src\cpuid_s.obj
gawobj = src\gaw\gaw_sw.obj src\gaw\gawswtnl.obj src\gaw\polyclip.obj src\gaw\polyfill.obj

incpath = -Isrc -Isrc\sys_dos -Ilibs -Ilibs\imago\src -Ilibs\treestor\include -Ilibs\drawtext
libpath = libpath libs\dos
!endif

obj = $(dosobj) $(appobj) $(gawobj)
bin = retroray.exe

opt = -otexan
def = -DGFX_SW
libs = imago.lib treestor.lib drawtext.lib

AS = nasm
CC = wcc386
LD = wlink
ASFLAGS = -fobj
CFLAGS = -d3 -4 $(opt) $(def) -s -zq -bt=dos $(incpath)
LDFLAGS = option map $(libpath) library { $(libs) }

$(bin): cflags.occ $(obj) $(libs)
	%write objects.lnk $(obj)
	%write ldflags.lnk $(LDFLAGS)
	$(LD) debug all name $@ system dos4g file { @objects } @ldflags

.c: src;src/sys_dos;src/gaw
.asm: src;src/sys_dos;src/gaw

cflags.occ: Makefile.wat
	%write $@ $(CFLAGS)

.c.obj: .autodepend
	$(CC) -fo=$@ @cflags.occ $[*

.asm.obj:
	nasm $(ASFLAGS) -o $@ $[*.asm


!ifdef __UNIX__
clean: .symbolic
	rm -f $(obj)
	rm -f $(bin)
	rm -f cflags.occ *.lnk

imago.lib:
	cd libs/imago
	wmake -f Makefile.wat
	cd ../..

treestor.lib:
	cd libs/treestor
	wmake -f Makefile.wat
	cd ../..

drawtext.lib:
	cd libs/drawtext
	wmake -f Makefile.wat
	cd ../..

!else

imago.lib:
	cd libs\imago
	wmake -f Makefile.wat
	cd ..\..

treestor.lib:
	cd libs\treestor
	wmake -f Makefile.wat
	cd ..\..

drawtext.lib:
	cd libs\drawtext
	wmake -f Makefile.wat
	cd ..\..

clean: .symbolic
	del src\*.obj
	del src\sys_dos\*.obj
	del src\gaw\*.obj
	del *.lnk
	del cflags.occ
	del $(bin)
!endif
