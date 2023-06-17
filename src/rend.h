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
#ifndef REND_H_
#define REND_H_

#include "sizeint.h"
#include "imago2.h"

extern struct img_pixmap renderbuf;

int rend_init(void);
void rend_size(int xsz, int ysz);
void rend_begin(int x, int y, int w, int h);
int render(uint32_t *fb);

#endif	/* REND_H_ */
