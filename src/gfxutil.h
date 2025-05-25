/*
RetroRay - integrated standalone vintage modeller/renderer
Copyright (C) 2023  John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef GFXUTIL_H_
#define GFXUTIL_H_

#ifdef GFX_GL
#define PACK_RGB32(r, g, b)	(0xff000000 | (r) | ((g) << 8) | ((b) << 16))
#define UNP_RED(c)			((c) & 0xff)
#define UNP_GREEN(c)		(((c) & 0xff00) >> 8)
#define UNP_BLUE(c)			(((c) & 0xff0000) >> 16)
#else
#define PACK_RGB32(r, g, b)	(0xff000000 | ((r) << 16) | ((g) << 8) | (b))
#define UNP_RED(c)			(((c) & 0xff0000) >> 16)
#define UNP_GREEN(c)		(((c) & 0xff00) >> 8)
#define UNP_BLUE(c)			((c) & 0xff)
#endif


#endif	/* GFXUTIL_H_ */
