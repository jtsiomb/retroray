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
#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>
#include <stdarg.h>

void init_logger(void);
void cleanup_logger(void);

int add_log_file(const char *fname);
int add_log_stream(FILE *fp);
int add_log_console(const char *devname);
int add_log_callback(void (*cbfunc)(const char*, void*), void *cls);

void errormsg(const char *fmt, ...);
void warnmsg(const char *fmt, ...);
void infomsg(const char *fmt, ...);
void dbgmsg(const char *fmt, ...);

void verrormsg(const char *fmt, va_list ap);
void vwarnmsg(const char *fmt, va_list ap);
void vinfomsg(const char *fmt, va_list ap);
void vdbgmsg(const char *fmt, va_list ap);

#endif	/* LOGGER_H_ */
