#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "vidsys.h"
#include "vga.h"
#include "drv.h"
#include "cdpmi.h"

struct vid_driver *vid_drvlist[MAX_DRV];
int vid_numdrv;

void *vid_vmem;
int vid_vmem_size;

static struct vid_modeinfo **modes, *cur_mode;
static int num_modes, max_modes;


int vid_init(void)
{
	int i, j, len;
	struct vid_modeinfo *vm;

	vid_numdrv = 0;
	free(modes);
	modes = 0;
	cur_mode = 0;
	num_modes = max_modes = 0;

	vid_vmem = 0;
	vid_vmem_size = 0;

	if(dpmi_init() == -1) {
		return -1;
	}

	vid_register_vga();
	vid_register_vbe();
	vid_register_s3();

	for(i=0; i<vid_numdrv; i++) {
		struct vid_driver *drv = vid_drvlist[i];

		drv->ops->init();

		for(j=0; j<drv->num_modes; j++) {
			if(num_modes >= max_modes) {
				int newsz = max_modes ? max_modes * 2 : 128;
				void *tmp = realloc(modes, newsz * sizeof *modes);
				if(!tmp) {
					fprintf(stderr, "failed to allocate modes list\n");
					return -1;
				}
				modes = tmp;
				max_modes = newsz;
			}

			modes[num_modes++] = drv->modes + j;
		}
	}

	printf("found %d modes:\n", num_modes);
	for(i=0; i<num_modes; i+=2) {
		vm = modes[i];
		len = printf("[%4s] %04x: %dx%d %dbpp", vm->drv->name, vm->modeno,
				vm->width, vm->height, vm->bpp);
		if(i + 1 >= num_modes) {
			putchar('\n');
			break;
		}
		for(j=len; j<40; j++) putchar(' ');
		vm = modes[i + 1];
		printf("[%4s] %04x: %dx%d %dbpp\n", vm->drv->name, vm->modeno,
				vm->width, vm->height, vm->bpp);
	}

	return 0;
}

void vid_cleanup(void)
{
	int i;

	if(cur_mode && cur_mode->modeno != 3) {
		vid_setmode(3);
	}

	if(vid_vmem >= (void*)0x100000) {
		dpmi_munmap(vid_vmem);
	}

	for(i=0; i<vid_numdrv; i++) {
		struct vid_driver *drv = vid_drvlist[i];
		drv->ops->cleanup();
	}

	free(modes);
	dpmi_cleanup();
}


int vid_curmode(void)
{
	if(cur_mode) {
		return cur_mode->modeno;
	}
	return -1;
}

void *vid_setmode(int mode)
{
	int i;
	struct vid_driver *drv;

	for(i=0; i<num_modes; i++) {
		if(modes[i]->modeno == mode) {
			drv = modes[i]->drv;
			if(drv->ops->setmode(mode) == 0) {
				cur_mode = modes[i];

				if(vid_vmem >= (void*)0x100000) {
					assert(vid_vmem_size);
					dpmi_munmap(vid_vmem);
				}

				if(modes[i]->vmem_addr < 0x100000) {
					vid_vmem = (void*)modes[i]->vmem_addr;
					vid_vmem_size = 0;
				} else {
					vid_vmem = dpmi_mmap(modes[i]->vmem_addr, modes[i]->vmem_size);
					vid_vmem_size = modes[i]->vmem_size;
				}
				return vid_vmem;
			}
		}
	}
	return 0;
}

#define EQUIV_BPP(a, b)	\
	((a) == (b) || ((a) == 16 && (b) == 15) || ((a) == 15 && (b) == 16) || \
	 ((a) == 24 && (b) == 32) || ((a) == 32 && (b) == 24))

int vid_findmode(int xsz, int ysz, int bpp)
{
	int i;

	for(i=0; i<num_modes; i++) {
		if(modes[i]->width == xsz && modes[i]->height == ysz && modes[i]->bpp == bpp) {
			return modes[i]->modeno;
		}
	}

	/* try fuzzy bpp matching */
	for(i=0; i<num_modes; i++) {
		if(modes[i]->width == xsz && modes[i]->height == ysz &&
				EQUIV_BPP(modes[i]->bpp, bpp)) {
			return modes[i]->modeno;
		}
	}

	return -1;
}


struct vid_modeinfo *vid_modeinfo(int mode)
{
	int i;

	for(i=0; i<num_modes; i++) {
		if(modes[i]->modeno == mode) {
			return modes[i];
		}
	}
	return 0;
}

int vid_islinear(void)
{
	return !vid_isbanked();
}

int vid_isbanked(void)
{
	return cur_mode->win_size && vid_vmem < (void*)0x100000;
}

void vid_setpal(int idx, int count, const struct vid_color *col)
{
	cur_mode->ops.setpal(idx, count, col);
}

void vid_getpal(int idx, int count, struct vid_color *col)
{
	cur_mode->ops.getpal(idx, count, col);
}

void vid_blitfb(void *fb, int vsync)
{
	cur_mode->ops.blitfb(fb, vsync);
}