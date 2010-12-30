/*
 *  linux/mm/dvrblueprint.c
 *
 *  Copyright (C) 1991, 1992, 1993, 1994  Linus Torvalds
 *
 *  Support for blueprint of dvr zone, I-Chieh Hsu
 */

#include <linux/mm.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mmzone.h>
#include <linux/dvrblueprint.h>

#ifndef CONFIG_REALTEK_ADVANCED_RECLAIM

static struct free_area dvr_free_area[MAX_ORDER];
static struct mem_unit *dvr_mem_map = 0;;
static unsigned int map_order = 0;

static inline struct mem_unit *
expand(struct mem_unit *unit,
		int low, int high, struct free_area *area)
{
	unsigned long size = 1 << high;
	
	while (high > low) {
		area--;
		high--;
		size >>= 1;
//		BUG_ON(bad_range(zone, &page[size]));
		list_add(&unit[size].lru, &area->free_list);
		area->nr_free++;
		unit[size].order = high;
	}
	return unit;
}

static struct mem_unit *alloc_mem_unit_bulk(unsigned int order)
{
	struct free_area * area;
	unsigned int current_order;
	struct mem_unit *unit;

	for (current_order = order; current_order < MAX_ORDER; ++current_order) {
		area = dvr_free_area + current_order;
		if (list_empty(&area->free_list))
			        continue;

		unit = list_entry(area->free_list.next, struct mem_unit, lru);
//		printk(" @@ alloc unit: %x \n", unit);
		list_del(&unit->lru);
		unit->order = 0;
		area->nr_free--;
		return expand(unit, order, current_order, area);
	}

	return NULL;
}

static inline struct mem_unit *
unit_find_buddy(struct mem_unit *unit, unsigned long unit_idx, unsigned int order)
{
	unsigned long buddy_idx = unit_idx ^ (1 << order);

	return unit + (buddy_idx - unit_idx);
}

static void free_mem_unit_bulk(struct mem_unit *unit, unsigned int order)
{
	unsigned long unit_idx;
	int order_size = 1 << order;

	unit_idx = (unit - dvr_mem_map) & ((1 << MAX_ORDER) - 1);
//	printk(" @@ 1unit: %x index: %d \n", unit, unit_idx);

	BUG_ON(unit_idx & (order_size - 1));
//	BUG_ON(bad_range(zone, page));

	while (order < MAX_ORDER-1) {
		unsigned long combined_idx;
		struct free_area *area;
		struct mem_unit *buddy;

//		printk(" @@ 2unit: %x index: %d \n", unit, unit_idx);
		combined_idx = (unit_idx & ~(1 << order));
		buddy = unit_find_buddy(unit, unit_idx, order);
//		printk(" @@ buddy: %x combine: %d \n", buddy, combined_idx);

//		if (bad_range(zone, buddy))
//			break;
		if (buddy->order != order)
			break;          /* Move the buddy up one level. */
//		printk(" @@ combine buddy %x %x\n", unit, buddy);
		list_del(&buddy->lru);
		area = dvr_free_area + order;
		area->nr_free--;
		buddy->order = 0;
		unit = unit + (combined_idx - unit_idx);
		unit_idx = combined_idx;
		order++;
	}
//	printk(" @@ end \n");
	unit->order = order;
	list_add(&unit->lru, &dvr_free_area[order].free_list);
	dvr_free_area[order].nr_free++;
}

void init_dvrbp(void)
{
	struct zone *zone = zone_table[ZONE_DVR];
	struct page *page = zone->zone_mem_map;
	int count = zone->spanned_pages;
	int order, index;

	// inii free lists
	for (order = 0; order < MAX_ORDER ; order++) {
		INIT_LIST_HEAD(&dvr_free_area[order].free_list);
		dvr_free_area[order].nr_free = 0;
	}

	map_order = fls(count * sizeof(struct mem_unit)) - 1;
	if ((count * sizeof(struct mem_unit)) == (1 << map_order))
		map_order -= 12;
	else
		map_order -= 11;
	dvr_mem_map = (struct mem_unit *)__get_free_pages(GFP_KERNEL, map_order);
	memset(dvr_mem_map, 0, count * sizeof(struct mem_unit));
	printk("   AllocBP size: %d order: %d \n", count * sizeof(struct mem_unit), map_order);

	// add the dvr zone into our free lists (use 1MB as unit)
	for (index = 0; index < count; index+=256) {
		if (!PageReserved(page+index))
			free_mem_unit_bulk(dvr_mem_map+index, 8);
	}
	show_dvrbp();
}

void exit_dvrbp(void)
{
	if (dvr_mem_map) {
		free_pages((unsigned long)dvr_mem_map, map_order);
		dvr_mem_map = 0;
	}
}

static inline int check_page(struct zone *zone, struct page *page)
{
	if (page_to_pfn(page) >= zone->zone_start_pfn + zone->spanned_pages)
		return 1;
	if (page_to_pfn(page) < zone->zone_start_pfn)
		return 1;
	if (zone != page_zone(page))
		return 1;
	return 0;
}

struct page *alloc_dvrbp_memory(unsigned int order)
{
	struct zone *zone = zone_table[ZONE_DVR];
	struct page *page;
	struct mem_unit *unit;
	int index;

	unit = alloc_mem_unit_bulk(order);
	if (unit == 0)
		return NULL;
	index = unit - dvr_mem_map;
	printk("+++ ALLOC index: %d order: %d \n", index, order);

	page = zone->zone_mem_map + index;
	BUG_ON(check_page(zone, page));
//	printk(" zone: %x page: %x count: %d \n", zone->zone_mem_map, page, 1 << order);

	return page;
}

void free_dvrbp_memory(struct page *page, unsigned int order)
{
	struct zone *zone = zone_table[ZONE_DVR];
	struct mem_unit *unit;
	int index;

	if (page == NULL)
		return;
#ifdef CONFIG_REALTEK_MARS_256MB
	if (zone != page_zone(page))
		return;
#endif
	BUG_ON(check_page(zone, page));
	index = page - zone->zone_mem_map;
	printk("--- FREE index: %d order: %d \n", index, order);

	unit = dvr_mem_map + index;
	free_mem_unit_bulk(unit, order);
}

void show_dvrbp(void)
{
	struct zone *zone = zone_table[ZONE_DVR];
	int idx;

	printk("\t<<<DVR FREE AREA>>>\n");
	for (idx = 0; idx < MAX_ORDER; idx++) {
		struct list_head *up;
		struct mem_unit *unit;
		struct page *page;
		int index;

		printk("\t%d:\t%d\n", idx, (int)dvr_free_area[idx].nr_free);

		list_for_each(up, &dvr_free_area[idx].free_list) {
			unit = list_entry(up, struct mem_unit, lru);
			index = unit - dvr_mem_map;
			page = zone->zone_mem_map + index;
			printk("\t\tAddr: %p, order: %d \n", page_address(page), (int)unit->order);
		}
	}
}

#endif /* CONFIG_REALTEK_ADVANCED_RECLAIM */

