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
#include <asm/mach-venus/mcp/mcp_cipher.h>
#include <asm/mach-venus/mcp/mcp_aux.h>


#define MCP_CIPHER_FILE_NAME       "mcp/cipher"
static struct cdev mcp_cdev;
static dev_t devno;


#define MCP_CIPHER_IOCTL_CMD(x)         (0x12839000 + x)
#define MCP_CIPHER_IOCTL_INIT           MCP_CIPHER_IOCTL_CMD(0)
#define MCP_CIPHER_IOCTL_UPDATE         MCP_CIPHER_IOCTL_CMD(1)
#define MCP_CIPHER_IOCTL_FINAL          MCP_CIPHER_IOCTL_CMD(2)


typedef struct{
    unsigned char   type;                   
    unsigned char   enc;   
    unsigned char   key_len;
    unsigned char*  key;
    unsigned char   iv_len;
    unsigned char*  iv;
}MCP_CIPHER_PARAM;


typedef struct{
    unsigned char*  in;
    unsigned long   len;
    unsigned char*  out;    
}MCP_CIPHER_DATA;


#define MCP_CIPHER_DES_ECB              0x00
#define MCP_CIPHER_DES_CBC              0x01
#define MCP_CIPHER_TDES_ECB             0x10
#define MCP_CIPHER_TDES_CBC             0x11
#define MCP_CIPHER_AES_128_ECB          0x20
#define MCP_CIPHER_AES_128_CBC          0x21


    
/*------------------------------------------------------------------
 * Func : mcp_cipher_do_update
 *
 * Desc : open function of md dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int mcp_cipher_do_init(
    MCP_CIPHER_CTX*         ctx, 
    MCP_CIPHER_PARAM*       param
    )
{     
    MCP_CIPHER* cipher = NULL;        
    unsigned char buff[MAX_CIPHER_KEY_SIZE + MAX_CIPHER_IV_SIZE];
    
    switch (param->type) 
    {                                 
    case MCP_CIPHER_AES_128_ECB:    cipher = MCP_aes_128_ecb();     break;        
    case MCP_CIPHER_AES_128_CBC:    cipher = MCP_aes_128_cbc();     break;    
    case MCP_CIPHER_DES_ECB:        cipher = MCP_des_ecb();         break;        
    case MCP_CIPHER_DES_CBC:        cipher = MCP_des_cbc();         break;        
    case MCP_CIPHER_TDES_ECB:       cipher = MCP_tdes_ecb();        break;        
    case MCP_CIPHER_TDES_CBC:       cipher = MCP_tdes_cbc();        break;                                    
    default:    
        printk("[MCP][CIPHER] do cipher init failed, unknow cipher\n");            
        return -EFAULT;
    }  
    
    memset(buff, 0, sizeof(buff));        
    
    if (param->key && param->key_len)
    {        
        if (param->key_len > MAX_CIPHER_KEY_SIZE)
            param->key_len = MAX_CIPHER_KEY_SIZE;
            
        if (copy_from_user((void *) buff, (void __user *)param->key, param->key_len))    
        {
            printk("[MCP][CIPHER] do cipher init failed, copy key from user space failed\n");            
            return -EFAULT;
        }                  
        param->key = buff;
    }        
    else
    {        
        param->key = NULL;
    }        
    
    if (param->iv && param->iv_len)
    {        
        if (param->iv_len > MAX_CIPHER_IV_SIZE)
            param->iv_len = MAX_CIPHER_IV_SIZE;
            
        if (copy_from_user((void *) &buff[MAX_CIPHER_KEY_SIZE], (void __user *)param->iv, param->iv_len))    
        {
            printk("[MCP][CIPHER] do cipher init failed, copy iv from user space failed\n");            
            return -EFAULT;
        }                  
                         
        param->iv = &buff[MAX_CIPHER_KEY_SIZE];
    }        
    else
    {        
        param->iv = NULL;
    }        
                                    
    return MCP_CipherInit(ctx, cipher, param->key, param->iv, param->enc);        
}

    
    
/*------------------------------------------------------------------
 * Func : mcp_cipher_do_update
 *
 * Desc : open function of md dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int mcp_cipher_do_update(
    MCP_CIPHER_CTX*         ctx, 
    unsigned char*          in,
    unsigned long           len,
    unsigned char*          out
    )
{       
    int outlen;
    unsigned char* buff = NULL;    
    
    if (mcp_is_pli_address(in) && mcp_is_pli_address(out))
        return MCP_CipherUpdate(ctx, pli_to_kernel(in), len, pli_to_kernel(out));        
        
    if ((buff = mcp_malloc(len))==NULL)
        goto err_alloc_kernel_buffer_failed;
            
    if (mcp_is_pli_address(in))               // input buffer is allocated by pli
    {
        if ((outlen = MCP_CipherUpdate(ctx, pli_to_kernel(in), len, buff))<0)
        {
            printk("[MCP][CIPHER] do cipher update failed, update failed\n");
            goto err_update_cipher_failed;
        }
        
        if (copy_to_user((void __user *) out, buff, outlen))    
        {
            printk("[MCP][CIPHER] do cipher update failed, copy data to user buffer failed\n");            
            goto err_copy_to_user_failed;
        }
    }
    else if (mcp_is_pli_address(out))         // output buffer is allocated by pli
    {
        if (copy_from_user((void *) buff, (void __user *)in, len))    
        {
            printk("[MCP][CIPHER] do cipher update failed, copy data from user space failed\n");            
            goto err_copy_from_user_failed;
        }           
        
        if ((outlen = MCP_CipherUpdate(ctx, buff, len, pli_to_kernel(out)))<0)
        {            
            printk("[MCP][CIPHER] do cipher update failed, update failed\n");
            goto err_update_cipher_failed;
        }
    }
    else                                // none of them are allocated by pli
    {
        if (copy_from_user((void *) buff, (void __user *)in, len))    
        {
            printk("[MCP][CIPHER] do cipher update failed, copy data from user space failed\n");
            outlen = -EFAULT;
            goto err_copy_from_user_failed;
        }           
        
        if ((outlen = MCP_CipherUpdate(ctx, buff, len, buff))<0)
        {            
            printk("[MCP][CIPHER] do cipher update failed, update failed\n");
            goto err_update_cipher_failed;
        }
        
        if (copy_to_user((void __user *) out, buff, outlen))    
        {
            printk("[MCP][CIPHER] do cipher update failed, copy data to user buffer failed\n");            
            goto err_copy_to_user_failed;
        }          
    }  
    
    mcp_free(buff, len);   

    return outlen;
    
//----------------------------------    
err_copy_from_user_failed:
err_copy_to_user_failed:        
err_update_cipher_failed:    
    mcp_free(buff, len);       
err_alloc_kernel_buffer_failed:        
    return -EFAULT;
}

            
    
/*------------------------------------------------------------------
 * Func : mcp_cipher_do_final
 *
 * Desc : open function of md dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int mcp_cipher_do_final(
    MCP_CIPHER_CTX*         ctx, 
    unsigned char*          out
    )
{           
    int ret;
    unsigned char buff[MAX_CIPHER_BLOCK_SIZE];
    
    if (mcp_is_pli_address(out))
    {        
        return MCP_CipherFinal(ctx, pli_to_kernel(out));        
    }        
    else
    {                
        if ((ret = MCP_CipherFinal(ctx, buff))> 0)
        {
            if (copy_to_user((void __user *) out, buff, ret))    
                return -EFAULT;                
        }
        
        return ret;    
    }
}

            


/***************************************************************************
     ------------------- Device File Operations  --------------------
****************************************************************************/

/*------------------------------------------------------------------
 * Func : mcp_cipher_open
 *
 * Desc : open function of md dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int mcp_cipher_open(struct inode *inode, struct file *file)
{       
    file->private_data = kmalloc(sizeof(MCP_CIPHER_CTX),GFP_KERNEL);
    
    if (file->private_data==NULL)
        return -EFAULT;
    
    MCP_CIPHER_CTX_init((MCP_CIPHER_CTX*) file->private_data);
     
    return 0;
}



/*------------------------------------------------------------------
 * Func : mcp_cipher_release
 *
 * Desc : release function of mcp dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int mcp_cipher_release(
    struct inode*           inode, 
    struct file*            file
    )
{               
    MCP_CIPHER_CTX* ctx = (MCP_CIPHER_CTX*) file->private_data;  
    
    if (ctx)
    {        
        MCP_CIPHER_CTX_cleanup(ctx);
        kfree(ctx);
    }        
    
	return 0;
}



/*------------------------------------------------------------------
 * Func : mcp_cipher_ioctl
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
int mcp_cipher_ioctl(
    struct inode*           inode, 
    struct file*            file,
    unsigned int            cmd, 
    unsigned long           arg
    )
{              
    MCP_CIPHER_CTX*   ctx = (MCP_CIPHER_CTX*) file->private_data;              
    MCP_CIPHER_PARAM  param;
    MCP_CIPHER_DATA   data;
    
	switch (cmd)		
    {        
    case MCP_CIPHER_IOCTL_INIT:           
        
        if (copy_from_user((void __user *) &param, (void __user *)arg, sizeof(MCP_CIPHER_PARAM)))
        {            
            printk("[MCP][CIPHER] WARNING, do cipher init failed, copy cipher data from user failed\n");
			return -EFAULT;
        }                                  
         
        return mcp_cipher_do_init(ctx, &param);                          
        
    case MCP_CIPHER_IOCTL_UPDATE:
                                
        if (copy_from_user((void __user *) &data, (void __user *)arg, sizeof(MCP_CIPHER_DATA)))
        {            
            printk("[MCP][CIPHER] WARNING, do cipher update failed, copy cipher data from user failed\n");
			return -EFAULT;
        }                

        return mcp_cipher_do_update(ctx, data.in, data.len, data.out);
        
        
    case MCP_CIPHER_IOCTL_FINAL:
                       
        return mcp_cipher_do_final(ctx, (unsigned char*)arg);
                       
	default:		
	    printk("[MCP] WARNING, unknown command\n");                
		return -EFAULT;          
	}	           
}



static struct file_operations mcp_ops = 
{
	.owner		= THIS_MODULE,	
	.ioctl		= mcp_cipher_ioctl,
	.open		= mcp_cipher_open,
	.release	= mcp_cipher_release,
};




/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/

static int __init mcp_cipher_init(void)
{    
    cdev_init(&mcp_cdev, &mcp_ops);            
                
    if (alloc_chrdev_region(&devno, 0, 1, MCP_CIPHER_FILE_NAME)!=0)    
    {
        cdev_del(&mcp_cdev);
        return -EFAULT;
    }                                 
    
    if (cdev_add(&mcp_cdev, devno, 1)<0)
        return -EFAULT;                          
                      
    devfs_mk_cdev(devno, S_IFCHR|S_IRUSR|S_IWUSR, MCP_CIPHER_FILE_NAME);         
    
    return 0;	
}


static void __exit mcp_cipher_exit(void)
{    
    cdev_del(&mcp_cdev);
    devfs_remove(MCP_CIPHER_FILE_NAME);
    unregister_chrdev_region(devno, 1);
}



module_init(mcp_cipher_init);
module_exit(mcp_cipher_exit);
