!ifdef __UNIX__
obj = src/treestor.obj src/text.obj src/dynarr.obj
alib = ../../treestor.lib
!else
obj = src\treestor.obj src\text.obj src\dynarr.obj
alib = ..\..\treestor.lib
!endif

CC = wcc386
CFLAGS = -d1 -5 -fp5 -otexan -zq -Iinclude

$(alib): $(obj)
	%write objects.lbc $(obj)
	wlib -b -n $@ @objects

.c: src

.c.obj: .autodepend
	%write cflags.occ $(CFLAGS)
	$(CC) -fo=$@ @cflags.occ $[*

!ifdef __UNIX__
clean: .symbolic
	rm -f $(obj)
	rm -f $(alib)
!else
clean: .symbolic
	del src\*.obj
	del objects.lbc
	del cflags.occ
	del $(alib)
!endif
