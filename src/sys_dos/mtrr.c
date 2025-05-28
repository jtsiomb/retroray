#include "mtrr.h"

#define MSR_MTRRCAP			0xfe
#define MSR_MTRRDEFTYPE		0x2ff
#define MSR_MTRRBASE(x)		(0x200 | ((x) << 1))
#define MSR_MTRRMASK(x)		(0x201 | ((x) << 1))
#define MTRRDEF_EN			0x800
#define MTRRCAP_HAVE_WC		0x400
#define MTRRMASK_VALID		0x800

static const char *mtrr_type_name(int type);
static void print_mtrr(void);


int get_page_memtype(uint32_t addr, int num_ranges)
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

int check_wrcomb_enabled(uint32_t addr, int len, int num_ranges)
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

int alloc_mtrr(int num_ranges)
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

void enable_wrcomb(uint32_t addr, int len)
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
