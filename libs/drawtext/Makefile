obj = font.obj draw.obj drawgl.obj drawrast.obj utf8.obj

!ifdef __UNIX__
alib = ../../drawtext.lib
!else
alib = ..\..\drawtext.lib
!endif

CC = wcc386
CFLAGS = -d1 -4 -otexan -zq -DNO_FREETYPE -DNO_OPENGL

$(alib): $(obj)
	%write objects.lbc $(obj)
	wlib -b -n $@ @objects

.c.obj: .autodepend
	%write cflags.occ $(CFLAGS)
	$(CC) -fo=$@ @cflags.occ $[*

!ifdef __UNIX__
clean: .symbolic
	rm -f $(obj)
	rm -f $(alib)
!else
clean: .symbolic
	del *.obj
	del objects.lbc
	del cflags.occ
	del $(alib)
!endif
