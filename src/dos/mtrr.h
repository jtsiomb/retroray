#ifndef MTRR_H_
#define MTRR_H_

#define MTRR_WC		1

int get_page_memtype(uint32_t addr, int num_ranges);
int check_wrcomb_enabled(uint32_t addr, int len, int num_ranges);
int alloc_mtrr(int num_ranges);
void enable_wrcomb(uint32_t addr, int len);

#endif	/* MTRR_H_ */
