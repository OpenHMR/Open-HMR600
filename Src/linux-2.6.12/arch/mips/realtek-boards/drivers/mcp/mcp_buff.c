#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/auth.h>
#include <asm/semaphore.h>
#include <asm/io.h>
#include <platform.h>
#include <asm/mach-venus/mcp/mcp_buff.h>

#define MAX_MCPB_LEN        (4 * 1024)
#define USE_DVR_MALLOC

#ifdef MCPB_DBG_ENABLE
#define mcpb_dbg(fmt, args...)          printk("[MCP][BUF] DBG, " fmt, ## args)
#else
#define mcpb_dbg(fmt, args...)      
#endif

#define mcpb_warning(fmt, args...)      printk("[MCP][BUF] WARNING, " fmt, ## args)


void CHECK_BOUNDRARY(mcp_buff* mcpb)
{
    if (mcpb->data < mcpb->head || mcpb->tail > mcpb->end  || mcpb->tail < mcpb->data)
    {
        mcpb_warning("Errrrrrrrrrrrr mcpb(%p) : head=%p, data=%p, tail=%p, end=%p\n", 
                mcpb, mcpb->head, mcpb->data, mcpb->tail, mcpb->end);        
    }            
}


/*---------------------------------------------------------------------- 
 * Func : __alloc_mcpb 
 *
 * Desc : alloc a mcp buffer with specified length
 *
 * Parm : len : size of mcp buffer
 *
 * Retn : handle of mcp buffer
 *----------------------------------------------------------------------*/ 
static mcp_buff* __alloc_mcpb(unsigned int len)
{
    mcp_buff* p_buff;
    
    len += sizeof(mcp_buff);
        
#ifdef USE_DVR_MALLOC                                          
    p_buff =(len >= MAX_MCPB_LEN) ? dvr_malloc(len)
                                   : (mcp_buff*) kmalloc(len, GFP_KERNEL);
#else    
    p_buff = (len >= MAX_MCPB_LEN) ? (mcp_buff*) __get_free_pages(GFP_KERNEL, get_order(len))
                                   : (mcp_buff*) kmalloc(len, GFP_KERNEL);    
#endif

    if (p_buff)
    {
        mcpb_dbg("[MCP][BUF] Alloc Buffer %p (len = %d)\n", p_buff, len);
        p_buff->head = (unsigned char*) p_buff + sizeof(mcp_buff);
        p_buff->data = p_buff->head;
        p_buff->tail = p_buff->head;
        p_buff->end  = p_buff->head + len - sizeof(mcp_buff);
        p_buff->len  = 0;
    }

    return p_buff;
}




/*---------------------------------------------------------------------- 
 * Func : alloc_mcpb 
 *
 * Desc : alloc a mcp buffer with specified length
 *
 * Parm : len : size of mcp buffer
 *
 * Retn : handle of mcp buffer
 *----------------------------------------------------------------------*/ 
mcp_buff* alloc_mcpb(unsigned int len)
{
    mcp_buff* mcpb = __alloc_mcpb(len + MCP_BUFF_RESERVE_AREA);   

    if (!mcpb) 
    {
        printk("Errrr Alloc MCP Buffer failed, size to request = %d", (int) len + MCP_BUFF_RESERVE_AREA);
        return NULL;
    }

    mcpb_reserve(mcpb, MCP_BUFF_HEAD_RESERVE);

    return mcpb;
}




/*---------------------------------------------------------------------- 
 * Func : free_mcpb 
 *
 * Desc : free a mcp buffer with specified length
 *
 * Parm : mcpb : mcp buffer to be free
 *
 * Retn : N/A
 *----------------------------------------------------------------------*/ 
void free_mcpb(mcp_buff* mcpb)
{
    unsigned long len;
    
    if (mcpb)
    {        
        len = (unsigned long) mcpb->end - (unsigned long) mcpb;
        
        mcpb_dbg("Free Buffer %p (len = %ul)\n", mcpb, len);
                
#ifdef USE_DVR_MALLOC                   
        if (len >= MAX_MCPB_LEN) 
	   dvr_free(mcpb);
	else
	   kfree(mcpb);
#else
        if (len >= MAX_MCPB_LEN)
            free_pages((unsigned long) mcpb, get_order(len));
        else            
            kfree(mcpb);
#endif            
    }        
}



/*---------------------------------------------------------------------- 
 * Func : mcpb_reserve 
 *
 * Desc : reserve an area in front of mcp buffer
 *
 * Parm : mcpb : mcp buffer
 *        len : size to reserve
 *
 * Retn : N/A
 *----------------------------------------------------------------------*/ 
void mcpb_reserve(mcp_buff* mcpb, unsigned int len)
{
    mcpb->data += len;
    mcpb->tail += len;        
    CHECK_BOUNDRARY(mcpb);
}



/*---------------------------------------------------------------------- 
 * Func : mcpb_put 
 *
 * Desc : put data from tail of mcp buffer
 *
 * Parm : mcpb : mcp buffer
 *        len : size to put
 *
 * Retn : previous buffer tail
 *----------------------------------------------------------------------*/ 
unsigned char* mcpb_put(mcp_buff* mcpb, unsigned int len)
{
    unsigned char* ptr = mcpb->tail;            
    mcpb->tail += len;
    mcpb->len  += len;
    CHECK_BOUNDRARY(mcpb);                            
    return ptr;
}



/*---------------------------------------------------------------------- 
 * Func : mcpb_trim 
 *
 * Desc : remove data from tail of mcp buffer
 *
 * Parm : mcpb : mcp buffer
 *        len : size to remove
 *
 * Retn : N/A
 *----------------------------------------------------------------------*/ 
void mcpb_trim(mcp_buff* mcpb, unsigned int len)
{        
    mcpb->tail -= len;
    mcpb->len  -= len;
    CHECK_BOUNDRARY(mcpb);    
}



/*---------------------------------------------------------------------- 
 * Func : mcpb_push 
 *
 * Desc : push data to the front of mcp buffer
 *
 * Parm : mcpb : mcp buffer
 *        len : size to push
 *
 * Retn : new start address of mcp buffer data
 *----------------------------------------------------------------------*/ 
unsigned char* mcpb_push(mcp_buff *mcpb, unsigned int len)
{    
    mcpb->data -= len;
    mcpb->len  += len;
    CHECK_BOUNDRARY(mcpb);            
    return mcpb->data;
}



/*---------------------------------------------------------------------- 
 * Func : mcpb_pull 
 *
 * Desc : remove data from the front of mcp buffer
 *
 * Parm : mcpb : mcp buffer
 *        len : size to pull
 *
 * Retn : new start address of mcp buffer data
 *----------------------------------------------------------------------*/ 
unsigned char* mcpb_pull(mcp_buff *mcpb, unsigned int len)
{
    if (len <= mcpb->len)            
    {
        mcpb->data += len;
        mcpb->len  -= len;
        return mcpb->data;    
    }
    return NULL;    
}



/*---------------------------------------------------------------------- 
 * Func : __mcpb_purge 
 *
 * Desc : clear all data of mcp buffer
 *
 * Parm : mcpb : mcp buffer
 *
 * Retn : N/A
 *----------------------------------------------------------------------*/ 
void __mcpb_purge(mcp_buff* mcpb)
{
    mcpb->data = mcpb->head;
    mcpb->tail = mcpb->head;    
    mcpb->len  = 0;    
}


/*---------------------------------------------------------------------- 
 * Func : mcpb_purge 
 *
 * Desc : clear all data of mcp buffer
 *
 * Parm : mcpb : mcp buffer
 *
 * Retn : N/A
 *----------------------------------------------------------------------*/ 
void mcpb_purge(mcp_buff* mcpb)
{
    __mcpb_purge(mcpb);
    mcpb_reserve(mcpb, MCP_BUFF_HEAD_RESERVE);    
}



/*---------------------------------------------------------------------- 
 * Func : mcpb_headroom 
 *
 * Desc : get head room of mcp buffer
 *
 * Parm : mcpb : mcp buffer
 *
 * Retn : size of head room of mcpb
 *----------------------------------------------------------------------*/ 
int mcpb_headroom(mcp_buff* mcpb)
{
    return mcpb->data - mcpb->head;    
}



/*---------------------------------------------------------------------- 
 * Func : mcpb_tail_room 
 *
 * Desc : get tail room of mcp buffer
 *
 * Parm : mcpb : mcp buffer
 *
 * Retn : size of tail room of mcpb
 *----------------------------------------------------------------------*/ 
int mcpb_tailroom(mcp_buff* mcpb)
{
    return (int) (mcpb->end - mcpb->tail);
}


//////////////////////////////////////////////////////////////////////////////
// for debug .........
//////////////////////////////////////////////////////////////////////////////



/*---------------------------------------------------------------------- 
 * Func : _dump_data 
 *
 * Desc : dump data in memory
 *
 * Parm : data : start address of data
 *        len  : length of data
 *
 * Retn : N/A
 *----------------------------------------------------------------------*/ 
static void _dump_data(unsigned char* data, unsigned int len)
{
    int i;                  
    for (i=0; i<len; i++)
    {
        if ((i & 0xF)==0)
            printk("\n %04x | ", i);                    
        printk("%02x ", data[i]);                    
    }    
}



/*---------------------------------------------------------------------- 
 * Func : dump_mcpb_data 
 *
 * Desc : dump data in mcp buffer
 *
 * Parm : mcpb : mcp buffer 
 *
 * Retn : N/A
 *----------------------------------------------------------------------*/ 
void dump_mcpb_data(mcp_buff* mcpb)
{    
    printk("****************** mcpb (%p) ************************\n", mcpb);  
    _dump_data(mcpb->data, mcpb->len);                      
    printk("\n\n");                                
}



/*---------------------------------------------------------------------- 
 * Func : dump_mcpb_tail 
 *
 * Desc : dump tail data in mcp buffer
 *
 * Parm : mcpb : mcp buffer 
 *
 * Retn : N/A
 *----------------------------------------------------------------------*/ 
void dump_mcpb_tail(mcp_buff* mcpb)
{    
    printk("****************** mcpb (%p) ************************\n", mcpb);  
    _dump_data(mcpb->tail, mcpb_tailroom(mcpb));                      
    printk("\n\n");                                
}
