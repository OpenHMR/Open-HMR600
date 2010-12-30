/*
 * $Id: RPCkern.c,v 1.10 2004/8/4 09:25 Jacky Exp $
 */
#include <linux/config.h>
/*
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
    #define MODVERSIONS
#endif

#ifdef MODVERSIONS
    #include <linux/modversions.h>
#endif

#ifndef MODULE
#define MODULE
#endif
*/
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/ioctl.h>    /* needed for the _IOW etc stuff used later */
#include <linux/delay.h>
#include <linux/RPCDriver.h>

#include <asm/io.h>
#include <asm/system.h>     /* cli(), *_flags */
#include <asm/uaccess.h>    /* copy_to_user() copy_from_user() */

//#define PDEBUG(fmt, args...) printk(KERN_ALERT "RPC: " fmt, ## args)

typedef struct RPC_STRUCT {
	unsigned long programID;      // program ID defined in IDL file
	unsigned long versionID;      // version ID defined in IDL file
	unsigned long procedureID;    // function ID defined in IDL file

	unsigned long taskID;         // the caller's task ID, assign 0 if NONBLOCK_MODE
	unsigned long parameterSize;  // packet's body size
	unsigned long mycontext;       // return address of reply value
} RPC_STRUCT;

RPC_KERN_Dev *rpc_kern_devices;

static int rpc_kernel_thread(void *p);

int rpc_kern_init(void)
{
	static is_init = 0;
	int result = 0, num, i;

	num = RPC_NR_KERN_DEVS;

	// Create corresponding structures for each device.
	rpc_kern_devices = (RPC_KERN_Dev *)RPC_KERN_RECORD_ADDR;

	for (i = 0; i < num; i++) {
		PDEBUG("rpc_kern_device %d addr: %x \n", i, (unsigned)&rpc_kern_devices[i]);
		rpc_kern_devices[i].ringBuf = (char *)(RPC_KERN_DEV_ADDR+i*RPC_RING_SIZE);

		// Initialize pointers...
		rpc_kern_devices[i].ringStart = rpc_kern_devices[i].ringBuf;
		rpc_kern_devices[i].ringEnd = rpc_kern_devices[i].ringBuf+RPC_RING_SIZE;
		rpc_kern_devices[i].ringIn = rpc_kern_devices[i].ringBuf;
		rpc_kern_devices[i].ringOut = rpc_kern_devices[i].ringBuf;

		PDEBUG("The %dth RPC_KERN_Dev:\n", i);
		PDEBUG("RPC ringStart: 0x%8x\n", (int)rpc_kern_devices[i].ringStart);
		PDEBUG("RPC ringEnd:   0x%8x\n", (int)rpc_kern_devices[i].ringEnd);
		PDEBUG("RPC ringIn:    0x%8x\n", (int)rpc_kern_devices[i].ringIn);
		PDEBUG("RPC ringOut:   0x%8x\n", (int)rpc_kern_devices[i].ringOut);
		PDEBUG("\n");

		if (!is_init) {
			// Initialize wait queue...
			init_waitqueue_head(&(rpc_kern_devices[i].waitQueue));
	
			// Initialize sempahores...
			sema_init(&rpc_kern_devices[i].readSem, 1);
			sema_init(&rpc_kern_devices[i].writeSem, 1);
	
			if (i%RPC_NR_PAIR == 1)
				kernel_thread(rpc_kernel_thread, (void *)i, CLONE_KERNEL);
		}
	}
	is_init = 1;

	return result;
}

ssize_t rpc_kern_read(int opt, char *buf, size_t count)
{
    RPC_KERN_Dev *dev;
    int temp, size;
    ssize_t ret = 0;
    char *ptmp;

    dev = &rpc_kern_devices[opt*RPC_NR_KERN_DEVS/RPC_NR_PAIR+1];
    PDEBUG("read rpc_kern_device: %x \n", (unsigned int)dev);
    if (down_interruptible(&dev->readSem))
        return -ERESTARTSYS;

    if (dev->ringIn == dev->ringOut)
        goto out;   // the ring is empty...
    else if (dev->ringIn > dev->ringOut)
        size = dev->ringIn - dev->ringOut;
    else
        size = RPC_RING_SIZE + dev->ringIn - dev->ringOut;

	if (count > size)
		count = size;
	
	temp = dev->ringEnd - dev->ringOut;
	if (temp >= count) {
		if (my_copy_user((int *)buf, (int *)dev->ringOut, count)) {
        	ret = -EFAULT;
			goto out;
		}
		ret += count;
		ptmp = dev->ringOut + ((count+3) & 0xfffffffc);
		if (ptmp == dev->ringEnd)
			dev->ringOut = dev->ringStart;
		else
			dev->ringOut = ptmp;
    	
    	PDEBUG("RPC Read is in 1st kind...\n");
	} else {
		if (my_copy_user((int *)buf, (int *)dev->ringOut, temp)) {
        	ret = -EFAULT;
			goto out;
		}
		count -= temp;
		
		if (my_copy_user((int *)(buf+temp), (int *)dev->ringStart, count)) {
        	ret = -EFAULT;
			goto out;
		}
		ret += (temp + count);
		dev->ringOut = dev->ringStart+((count+3) & 0xfffffffc);
    	
    	PDEBUG("RPC Read is in 2nd kind...\n");
	}
out:
    PDEBUG("RPC kern ringOut pointer is : 0x%8x\n", (int)dev->ringOut);
    up(&dev->readSem);
    return ret;
}

ssize_t rpc_kern_write(int opt, const char *buf, size_t count)
{
    RPC_KERN_Dev *dev;
    int temp, size;
    ssize_t ret = 0;
    char *ptmp;

    dev = &rpc_kern_devices[opt*RPC_NR_KERN_DEVS/RPC_NR_PAIR];
    PDEBUG("read rpc_kern_device: %x \n", (unsigned int)dev);
    if (down_interruptible(&dev->writeSem))
        return -ERESTARTSYS;

    if (dev->ringIn == dev->ringOut)
        size = 0;   // the ring is empty
    else if (dev->ringIn > dev->ringOut)
        size = dev->ringIn - dev->ringOut;
    else
        size = RPC_RING_SIZE + dev->ringIn - dev->ringOut;

	if (count > (RPC_RING_SIZE - size - 1))
		goto out;

	temp = dev->ringEnd - dev->ringIn;
	if (temp >= count) {
		if (my_copy_user((int *)dev->ringIn, (int *)buf, count)) {
        	ret = -EFAULT;
			goto out;
		}
		ret += count;
		ptmp = dev->ringIn + ((count+3) & 0xfffffffc);

		__asm__ __volatile__ ("sync;");

		if (ptmp == dev->ringEnd)
			dev->ringIn = dev->ringStart;
		else
			dev->ringIn = ptmp;
    	
    	PDEBUG("RPC Write is in 1st kind...\n");
	} else {
		if (my_copy_user((int *)dev->ringIn, (int *)buf, temp)) {
        	ret = -EFAULT;
			goto out;
		}
		count -= temp;
		
		if (my_copy_user((int *)dev->ringStart, (int *)(buf+temp), count)) {
        	ret = -EFAULT;
			goto out;
		}
		ret += (temp + count);

		__asm__ __volatile__ ("sync;");

		dev->ringIn = dev->ringStart+((count+3) & 0xfffffffc);
    	
    	PDEBUG("RPC Write is in 2nd kind...\n");
	}

	if (opt == RPC_AUDIO)
		writel(0x3, (void *)0xb801a104);        // audio
	else if (opt == RPC_VIDEO)
		writel(0x5, (void *)0xb801a104);        // video
	else
		printk("error device number...\n");

out:
    PDEBUG("RPC kern ringIn pointer is : 0x%8x\n", (int)dev->ringIn);
    up(&dev->writeSem);
    return ret;
}

static int rpc_kernel_thread(void *p)
{
	char readbuf[sizeof(RPC_STRUCT)+2*sizeof(unsigned long)];
	RPC_KERN_Dev *dev;
	RPC_STRUCT *rpc;
	unsigned long *tmp;
	char name[20];

	sprintf(name, "rpc-%d", (int)p);
	daemonize(name);

	dev = &rpc_kern_devices[(int)p];
	while (1) {
		if (current->flags & PF_FREEZE)
			refrigerator(PF_FREEZE);

//		printk(" #@# wait %s %x %x \n", current->comm, dev, dev->waitQueue);
		if (wait_event_interruptible(dev->waitQueue, dev->ringIn != dev->ringOut) == -ERESTARTSYS) {
			printk("%s got signal...\n", current->comm);
			continue;
		}
//		printk(" #@# wakeup %s \n", current->comm);

		// read the reply data...
		if (rpc_kern_read(((int)p)/RPC_NR_PAIR, readbuf, sizeof(readbuf)) != sizeof(readbuf)) {
			printk("ERROR in read kernel RPC...\n");
			continue;
		}

		// parse the reply data...
		rpc = (RPC_STRUCT *)readbuf;
		tmp = (unsigned long *)(readbuf+sizeof(RPC_STRUCT));
		*((unsigned long *)ntohl(rpc->mycontext)) = ntohl(*(tmp+1));
		*((unsigned long *)ntohl(*tmp)) = 1; // ack the sync...
	}

	return 0;
}

int send_rpc_command(int opt, unsigned long command, unsigned long param1, unsigned long param2, unsigned long *retvalue)
{
	char sendbuf[sizeof(RPC_STRUCT)+3*sizeof(unsigned long)];
	RPC_STRUCT *rpc = (RPC_STRUCT *)sendbuf;
	unsigned long *tmp;
	unsigned long sync = 0;

//	printk(" #@# sendbuf: %d \n", sizeof(sendbuf));
	// fill the RPC_STRUCT...
	rpc->programID = htonl(KERNELID);
	rpc->versionID = htonl(KERNELID);
	rpc->procedureID = 0;
	rpc->taskID = htonl((unsigned long)&sync);
	rpc->parameterSize = htonl(3*sizeof(unsigned long));
	rpc->mycontext = htonl((unsigned long)retvalue);

	// fill the parameters...
	tmp = (unsigned long *)(sendbuf+sizeof(RPC_STRUCT));
//	printk(" aaa: %x bbb: %x \n", sendbuf, tmp);
	*tmp = htonl(command);
	*(tmp+1) = htonl(param1);
	*(tmp+2) = htonl(param2);

	if (rpc_kern_write(opt, sendbuf, sizeof(sendbuf)) != sizeof(sendbuf)) {
		printk("ERROR in send kernel RPC...\n");
		return RPC_FAIL;
	}

	// sync the result...
	while (sync == 0) msleep(1);
	
//	printk(" #@# ret: %d \n", *retvalue);

	return RPC_OK;
}

