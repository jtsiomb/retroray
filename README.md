RetroRay
========

Status: early development. Not usable yet.

![shot](http://nuclear.mutantstargoat.com/sw/retroray/img/latest-thumb.png)

The primary target is DOS, running on a PC with at least a 486 (ideally pentium
or faster), and a VESA-compatible SVGA card. Currently VBE 2.0 with a linear
framebuffer is required. If your graphics card doesn't support VBE 2.0 natively,
run univbe first. This restriction will be lifted later on as the VBE 1.x
support becomes more complete.

On other platforms (UNIX, Windows, etc) OpenGL is used for video output.

Retroray needs to find some data files at runtime. The data files are not
included in the git repository, you need to download them separately from:
http://nuclear.mutantstargoat.com/sw/retroray/releases/rraydata.zip. Extract the
archive in the project root directory. This will create a `data/` directory
where retroray will expect to find its assets.

License
-------
Copyright (C) 2023 John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software. Feel free to use, modify, and/or redistribute it
under the terms of the GNU General Public License v3, or at your option, any
later version published buy the Free Software Foundation. See COPYING for
details.

DOS Build
---------
You will need some version of the Watcom C compiler installed, either natively
under DOS or as a cross-compiler. Simply run `wmake` to build.

UNIX build
----------
Just type `make` (or `gmake` on non-GNU systems). Installing is not supported
yet; run it from the project directory. There are no dependencies other than
OpenGL and Xlib.

Windows build
-------------
From a mingw shell, just type `make`. No dependencies other than OpenGL. To
cross-compile from UNIX, try `make crosswin`, assuming `i686-w64-mingw32-gcc` is
in the path.
