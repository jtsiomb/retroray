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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "logger.h"

#if defined(__MSDOS__) || defined(MSDOS)
static int setup_serial(int sdev);

void ser_putchar(int c);
void ser_puts(const char *s);
void ser_printf(const char *fmt, ...);
#else
#define USE_STD
#endif

enum { LOG_FILE, LOG_STREAM, LOG_CON, LOG_CB };
enum { LOG_DBG, LOG_INFO, LOG_WARN, LOG_ERR };

struct log_callback {
	void (*func)(const char*, void*);
	void *cls;
};

struct log_output {
	int type, level;
	union {
		FILE *fp;
		int con;
		struct log_callback cb;
	} out;
};

#define MAX_OUTPUTS	8
static struct log_output outputs[MAX_OUTPUTS];
static int num_outputs;

void init_logger(void)
{
	num_outputs = 0;
}

void cleanup_logger(void)
{
	int i;

	for(i=0; i<num_outputs; i++) {
		if(outputs[i].type == LOG_FILE) {
			fclose(outputs[i].out.fp);
		}
	}
	num_outputs = 0;
}

int add_log_file(const char *fname)
{
	FILE *fp;
	int idx;

	if(num_outputs >= MAX_OUTPUTS) {
		return -1;
	}
	if(!(fp = fopen(fname, "w"))) {
		return -1;
	}
	idx = num_outputs++;

	outputs[idx].type = LOG_FILE;
	outputs[idx].out.fp = fp;
	return 0;
}

int add_log_stream(FILE *fp)
{
	int idx;

	if(num_outputs >= MAX_OUTPUTS) {
		return -1;
	}
	idx = num_outputs++;

	outputs[idx].type = LOG_STREAM;
	outputs[idx].out.fp = fp;
	return 0;
}

int add_log_console(const char *devname)
{
#if defined(MSDOS) || defined(__MSDOS__)
	int i, comport;
	if(sscanf(devname, "COM%d", &comport) != 1 || comport < 1 || comport > 2) {
		return -1;
	}
	comport--;

	if(num_outputs >= MAX_OUTPUTS) {
		return -1;
	}
	for(i=0; i<num_outputs; i++) {
		if(outputs[i].type == LOG_CON && outputs[i].out.con == comport) {
			return -1;
		}
	}
	if(setup_serial(comport) == -1) {
		return -1;
	}

	i = num_outputs++;
	outputs[i].type = LOG_CON;
	outputs[i].out.con = comport;
	return 0;

#elif defined(unix) || defined(__unix__)
	/* TODO? */
	return -1;
#endif
}

int add_log_callback(void (*cbfunc)(const char*, void*), void *cls)
{
	int idx;

	if(num_outputs >= MAX_OUTPUTS) {
		return -1;
	}
	idx = num_outputs++;

	outputs[idx].type = LOG_CB;
	outputs[idx].out.cb.func = cbfunc;
	outputs[idx].out.cb.cls = cls;
	return 0;
}

#if defined(__WATCOMC__)
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#endif

static void logmsg(int type, const char *fmt, va_list ap)
{
	static char buf[2048];
	int i;

	vsnprintf(buf, sizeof buf, fmt, ap);

	for(i=0; i<num_outputs; i++) {
		switch(outputs[i].type) {
		case LOG_FILE:
		case LOG_STREAM:
			fputs(buf, outputs[i].out.fp);
			fflush(outputs[i].out.fp);
			break;

#if defined(MSDOS) || defined(__MSDOS__)
		case LOG_CON:
			ser_puts(buf);
			break;
#endif
		case LOG_CB:
			outputs[i].out.cb.func(buf, outputs[i].out.cb.cls);
			break;

		default:
			break;
		}
	}
}

void errormsg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logmsg(LOG_ERR, fmt, ap);
	va_end(ap);
}

void warnmsg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logmsg(LOG_WARN, fmt, ap);
	va_end(ap);
}

void infomsg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logmsg(LOG_INFO, fmt, ap);
	va_end(ap);
}

void dbgmsg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logmsg(LOG_DBG, fmt, ap);
	va_end(ap);
}

void verrormsg(const char *fmt, va_list ap)
{
	logmsg(LOG_ERR, fmt, ap);
}

void vwarnmsg(const char *fmt, va_list ap)
{
	logmsg(LOG_ERR, fmt, ap);
}

void vinfomsg(const char *fmt, va_list ap)
{
	logmsg(LOG_ERR, fmt, ap);
}

void vdbgmsg(const char *fmt, va_list ap)
{
	logmsg(LOG_ERR, fmt, ap);
}


#if defined(MSDOS) || defined(__MSDOS__)
#include <conio.h>

#define UART1_BASE	0x3f8
#define UART2_BASE	0x2f8

#define UART_DATA	0
#define UART_DIVLO	0
#define UART_DIVHI	1
#define UART_FIFO	2
#define UART_LCTL	3
#define UART_MCTL	4
#define UART_LSTAT	5

#define DIV_9600			(115200 / 9600)
#define DIV_38400			(115200 / 38400)
#define LCTL_8N1			0x03
#define LCTL_DLAB			0x80
#define FIFO_ENABLE_CLEAR	0x07
#define MCTL_DTR_RTS_OUT2	0x0b
#define LST_TRIG_EMPTY		0x20

static unsigned int iobase;

static int setup_serial(int sdev)
{
	if(sdev < 0 || sdev > 1) {
		return -1;
	}
	iobase = sdev == 0 ? UART1_BASE : UART2_BASE;

	/* set clock divisor */
	outp(iobase | UART_LCTL, LCTL_DLAB);
	outp(iobase | UART_DIVLO, DIV_9600 & 0xff);
	outp(iobase | UART_DIVHI, DIV_9600 >> 8);
	/* set format 8n1 */
	outp(iobase | UART_LCTL, LCTL_8N1);
	/* assert RTS and DTR */
	outp(iobase | UART_MCTL, MCTL_DTR_RTS_OUT2);
	return 0;
}

void ser_putchar(int c)
{
	if(c == '\n') {
		ser_putchar('\r');
	}

	while((inp(iobase | UART_LSTAT) & LST_TRIG_EMPTY) == 0);
	outp(iobase | UART_DATA, c);
}

void ser_puts(const char *s)
{
	while(*s) {
		ser_putchar(*s++);
	}
}

void ser_printf(const char *fmt, ...)
{
	va_list ap;
	char buf[512];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	ser_puts(buf);
}
#endif
