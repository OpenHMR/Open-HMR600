#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>                  
#include <asm/page.h>
#include <platform.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/mach-venus/mcp/mcp_dgst.h>
#include <asm/mach-venus/mcp/mcp_aux.h>

#define MCP_DEV_FILE_NAME      "mcp/dgst"

static struct cdev mcp_cdev;
static dev_t devno;


#define MCP_DGST_IOCTL_CMD(x)         (x)
#define MCP_DGST_IOCTL_INIT           MCP_DGST_IOCTL_CMD(0)
#define MCP_DGST_IOCTL_UPDATE         MCP_DGST_IOCTL_CMD(1)
#define MCP_DGST_IOCTL_FINAL          MCP_DGST_IOCTL_CMD(2)
#define MCP_DGST_IOCTL_PEEK           MCP_DGST_IOCTL_CMD(3)


/***************************************************************************
     ------------------- Device File Operations  --------------------
****************************************************************************/

/*------------------------------------------------------------------
 * Func : mcp_dgst_dev_open
 *
 * Desc : open function of md dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int mcp_dgst_dev_open(struct inode *inode, struct file *file)
{       
    file->private_data = kmalloc(sizeof(MCP_MD_CTX),GFP_KERNEL);
    
    if (file->private_data==NULL)
        return -EFAULT;
    
    MCP_MD_CTX_init((MCP_MD_CTX*) file->private_data);
    
    ((MCP_MD_CTX*) file->private_data)->private_data = NULL;
     
    return 0;
}



/*------------------------------------------------------------------
 * Func : mcp_dgst_dev_release
 *
 * Desc : release function of mcp dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int mcp_dgst_dev_release(
    struct inode*           inode, 
    struct file*            file
    )
{               
    MCP_MD_CTX* ctx = (MCP_MD_CTX*) file->private_data;  
    
    if (ctx)
    {        
        if (ctx->private_data)
            free_mcpb((MCP_BUFF*)ctx->private_data);                               
                    
        MCP_MD_CTX_cleanup(ctx);
        kfree(ctx);
    }        
    
	return 0;
}



/*------------------------------------------------------------------
 * Func : mcp_dgst_dev_ioctl
 *
 * Desc : ioctl function of md dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *        cmd   : control command
 *        arg   : arguments
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int mcp_dgst_dev_ioctl(
    struct inode*           inode, 
    struct file*            file,
    unsigned int            cmd, 
    unsigned long           arg
    )
{              
    MCP_MD_CTX*   ctx = (MCP_MD_CTX*) file->private_data;  
    MCP_BUFF      mcpb;   
    MCP_BUFF*     kmcpb; 
    unsigned char hash[MCP_MAX_DIGEST_SIZE];
    unsigned int  hash_len;    
    int           ret = -EFAULT;
    
	switch (cmd)		
    {        
    case MCP_DGST_IOCTL_INIT:           
                        
        switch(arg) {
        case MCP_MD_AES_H:      return MCP_DigestInit(ctx, MCP_aes_h()); 
        case MCP_MD_MARS_AES_H: return MCP_DigestInit(ctx, MCP_mars_aes_h());
        case MCP_MD_SHA1:       return MCP_DigestInit(ctx, MCP_sha1());
        case MCP_MD_MARS_SHA1:  return MCP_DigestInit(ctx, MCP_mars_sha1());
        }                             
        break;
        
    case MCP_DGST_IOCTL_UPDATE:
                                
        if (copy_from_user((void __user *) &mcpb, (void __user *)arg, sizeof(MCP_BUFF)))
        {            
            printk("[MCP][DGST] WARNING, do dgst update failed, copy data from user failed\n");
			return -EFAULT;
        }                
        
        // check is the MCP_buff is valid or not                            
        
        if (mcp_is_pli_address(mcpb.head))        
        {                		    
            mcpb.head = pli_to_kernel(mcpb.head);
            mcpb.data = pli_to_kernel(mcpb.data);
            mcpb.tail = pli_to_kernel(mcpb.tail);
            mcpb.end  = pli_to_kernel(mcpb.end);
            
            ret = MCP_DigestUpdate(ctx, &mcpb);                	       
        }
        else
        {
            kmcpb = (MCP_BUFF*) ctx->private_data;                        
            
            if (kmcpb)
            {
                mcpb_purge(kmcpb);
                                
                if (mcpb_tailroom(kmcpb) < mcpb.len)
                {                                        
                    if (kmcpb)
                        free_mcpb(kmcpb);
                                                                                                                   
                    kmcpb = alloc_mcpb(mcpb.len);                
                    
                    ctx->private_data = (void*) kmcpb;
                }                                                                                    
            }
            else
            {
                 kmcpb = alloc_mcpb(mcpb.len);
                 ctx->private_data = (void*) kmcpb;
            }                
            
            if (kmcpb==NULL)
            {            
                printk("[MCP][DGST] WARNING, do dgst update failed, alloc mcpb buffer failed\n");
			    return -EFAULT;
            }                                     
                        
            if (copy_from_user((void *) kmcpb->data, (void __user *)mcpb.data, mcpb.len)==0)    
            {
                mcpb_put(kmcpb, mcpb.len);
                ret = MCP_DigestUpdate(ctx, kmcpb);            
            }                                                      
        }
        
        return ret;                 
        
        
    case MCP_DGST_IOCTL_FINAL:
        
        if (MCP_DigestFinal(ctx, hash, &hash_len)<0)
            return -EFAULT;
                
        return (copy_to_user((unsigned char __user *)arg, hash, hash_len)<0) ? -EFAULT : hash_len;
        
    case MCP_DGST_IOCTL_PEEK:                            
        
        if (MCP_DigestPeek(ctx, hash, &hash_len)<0)
            return -EFAULT;
        
        return (copy_to_user((unsigned char __user *)arg, hash, hash_len)<0) ? -EFAULT : hash_len;          
                         
	default:		
	    printk("[MCP][DGST] WARNING, unknown command\n");                
		return -EFAULT;          
	}
	
    return ret;          
}



static struct file_operations mcp_dgst_dev_ops = 
{
	.owner		= THIS_MODULE,	
	.ioctl		= mcp_dgst_dev_ioctl,
	.open		= mcp_dgst_dev_open,
	.release	= mcp_dgst_dev_release,
};




/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/

static int __init mcp_dgst_dev_init(void)
{    
    cdev_init(&mcp_cdev, &mcp_dgst_dev_ops);            
                
    if (alloc_chrdev_region(&devno, 0, 1, MCP_DEV_FILE_NAME)!=0)    
    {
        cdev_del(&mcp_cdev);
        return -EFAULT;
    }                                 
    
    if (cdev_add(&mcp_cdev, devno, 1)<0)
        return -EFAULT;                          
                      
    devfs_mk_cdev(devno, S_IFCHR|S_IRUSR|S_IWUSR, MCP_DEV_FILE_NAME);         
    
    return 0;	
}


static void __exit mcp_dgst_dev_exit(void)
{    
    cdev_del(&mcp_cdev);
    devfs_remove(MCP_DEV_FILE_NAME);
    unregister_chrdev_region(devno, 1);
}



module_init(mcp_dgst_dev_init);
module_exit(mcp_dgst_dev_exit);
