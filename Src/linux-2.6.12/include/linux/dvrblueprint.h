#ifndef _LINUX_DVRBLUEPRINT_H
#define _LINUX_DVRBLUEPRINT_H

#include <linux/config.h>
#include <linux/list.h>

#ifndef CONFIG_REALTEK_ADVANCED_RECLAIM

#ifdef __KERNEL__

struct mem_unit {
	struct list_head	lru;
	unsigned long		order;
};

void init_dvrbp(void);
void exit_dvrbp(void);
struct page *alloc_dvrbp_memory(unsigned int order);
void free_dvrbp_memory(struct page *page, unsigned int order);
void show_dvrbp(void);

#endif /* __KERNEL__ */
#endif /* CONFIG_REALTEK_ADVANCED_RECLAIM */
#endif /* _LINUX_DVRBLUEPRINT_H */

