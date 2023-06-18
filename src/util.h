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
#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include "sizeint.h"

#if defined(__WATCOMC__) || defined(_WIN32) || defined(__DJGPP__)
#include <malloc.h>
#else
#include <alloca.h>
#endif

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

unsigned int get_cs(void);
#define get_cpl()	((int)(get_cs() & 3))

void get_msr(uint32_t msr, uint32_t *low, uint32_t *high);
void set_msr(uint32_t msr, uint32_t low, uint32_t high);


/* Non-failing versions of malloc/calloc/realloc. They never return 0, they call
 * demo_abort on failure. Use the macros, don't call the *_impl functions.
 */
#define malloc_nf(sz)	malloc_nf_impl(sz, __FILE__, __LINE__)
void *malloc_nf_impl(size_t sz, const char *file, int line);
#define calloc_nf(n, sz)	calloc_nf_impl(n, sz, __FILE__, __LINE__)
void *calloc_nf_impl(size_t num, size_t sz, const char *file, int line);
#define realloc_nf(p, sz)	realloc_nf_impl(p, sz, __FILE__, __LINE__)
void *realloc_nf_impl(void *p, size_t sz, const char *file, int line);
#define strdup_nf(s)	strdup_nf_impl(s, __FILE__, __LINE__)
char *strdup_nf_impl(const char *s, const char *file, int line);

int match_prefix(const char *str, const char *prefix);

void enable_fpexcept(void);
void disable_fpexcept(void);

#ifndef INLINE
#if (__STDC_VERSION__ >= 199901) || defined(__GNUC__)
#define INLINE inline
#else
#define INLINE __inline
#endif
#endif

#if defined(__i386__) || defined(__386__) || defined(MSDOS)

/* fast conversion of double -> 32bit int
 * for details see:
 *  - http://chrishecker.com/images/f/fb/Gdmfp.pdf
 *  - http://stereopsis.com/FPU.html#convert
 */
static INLINE int32_t cround64(double val)
{
	val += 6755399441055744.0;
	return *(int32_t*)&val;
}
#else
#define cround64(x)	((int32_t)(x))
#endif

#endif	/* UTIL_H_ */
