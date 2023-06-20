#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "app.h"
#include "cdpmi.h"
#include "gfx.h"
#include "vbe.h"
#include "vga.h"
#include "util.h"
#include "cpuid.h"
#include "logger.h"

#ifdef __DJGPP__
#define VMEM_PTR	((void*)(0xa0000 + __djgpp_conventional_base))
#else
#define VMEM_PTR	((void*)0xa0000)
#endif

#define SAME_BPP(a, b)	\
	((a) == (b) || ((a) == 16 && (b) == 15) || ((a) == 15 && (b) == 16) || \
	 ((a) == 32 && (b) == 24) || ((a) == 24 && (b) == 32))

void (*blit_frame)(void*, int);

static void blit_frame_lfb(void *pixels, int vsync);
static void blit_frame_banked(void *pixels, int vsync);
static uint32_t calc_mask(int sz, int pos);

static void enable_wrcomb(uint32_t addr, int len);
static const char *mtrr_type_name(int type);
static void print_mtrr(void);

static struct video_mode *vmodes;
static int num_vmodes;

static int vbe_init_ver;
static struct vbe_info vbe;

/* current mode */
static struct video_mode *curmode;
static void *vpgaddr[2];
static int frontidx, backidx;
static int pgcount, pgsize, fbsize;


int init_video(void)
{
	int i, num, max_modes;
	struct video_mode *vmptr;

	if(vbe_info(&vbe) == -1) {
		errormsg("failed to retrieve VBE information\n");
		return -1;
	}
	vbe_print_info(stdout, &vbe);

	num_vmodes = 0;
	max_modes = 64;
	if(!(vmodes = malloc(max_modes * sizeof *vmodes))) {
		errormsg("failed to allocate video modes list\n");
		return -1;
	}

	num = vbe_num_modes(&vbe);
	for(i=0; i<num; i++) {
		struct vbe_mode_info minf;

		if(vbe_mode_info(vbe.modes[i], &minf) == -1) {
			continue;
		}

		if(num_vmodes >= max_modes) {
			int newmax = max_modes ? (max_modes << 1) : 16;
			if(!(vmptr = realloc(vmodes, newmax * sizeof *vmodes))) {
				errormsg("failed to grow video mode list (%d)\n", newmax);
				free(vmodes);
				return -1;
			}
			vmodes = vmptr;
			max_modes = newmax;
		}

		vmptr = vmodes + num_vmodes++;
		memset(vmptr, 0, sizeof *vmptr);
		vmptr->mode = vbe.modes[i];
		vmptr->xsz = minf.xres;
		vmptr->ysz = minf.yres;
		vmptr->bpp = minf.bpp;
		vmptr->pitch = minf.scanline_bytes;
		if(minf.mem_model == VBE_TYPE_DIRECT) {
			vmptr->rbits = minf.rsize;
			vmptr->gbits = minf.gsize;
			vmptr->bbits = minf.bsize;
			vmptr->rshift = minf.rpos;
			vmptr->gshift = minf.gpos;
			vmptr->bshift = minf.bpos;
			vmptr->rmask = calc_mask(minf.rsize, minf.rpos);
			vmptr->gmask = calc_mask(minf.gsize, minf.gpos);
			vmptr->bmask = calc_mask(minf.bsize, minf.bpos);
			/*vmptr->bpp = vmptr->rbits + vmptr->gbits + vmptr->bbits;*/
		}
		if(minf.attr & VBE_ATTR_LFB) {
			vmptr->fb_addr = minf.fb_addr;
		}
		vmptr->max_pages = minf.num_img_pages;
		vmptr->win_gran = minf.win_gran;

		infomsg("%04x: ", vbe.modes[i]);
		vbe_print_mode_info(stdout, &minf);
	}
	/*fflush(stdout);*/

	vbe_init_ver = VBE_VER_MAJOR(vbe.ver);
	return 0;
}

void cleanup_video(void)
{
	free(vmodes);
}

struct video_mode *video_modes(void)
{
	return vmodes;
}

int num_video_modes(void)
{
	return num_vmodes;
}

struct video_mode *get_video_mode(int idx)
{
	if(idx == VMODE_CURRENT) {
		return curmode;
	}
	return vmodes + idx;
}

int match_video_mode(int xsz, int ysz, int bpp)
{
	int i, best = -1;
	struct video_mode *vm;

	for(i=0; i<num_vmodes; i++) {
		vm = vmodes + i;
		if(vm->xsz != xsz || vm->ysz != ysz) continue;
		if(SAME_BPP(vm->bpp, bpp)) {
			best = i;
		}
		if(vm->bpp == bpp) break;
	}

	if(best == -1) {
		errormsg("failed to find video mode %dx%d %d bpp)\n", xsz, ysz, bpp);
		return -1;
	}
	return best;
}

int find_video_mode(int mode)
{
	int i;
	struct video_mode *vm;

	vm = vmodes;
	for(i=0; i<num_vmodes; i++) {
		if(vm->mode == mode) return i;
	}
	return -1;
}

void *set_video_mode(int idx, int nbuf)
{
	unsigned int mode;
	struct video_mode *vm = vmodes + idx;

	if(curmode == vm) return vpgaddr[0];

	infomsg("setting video mode %x (%dx%d %d bpp)\n", (unsigned int)vm->mode,
			vm->xsz, vm->ysz, vm->bpp);
	/*fflush(stdout);*/

	mode = vm->mode | VBE_MODE_LFB;
	if(vbe_setmode(mode) == -1) {
		infomsg("Warning: failed to get a linear framebuffer. falling back to banked mode\n");
		mode = vm->mode;
		if(vbe_setmode(mode) == -1) {
			errormsg("failed to set video mode %x\n", (unsigned int)vm->mode);
			return 0;
		}
	}

	/* unmap previous video memory mapping, if there was one (switching modes) */
	if(vpgaddr[0] && vpgaddr[0] != VMEM_PTR) {
		dpmi_munmap(vpgaddr[0]);
		vpgaddr[0] = vpgaddr[1] = 0;
	}

	curmode = vm;
	if(nbuf < 1) nbuf = 1;
	if(nbuf > 2) nbuf = 2;
	pgcount = nbuf > vm->max_pages + 1 ? vm->max_pages + 1 : nbuf;
	pgsize = vm->ysz * vm->pitch;
	fbsize = pgcount * pgsize;

	if(vm->bpp > 8) {
		infomsg("rgb mask: %x %x %x\n", (unsigned int)vm->rmask,
				(unsigned int)vm->gmask, (unsigned int)vm->bmask);
		infomsg("rgb shift: %d %d %d\n", vm->rshift, vm->gshift, vm->bshift);
	}
	infomsg("pgcount: %d, pgsize: %d, fbsize: %d\n", pgcount, pgsize, fbsize);
	if(vm->fb_addr) {
		infomsg("phys addr: %p\n", (void*)vm->fb_addr);
	}
	/*fflush(stdout);*/

	if(vm->fb_addr) {
		vpgaddr[0] = (void*)dpmi_mmap(vm->fb_addr, fbsize);
		if(!vpgaddr[0]) {
			errormsg("failed to map framebuffer (phys: %lx, size: %d)\n",
					(unsigned long)vm->fb_addr, fbsize);
			set_text_mode();
			return 0;
		}
		memset(vpgaddr[0], 0xaa, pgsize);

		if(pgcount > 1) {
			vpgaddr[1] = (char*)vpgaddr[0] + pgsize;
			backidx = 1;
			page_flip(FLIP_NOW);	/* start with the second page visible */
		} else {
			frontidx = backidx = 0;
			vpgaddr[1] = 0;
		}

		blit_frame = blit_frame_lfb;

		/* only attempt to set up write combining if the CPU we're running on
		 * supports memory type range registers, and we're running on ring 0
		 */
		if(CPU_HAVE_MTRR) {
			int cpl = get_cpl();
			if(cpl > 0) {
				errormsg("Can't set framebuffer range to write-combining, running in ring %d\n", cpl);
			} else {
				uint32_t len = (uint32_t)vbe.vmem_blk << 16;

				/* if vmem_blk is 0 or if the reported size is absurd (more than
				 * 256mb), just use the framebuffer size for this mode to setup the
				 * mtrr
				 */
				if(!len || len > 0x10000000) {
					infomsg("reported vmem too large or overflowed, using fbsize for wrcomb setup\n");
					len = fbsize;
				}
				print_mtrr();
				enable_wrcomb(vm->fb_addr, len);
			}
		}

	} else {
		vpgaddr[0] = VMEM_PTR;
		vpgaddr[1] = 0;

		blit_frame = blit_frame_banked;

		/* calculate window granularity shift */
		vm->win_gran_shift = 0;
		vm->win_64k_step = 1;
		if(vm->win_gran > 0 && vm->win_gran < 64) {
			int gran = vm->win_gran;
			while(gran < 64) {
				vm->win_gran_shift++;
				gran <<= 1;
			}
			vm->win_64k_step = 1 << vm->win_gran_shift;
		}

		infomsg("granularity: %dk (step: %d)\n", vm->win_gran, vm->win_64k_step);
	}

	/*fflush(stdout);*/
	return vpgaddr[0];
}

int set_text_mode(void)
{
	/* unmap previous video memory mapping, if there was one (switching modes) */
	if(vpgaddr[0] && vpgaddr[0] != VMEM_PTR) {
		dpmi_munmap(vpgaddr[0]);
		vpgaddr[0] = vpgaddr[1] = 0;
	}

	vga_setmode(3);
	curmode = 0;
	return 0;
}

void *page_flip(int vsync)
{
	if(!vpgaddr[1]) {
		/* page flipping not supported */
		return vpgaddr[0];
	}

	vbe_swap(backidx ? pgsize : 0, vsync ? VBE_SWAP_VBLANK : VBE_SWAP_NOW);
	frontidx = backidx;
	backidx = (backidx + 1) & 1;

	return vpgaddr[backidx];
}


static void blit_frame_lfb(void *pixels, int vsync)
{
	if(vsync) wait_vsync();
	memcpy(vpgaddr[frontidx], pixels, pgsize);
}

static void blit_frame_banked(void *pixels, int vsync)
{
	int sz, offs, pending;
	unsigned char *pptr = pixels;

	if(vsync) wait_vsync();

	/* assume initial window offset at 0 */
	offs = 0;
	pending = pgsize;
	while(pending > 0) {
		sz = pending > 65536 ? 65536 : pending;
		memcpy(VMEM_PTR, pptr, sz);
		pptr += sz;
		pending -= sz;
		offs += curmode->win_64k_step;
		vbe_setwin(0, offs);
	}
	vbe_setwin(0, 0);
}

static uint32_t calc_mask(int sz, int pos)
{
	uint32_t mask = 0;
	while(sz-- > 0) {
		mask = (mask << 1) | 1;
	}
	return mask << pos;
}

#define MSR_MTRRCAP			0xfe
#define MSR_MTRRDEFTYPE		0x2ff
#define MSR_MTRRBASE(x)		(0x200 | ((x) << 1))
#define MSR_MTRRMASK(x)		(0x201 | ((x) << 1))
#define MTRRDEF_EN			0x800
#define MTRRCAP_HAVE_WC		0x400
#define MTRRMASK_VALID		0x800

#define MTRR_WC				1

static int get_page_memtype(uint32_t addr, int num_ranges)
{
	int i;
	uint32_t rlow, rhigh;
	uint32_t base, mask;

	for(i=0; i<num_ranges; i++) {
		get_msr(MSR_MTRRMASK(i), &rlow, &rhigh);
		if(!(rlow & MTRRMASK_VALID)) {
			continue;
		}
		mask = rlow & 0xfffff000;

		get_msr(MSR_MTRRBASE(i), &rlow, &rhigh);
		base = rlow & 0xfffff000;

		if((addr & mask) == (base & mask)) {
			return rlow & 0xff;
		}
	}

	get_msr(MSR_MTRRDEFTYPE, &rlow, &rhigh);
	return rlow & 0xff;
}

static int check_wrcomb_enabled(uint32_t addr, int len, int num_ranges)
{
	while(len > 0) {
		if(get_page_memtype(addr, num_ranges) != MTRR_WC) {
			return 0;
		}
		addr += 4096;
		len -= 4096;
	}
	return 1;
}

static int alloc_mtrr(int num_ranges)
{
	int i;
	uint32_t rlow, rhigh;

	for(i=0; i<num_ranges; i++) {
		get_msr(MSR_MTRRMASK(i), &rlow, &rhigh);
		if(!(rlow & MTRRMASK_VALID)) {
			return i;
		}
	}
	return -1;
}

static void enable_wrcomb(uint32_t addr, int len)
{
	int num_ranges, mtrr;
	uint32_t rlow, rhigh;
	uint32_t def, mask;

	if(len <= 0 || (addr | (uint32_t)len) & 0xfff) {
		errormsg("failed to enable write combining, unaligned range: %p/%x\n",
				(void*)addr, (unsigned int)len);
		return;
	}

	get_msr(MSR_MTRRCAP, &rlow, &rhigh);
	num_ranges = rlow & 0xff;

	infomsg("enable_wrcomb: addr=%p len=%x\n", (void*)addr, (unsigned int)len);

	if(!(rlow & MTRRCAP_HAVE_WC)) {
		errormsg("failed to enable write combining, processor doesn't support it\n");
		return;
	}

	if(check_wrcomb_enabled(addr, len, num_ranges)) {
		return;
	}

	if((mtrr = alloc_mtrr(num_ranges)) == -1) {
		errormsg("failed to enable write combining, no free MTRRs\n");
		return;
	}

	mask = len - 1;
	mask |= mask >> 1;
	mask |= mask >> 2;
	mask |= mask >> 4;
	mask |= mask >> 8;
	mask |= mask >> 16;
	mask = ~mask & 0xfffff000;

	infomsg("  ... mask: %08x\n", (unsigned int)mask);

	_disable();
	get_msr(MSR_MTRRDEFTYPE, &def, &rhigh);
	set_msr(MSR_MTRRDEFTYPE, def & ~MTRRDEF_EN, rhigh);

	set_msr(MSR_MTRRBASE(mtrr), addr | MTRR_WC, 0);
	set_msr(MSR_MTRRMASK(mtrr), mask | MTRRMASK_VALID, 0);

	set_msr(MSR_MTRRDEFTYPE, def | MTRRDEF_EN, 0);
	_enable();
}

static const char *mtrr_names[] = { "N/A", "W C", "N/A", "N/A", "W T", "W P", "W B" };

static const char *mtrr_type_name(int type)
{
	if(type < 0 || type >= sizeof mtrr_names / sizeof *mtrr_names) {
		return mtrr_names[0];
	}
	return mtrr_names[type];
}

static void print_mtrr(void)
{
	int i, num_ranges;
	uint32_t rlow, rhigh, base, mask;

	get_msr(MSR_MTRRCAP, &rlow, &rhigh);
	num_ranges = rlow & 0xff;

	for(i=0; i<num_ranges; i++) {
		get_msr(MSR_MTRRBASE(i), &base, &rhigh);
		get_msr(MSR_MTRRMASK(i), &mask, &rhigh);

		if(mask & MTRRMASK_VALID) {
			infomsg("mtrr%d: base %p, mask %08x type %s\n", i, (void*)(base & 0xfffff000),
					(unsigned int)(mask & 0xfffff000), mtrr_type_name(base & 0xff));
		} else {
			infomsg("mtrr%d unused (%08x/%08x)\n", i, (unsigned int)base,
					(unsigned int)mask);
		}
	}
	/*fflush(stdout);*/
}
