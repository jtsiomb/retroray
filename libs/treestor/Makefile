include ../../config.mk

obj = src/treestor.o src/text.o src/dynarr.o
alib = ../unix/treestor.a

CFLAGS = $(CFLAGS_extra) $(opt) $(dbg) -Iinclude

$(alib): $(obj)
	$(AR) rcs $@ $(obj)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(obj) $(alib)
