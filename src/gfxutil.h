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


#if defined(__sgi)
#define BUILD_BIGEND
#else
#define BUILD_LITEND
#endif


#ifdef BUILD_BIGEND

#define RSHIFT		24
#define GSHIFT		16
#define BSHIFT		8
#define ASHIFT		0

#else	/* little endian */

#ifdef GFX_GL
#define RSHIFT		0
#define GSHIFT		8
#define BSHIFT		16
#define ASHIFT		24
#else
#define RSHIFT		16
#define GSHIFT		8
#define BSHIFT		0
#define ASHIFT		24
#endif
#endif

#define RMASK		(0xff << RSHIFT)
#define GMASK		(0xff << GSHIFT)
#define BMASK		(0xff << BSHIFT)
#define AMASK		(0xff << ASHIFT)

#define PACK_RGB32(r, g, b)	(((r) << RSHIFT) | ((g) << GSHIFT) | ((b) << BSHIFT) | (0xff << ASHIFT))
#define UNP_RED(c)			(((c) & RMASK) >> RSHIFT)
#define UNP_GREEN(c)		(((c) & GMASK) >> GSHIFT)
#define UNP_BLUE(c)			(((c) & BMASK) >> BSHIFT)

#endif	/* GFXUTIL_H_ */
