/* 
 * Direct MTD block device access
 *
 * $Id: mtdblock.c,v 1.66 2004/11/25 13:52:52 joern Exp $
 *
 * (C) 2000-2003 Nicolas Pitre <nico@cam.org>
 * (C) 1999-2003 David Woodhouse <dwmw2@infradead.org>
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>

#define MARS_DEBUG 0
#if MARS_DEBUG
      #define debug_mars(fmt, arg...)  printk(fmt, ##arg);
#else
      #define debug_mars(fmt, arg...)
#endif

/* Ken: 20090211 */
#define ERASE_NAND 1
#define MTD_NAND_WITHOUT_WAIT_QUEUE	(1)
#define MTD_DISABLE_READ_CACHE		(1)

#define NAND_TOTAL_BLOCKS			(8192)
static unsigned char* g_cTotalBlks = NULL;

static struct mtdblk_dev {
	struct mtd_info *mtd;
	int count;
	struct semaphore cache_sem;
	unsigned char *cache_data;
	unsigned long cache_offset;
	unsigned int cache_size;
	enum { STATE_EMPTY, STATE_CLEAN, STATE_DIRTY } cache_state;
} *mtdblks[MAX_MTD_DEVICES];

/*
 * Cache stuff...
 * 
 * Since typical flash erasable sectors are much larger than what Linux's
 * buffer cache can handle, we must implement read-modify-write on flash
 * sectors for each block write requests.  To avoid over-erasing flash sectors
 * and to speed things up, we locally cache a whole flash sector while it is
 * being written to until a different sector is required.
 */

static void erase_callback(struct erase_info *done)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
	wait_queue_head_t *wait_q = (wait_queue_head_t *)done->priv;
	wake_up(wait_q);
}

static unsigned char isBlockErased(unsigned int blkIdx)
{
	return g_cTotalBlks[blkIdx];
}
static void setBlockErased(unsigned int blkIdx)
{
	g_cTotalBlks[blkIdx] = 0xFF;
}
static int erase_write (struct mtd_info *mtd, unsigned long pos, 
			int len, const char *buf)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
//	printk("$");

	struct erase_info erase;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	size_t retlen;
	int ret;

	/*
	 * First, let's erase the flash block.
	 */
	//Ken, 20080816
	//printk("[%s] mtd->type =%d\n", __FUNCTION__, mtd->type);
	if ( mtd->type == MTD_NANDFLASH || strstr(mtd->name, "nand") ){	//Nand flash
//printk("---------[%d]----------\n", __LINE__);
#if ERASE_NAND	//Ken: 20090211
		if (isBlockErased(pos/mtd->erasesize) == 0x00){

		#if MTD_NAND_WITHOUT_WAIT_QUEUE
			erase.mtd = mtd;
			erase.addr = (pos/mtd->erasesize)*mtd->erasesize;
			erase.len = len;

			ret = MTD_ERASE(mtd, &erase);
			if (ret) {
				printk (KERN_WARNING "MTD_NAND_WITHOUT_WAIT_QUEUE: mtdblock:erase nand of region [0x%lx, 0x%x] "
						     "on \"%s\" failed\n", pos, len, mtd->name);
				return ret;
			}
			else
				setBlockErased(pos/mtd->erasesize);

		#else
			init_waitqueue_head(&wait_q);
			erase.mtd = mtd;
			erase.callback = erase_callback;
			erase.addr = pos;
			erase.len = len;
			erase.priv = (u_long)&wait_q;

			set_current_state(TASK_INTERRUPTIBLE);
			add_wait_queue(&wait_q, &wait);
			ret = MTD_ERASE(mtd, &erase);
			if (ret) {
				set_current_state(TASK_RUNNING);
				remove_wait_queue(&wait_q, &wait);
				printk (KERN_WARNING "mtdblock:erase nand of region [0x%lx, 0x%x] "
						     "on \"%s\" failed\n", pos, len, mtd->name);
				return ret;
			}
			//schedule();  /* Wait for erase to finish. */	//This will hang up when using dd
			remove_wait_queue(&wait_q, &wait);
		 #endif
		}
#endif		
	}else{		//Nor flash
		init_waitqueue_head(&wait_q);
		erase.mtd = mtd;
		erase.callback = erase_callback;
		erase.addr = pos;
		erase.len = len;
		erase.priv = (u_long)&wait_q;

		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&wait_q, &wait);

		ret = MTD_ERASE(mtd, &erase);
		if (ret) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			printk (KERN_WARNING "mtdblock: erase of region [0x%lx, 0x%x] "
					     "on \"%s\" failed\n",
				pos, len, mtd->name);
			return ret;
		}

		schedule();  /* Wait for erase to finish. */
		remove_wait_queue(&wait_q, &wait);	
	}
	/*
	 * Next, write data to flash.
	 */
	ret = MTD_WRITE (mtd, pos, len, &retlen, buf);
	if (ret)
		return ret;
	if (retlen != len)
		return -EIO;
	return 0;
}


static int write_cached_data (struct mtdblk_dev *mtdblk)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
	struct mtd_info *mtd = mtdblk->mtd;
	int ret;

	if (mtdblk->cache_state != STATE_DIRTY)
		return 0;

	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: writing cached data for \"%s\" "
			"at 0x%lx, size 0x%x\n", mtd->name, 
			mtdblk->cache_offset, mtdblk->cache_size);
	
	ret = erase_write (mtd, mtdblk->cache_offset, 
			   mtdblk->cache_size, mtdblk->cache_data);
	if (ret)
		return ret;

	/*
	 * Here we could argubly set the cache state to STATE_CLEAN.
	 * However this could lead to inconsistency since we will not 
	 * be notified if this content is altered on the flash by other 
	 * means.  Let's declare it empty and leave buffering tasks to
	 * the buffer cache instead.
	 */
	mtdblk->cache_state = STATE_EMPTY;
	return 0;
}


static int do_cached_write (struct mtdblk_dev *mtdblk, unsigned long pos, 
			    int len, const char *buf)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
	struct mtd_info *mtd = mtdblk->mtd;
	unsigned int sect_size = mtdblk->cache_size;
	size_t retlen;
	int ret;
	

	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: write on \"%s\" at 0x%lx, size 0x%x\n",
		mtd->name, pos, len);

//printk( "mtdblock: write on \"%s\" at 0x%lx, size 0x%x\n",
//		mtd->name, pos, len);	
	if (!sect_size)
		return MTD_WRITE (mtd, pos, len, &retlen, buf);

	while (len > 0) {
		unsigned long sect_start = (pos/sect_size)*sect_size;
		unsigned int offset = pos - sect_start;
		unsigned int size = sect_size - offset;
		//printk("[%s] sect_start=0x%x, pos=0x%x, offset=0x%x, sect_size=%d, size=%d\n", 
					//__FUNCTION__, sect_start, pos, offset, sect_size, size);
		if( size > len ) 
			size = len;
		//Ken, 20080903
		//printk("[%s] mtd->type =%d\n", __FUNCTION__, mtd->type);
		if ( mtd->type == MTD_NANDFLASH || strstr(mtd->name, "nand") ){
			ret = erase_write (mtd, pos, size, buf);
			if (ret)
				return ret;
		}else{
			if (size == sect_size) {
			/* 
			 * We are covering a whole sector.  Thus there is no
			 * need to bother with the cache while it may still be
			 * useful for other partial writes.
			 */
				ret = erase_write (mtd, pos, size, buf);
				if (ret)
					return ret;
			} else {
			/* Partial sector: need to use the cache */
				if (mtdblk->cache_state == STATE_DIRTY &&
				    mtdblk->cache_offset != sect_start) {
					ret = write_cached_data(mtdblk);
					if (ret) 
						return ret;
				}

				if (mtdblk->cache_state == STATE_EMPTY ||
				    mtdblk->cache_offset != sect_start) {
					/* fill the cache with the current sector */
					mtdblk->cache_state = STATE_EMPTY;
					ret = MTD_READ(mtd, sect_start, sect_size, &retlen, mtdblk->cache_data);
					if (ret)
						return ret;
					if (retlen != sect_size)
						return -EIO;

					mtdblk->cache_offset = sect_start;
					mtdblk->cache_size = sect_size;
					mtdblk->cache_state = STATE_CLEAN;
				}

				/* write data to our local cache */
				memcpy (mtdblk->cache_data + offset, buf, size);
				mtdblk->cache_state = STATE_DIRTY;
			}
		}
		buf += size;
		pos += size;
		len -= size;
	}

	return 0;
}


static int do_cached_read (struct mtdblk_dev *mtdblk, unsigned long pos, 
			   int len, char *buf)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
//printk("#");
	struct mtd_info *mtd = mtdblk->mtd;
	unsigned int sect_size = mtdblk->cache_size;
	size_t retlen;
	int ret;

	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: read on \"%s\" at 0x%lx, size 0x%x\n", 
			mtd->name, pos, len);
	
	//printk("mtdblock: read on \"%s\" at 0x%lx, size 0x%x\n", 
	//			mtd->name, pos, len);
		

	if (!sect_size)
		return MTD_READ (mtd, pos, len, &retlen, buf);
	
	while (len > 0) {
		unsigned long sect_start = (pos/sect_size)*sect_size;
		unsigned int offset = pos - sect_start;
		unsigned int size = sect_size - offset;
		if (size > len) 
			size = len;
#if 0 //To disable MTD cache while access NAND flash. Add by alexchang 0504-2010
		if ( mtd->type == MTD_NANDFLASH || strstr(mtd->name, "nand") ){
			ret = MTD_READ (mtd, pos, size, &retlen, buf);
			if (ret)
				return ret;

		}
		else
#endif
		{

			/*
			 * Check if the requested data is already cached
			 * Read the requested amount of data from our internal cache if it
			 * contains what we want, otherwise we read the data directly
			 * from flash.
			 */
			if (mtdblk->cache_state != STATE_EMPTY &&
			    mtdblk->cache_offset == sect_start) {
				memcpy (buf, mtdblk->cache_data + offset, size);
			} else {
				ret = MTD_READ (mtd, pos, size, &retlen, buf);
				if (ret)
					return ret;
				if (retlen != size)
					return -EIO;
			}
		}

		buf += size;
		pos += size;
		len -= size;
	}

	return 0;
}

static int mtdblock_readsect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];
	//Ken, 20080903
	struct mtd_info *mtd = mtdblk->mtd;
	//printk("[%s] mtd->type =%d\n", __FUNCTION__, mtd->type);
	if ( mtd->type == MTD_NANDFLASH || strstr(mtd->name, "nand") )
		return do_cached_read(mtdblk, block<<9, mtd->oobblock, buf);	
	else
		return do_cached_read(mtdblk, block<<9, 512, buf);
}

static int mtdblock_writesect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];
	if (unlikely(!mtdblk->cache_data && mtdblk->cache_size)) {
		mtdblk->cache_data = vmalloc(mtdblk->mtd->erasesize);
		if (!mtdblk->cache_data)
			return -EINTR;
		/* -EINTR is not really correct, but it is the best match
		 * documented in man 2 write for all cases.  We could also
		 * return -EAGAIN sometimes, but why bother?
		 */
	}
	//Ken, 20080903
	struct mtd_info *mtd = mtdblk->mtd;
	//printk("[%s] mtd->type =%d\n", __FUNCTION__, mtd->type);
	if ( mtd->type == MTD_NANDFLASH || strstr(mtd->name, "nand") )
		return do_cached_write(mtdblk, block<<9, mtd->oobblock, buf);
	else
		return do_cached_write(mtdblk, block<<9, 512, buf);	//Ken, org	
}

static int mtdblock_open(struct mtd_blktrans_dev *mbd)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
	struct mtdblk_dev *mtdblk;
	struct mtd_info *mtd = mbd->mtd;
	int dev = mbd->devnum;

	DEBUG(MTD_DEBUG_LEVEL1,"mtdblock_open\n");
	
	if (mtdblks[dev]) {
		mtdblks[dev]->count++;
		return 0;
	}
	
	/* OK, it's not open. Create cache info for it */
	mtdblk = kmalloc(sizeof(struct mtdblk_dev), GFP_KERNEL);
	if (!mtdblk)
		return -ENOMEM;

	memset(mtdblk, 0, sizeof(*mtdblk));
	mtdblk->count = 1;
	mtdblk->mtd = mtd;

	init_MUTEX (&mtdblk->cache_sem);
	mtdblk->cache_state = STATE_EMPTY;

	if ((mtdblk->mtd->flags & MTD_CAP_RAM) != MTD_CAP_RAM &&
	    mtdblk->mtd->erasesize) {
		mtdblk->cache_size = mtdblk->mtd->erasesize;
		mtdblk->cache_data = NULL;
	}

	if(!g_cTotalBlks)
	{
		g_cTotalBlks = vmalloc(NAND_TOTAL_BLOCKS);
		if (!g_cTotalBlks)
		{
			printk("[AT]g_cTotalBlks allocate fail..\n");
			return -ENOMEM;
		}
		memset(g_cTotalBlks,0,NAND_TOTAL_BLOCKS);
	}
	mtdblks[dev] = mtdblk;
	
	DEBUG(MTD_DEBUG_LEVEL1, "ok\n");

	return 0;
}

static int mtdblock_release(struct mtd_blktrans_dev *mbd)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
	int dev = mbd->devnum;
	struct mtdblk_dev *mtdblk = mtdblks[dev];

   	DEBUG(MTD_DEBUG_LEVEL1, "mtdblock_release\n");

	down(&mtdblk->cache_sem);
	write_cached_data(mtdblk);
	up(&mtdblk->cache_sem);

	if (!--mtdblk->count) {
		/* It was the last usage. Free the device */
		mtdblks[dev] = NULL;
		if (mtdblk->mtd->sync)
			mtdblk->mtd->sync(mtdblk->mtd);
		vfree(mtdblk->cache_data);
		kfree(mtdblk);
		
	}
	DEBUG(MTD_DEBUG_LEVEL1, "ok\n");
	
	if(g_cTotalBlks)
	{
		//printk("[AT]vfree g_cTotalBlks..\n");
		vfree(g_cTotalBlks);
		g_cTotalBlks = NULL;
	}
	printk("[mtdblock_release] Done\n");

	return 0;
}  

static int mtdblock_flush(struct mtd_blktrans_dev *dev)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];
	down(&mtdblk->cache_sem);
	write_cached_data(mtdblk);
	up(&mtdblk->cache_sem);

	if (mtdblk->mtd->sync)
		mtdblk->mtd->sync(mtdblk->mtd);
	return 0;
}

static void mtdblock_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
	struct mtd_blktrans_dev *dev = kmalloc(sizeof(*dev), GFP_KERNEL);

	if (!dev)
		return;

	memset(dev, 0, sizeof(*dev));

	dev->mtd = mtd;
	dev->devnum = mtd->index;
	//Ken, 20080903
	//printk("[%s] mtd->type =%d\n", __FUNCTION__, mtd->type);
	if ( mtd->type == MTD_NANDFLASH || strstr(mtd->name, "nand") ){
		dev->blksize = mtd->oobblock;
		dev->size = mtd->size/mtd->oobblock;
	}else{
		dev->blksize = 512;	
		dev->size = mtd->size >> 9;	
	}
	dev->tr = tr;

	if (!(mtd->flags & MTD_WRITEABLE))
		dev->readonly = 1;

	add_mtd_blktrans_dev(dev);
}

static void mtdblock_remove_dev(struct mtd_blktrans_dev *dev)
{
debug_mars("---------[%s]----------\n", __FUNCTION__);
	del_mtd_blktrans_dev(dev);
	kfree(dev);
}

static struct mtd_blktrans_ops mtdblock_tr = {
	.name		= "mtdblock",
	.major		= 31,
	.part_bits	= 0,
	.open		= mtdblock_open,
	.flush		= mtdblock_flush,
	.release	= mtdblock_release,
	.readsect	= mtdblock_readsect,
	.writesect	= mtdblock_writesect,
	.add_mtd	= mtdblock_add_mtd,
	.remove_dev	= mtdblock_remove_dev,
	.owner		= THIS_MODULE,
};

static int __init init_mtdblock(void)
{
	return register_mtd_blktrans(&mtdblock_tr);
}

static void __exit cleanup_mtdblock(void)
{
	deregister_mtd_blktrans(&mtdblock_tr);
}

module_init(init_mtdblock);
module_exit(cleanup_mtdblock);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicolas Pitre <nico@cam.org> et al.");
MODULE_DESCRIPTION("Caching read/erase/writeback block device emulation access to MTD devices");
