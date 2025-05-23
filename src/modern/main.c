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
#include <assert.h>
#include "miniglut.h"
#include "app.h"
#include "options.h"
#include "rtk.h"
#include "logger.h"

static void display(void);
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void skeydown(int key, int x, int y);
static void skeyup(int key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);
static int translate_skey(int key);

#if defined(__unix__) || defined(unix)
#include <GL/glx.h>
static Display *xdpy;
static Window xwin;

static void (*glx_swap_interval_ext)();
static void (*glx_swap_interval_sgi)();
#endif
#ifdef _WIN32
#include <windows.h>
static PROC wgl_swap_interval_ext;
#endif

#ifndef GL_BGRA
#define GL_BGRA	0x80e1
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE	0x812f
#endif

static rtk_rect rband;


int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	load_options(CFGFILE);

	glutInitWindowSize(opt.xres * opt.scale, opt.yres * opt.scale);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("RetroRay");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(skeydown);
	glutSpecialUpFunc(skeyup);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(motion);
	glutSpaceballMotionFunc(app_sball_motion);
	glutSpaceballRotateFunc(app_sball_rotate);
	glutSpaceballButtonFunc(app_sball_button);

#if defined(__unix__) || defined(unix)
	xdpy = glXGetCurrentDisplay();
	xwin = glXGetCurrentDrawable();

	if(!(glx_swap_interval_ext = glXGetProcAddress((unsigned char*)"glXSwapIntervalEXT"))) {
		glx_swap_interval_sgi = glXGetProcAddress((unsigned char*)"glXSwapIntervalSGI");
	}
#endif
#ifdef _WIN32
	wgl_swap_interval_ext = wglGetProcAddress("wglSwapIntervalEXT");
#endif

	win_width = opt.xres;
	win_height = opt.yres;
	win_aspect = (float)win_width / win_height;

	init_logger();
	add_log_stream(stdout);

	if(app_init() == -1) {
		return 1;
	}
	atexit(app_shutdown);
	glutMainLoop();
	return 0;
}

unsigned long get_msec(void)
{
	return (unsigned long)glutGet(GLUT_ELAPSED_TIME);
}

void app_redisplay(int x, int y, int w, int h)
{
	glutPostRedisplay();
}

void app_swap_buffers(void)
{
	glViewport(0, 0, win_width * opt.scale, win_height * opt.scale);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, win_width, win_height, 0, -1, 1);

	glRasterPos2i(0, 0);
	glPixelZoom(opt.scale, -opt.scale);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	glDrawPixels(win_width, win_height, GL_BGRA, GL_UNSIGNED_BYTE, framebuf);
	glDisable(GL_ALPHA_TEST);

	if(rband.width | rband.height) {
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);

		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp(GL_XOR);

		glBegin(GL_LINE_LOOP);
		glColor3f(1, 1, 1);
		glVertex2f(rband.x, rband.y);
		glVertex2f(rband.x + rband.width, rband.y);
		glVertex2f(rband.x + rband.width, rband.y + rband.height);
		glVertex2f(rband.x, rband.y + rband.height);
		glEnd();

		glPopAttrib();
	}

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}

void app_quit(void)
{
	exit(0);
}

void app_resize(int x, int y)
{
	if(x == win_width * opt.scale && y == win_height * opt.scale) {
		return;
	}

	glutReshapeWindow(x * opt.scale, y * opt.scale);
}

void app_fullscreen(int fs)
{
	static int prev_w, prev_h;

	if(fs == -1) {
		fs = !fullscr;
	}

	if(fs == fullscr) return;

	if(fs) {
		prev_w = glutGet(GLUT_WINDOW_WIDTH);
		prev_h = glutGet(GLUT_WINDOW_HEIGHT);
		glutFullScreen();
	} else {
		glutReshapeWindow(prev_w, prev_h);
	}
	fullscr = fs;
}

#if defined(__unix__) || defined(unix)
void app_vsync(int vsync)
{
	vsync = vsync ? 1 : 0;
	if(glx_swap_interval_ext) {
		glx_swap_interval_ext(xdpy, xwin, vsync);
	} else if(glx_swap_interval_sgi) {
		glx_swap_interval_sgi(vsync);
	}
}
#endif
#ifdef WIN32
void app_vsync(int vsync)
{
	if(wgl_swap_interval_ext) {
		wgl_swap_interval_ext(vsync ? 1 : 0);
	}
}
#endif

void app_rband(int x, int y, int w, int h)
{
	rband.x = x;
	rband.y = y;
	rband.width = w;
	rband.height = h;

	glutPostRedisplay();
}


static void display(void)
{
	app_display();
	app_swap_buffers();
}

static void reshape(int x, int y)
{
	app_reshape(x / opt.scale, y / opt.scale);
}

static void keydown(unsigned char key, int x, int y)
{
	modkeys = glutGetModifiers();
	app_keyboard(key, 1);
}

static void keyup(unsigned char key, int x, int y)
{
	app_keyboard(key, 0);
}

static void skeydown(int key, int x, int y)
{
	int k;
	modkeys = glutGetModifiers();
	if((k = translate_skey(key)) >= 0) {
		app_keyboard(k, 1);
	}

	if(key == GLUT_KEY_F1) {
		glutPostRedisplay();
	}
}

static void skeyup(int key, int x, int y)
{
	int k = translate_skey(key);
	if(k >= 0) {
		app_keyboard(k, 0);
	}
}

static void mouse(int bn, int st, int x, int y)
{
	modkeys = glutGetModifiers();
	app_mouse(bn - GLUT_LEFT_BUTTON, st == GLUT_DOWN, x / opt.scale, y / opt.scale);
}

static void motion(int x, int y)
{
	app_motion(x / opt.scale, y / opt.scale);
}

static int translate_skey(int key)
{
	switch(key) {
	case GLUT_KEY_LEFT:
		return KEY_LEFT;
	case GLUT_KEY_UP:
		return KEY_UP;
	case GLUT_KEY_RIGHT:
		return KEY_RIGHT;
	case GLUT_KEY_DOWN:
		return KEY_DOWN;
	case GLUT_KEY_PAGE_UP:
		return KEY_PGUP;
	case GLUT_KEY_PAGE_DOWN:
		return KEY_PGDN;
	case GLUT_KEY_HOME:
		return KEY_HOME;
	case GLUT_KEY_END:
		return KEY_END;
	case GLUT_KEY_INSERT:
		return KEY_INS;
	default:
		if(key >= GLUT_KEY_F1 && key <= GLUT_KEY_F12) {
			return key - GLUT_KEY_F1 + KEY_F1;
		}
	}

	return -1;
}
