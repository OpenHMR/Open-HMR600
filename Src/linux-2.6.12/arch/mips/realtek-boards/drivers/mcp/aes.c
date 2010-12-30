#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/mach-venus/mcp.h>
#include <asm/mach-venus/mcp/mcp_cipher.h>
#include <asm/mach-venus/mcp/mcp_dbg.h>
#include "aes.h"




/*------------------------------------------------------------------
 * Func : AES_Init
 *
 * Desc : Initial MCP Digest
 *
 * Parm : ctx  : context of md ctx 
 *        key  : key
 *        iv   : initial vector
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
int AES_Init(
    MCP_CIPHER_CTX*         ctx,
    const unsigned char*    key,
    const unsigned char*    iv, 
    int                     mode,
    int                     enc
    )
{             
    AES_CTX* p_aes = (AES_CTX*) ctx->cipher_data;  
        
    memset(p_aes, 0, sizeof(AES_CTX));         
    
    switch(mode) 
    {
    case MCP_CIPH_ECB_MODE: 
        p_aes->mode = MCP_BCM_ECB;  
        break;
        
    case MCP_CIPH_CBC_MODE: 
        p_aes->mode = MCP_BCM_CBC;  
        break;        
        
    default:        
        return -1;    
    }
    
    p_aes->enc = (enc) ? 1 : 0;
        
    if (key) 
    {                    
        p_aes->key = p_aes->buff[0];        
        memcpy(p_aes->buff[0], key, 16);        
    }    
            
    if (iv) 
    {
        p_aes->iv = p_aes->buff[1];        
        memcpy(p_aes->buff[1], iv, 16);        
    }   
    else
    {
        p_aes->iv = p_aes->buff[1];        
    }         

    return 0;
}



 /*------------------------------------------------------------------
 * Func : AES_DoCipher
 *
 * Desc : Do Cipher
 *
 * Parm : ctx       : context of cipher
 *        in        : input data buffer
 *        len       : size of input data buffer
 *        out       : output data buffer
 *         
 * Retn : output data length
 *------------------------------------------------------------------*/
int AES_DoCipher(
    MCP_CIPHER_CTX*         ctx,
    unsigned char*          in,           
    unsigned long           len,
    unsigned char*          out
    )
{       
    AES_CTX* p_aes = (AES_CTX*) ctx->cipher_data;        
    unsigned char iv[16];
    
    len &= ~0xF;
        
    if (len)
    {        
        if (p_aes->enc)
        {
            MCP_AES_Encryption(p_aes->mode, p_aes->key, p_aes->iv, in, out, len);                                                 
                                
            if (p_aes->mode==MCP_BCM_CBC)        
                memcpy(p_aes->iv, out + len - 16, 16);      // save iv        
        }
        else
        {
            memcpy(iv, p_aes->iv, 16);
         
            if (p_aes->mode==MCP_BCM_CBC)        
                memcpy(p_aes->iv, in + len - 16, 16);      // save iv        

            MCP_AES_Decryption(p_aes->mode, p_aes->key, iv, in, out, len);
        }        
    }
        
    return len;
}



/*------------------------------------------------------------------
 * Func : AES_Cleanup
 *
 * Desc : Finished MCP Digest and output the remain data
 *
 * Parm : ctx       : context of cipher ctx
 *        out       : digest output 
 *         
 * Retn : 0 for success, others failed
 *------------------------------------------------------------------*/
int AES_Cleanup(MCP_CIPHER_CTX* ctx) 
{
    // do nothing
    return 0;
}



/***************************************************************************
     ECB mode
****************************************************************************/


/*------------------------------------------------------------------
 * Func : AES_128_ECB_Init
 *
 * Desc : Initial AES 128 ECB Cipher
 *
 * Parm : ctx  : context of md ctx 
 *        key  : key
 *        iv   : initial vector
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
int AES_128_ECB_Init(
    MCP_CIPHER_CTX*         ctx, 
    const unsigned char*    key, 
    const unsigned char*    iv, 
    int                     enc
    )
{               
    return AES_Init(ctx, key, NULL, MCP_CIPH_ECB_MODE, enc);
}


static const MCP_CIPHER aes_128_ecb = 
{
    .block_size = 16,      
    .iv_len     = 0,    
    .ctx_size   = sizeof(AES_CTX),    
    .init       = AES_128_ECB_Init,
    .do_cipher  = AES_DoCipher,
    .cleanup    = AES_Cleanup,    
};



MCP_CIPHER *MCP_aes_128_ecb(void)
{
    return (MCP_CIPHER*) &aes_128_ecb;
}


/***************************************************************************
     CBC mode
****************************************************************************/


/*------------------------------------------------------------------
 * Func : AES_128_CBC_Init
 *
 * Desc : Initial AES 128 CBC Cipher
 *
 * Parm : ctx  : context of md ctx 
 *        key  : key
 *        iv   : initial vector
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
int AES_128_CBC_Init(
    MCP_CIPHER_CTX*         ctx,
    const unsigned char*    key,        
    const unsigned char*    iv,        
    int                     enc
    )
{               
    return AES_Init(ctx, key, iv, MCP_CIPH_CBC_MODE, enc);    
}


static const MCP_CIPHER aes_128_cbc = 
{    
    .block_size = 16,      
    .iv_len     = 16,    
    .ctx_size   = sizeof(AES_CTX),    
    .init       = AES_128_CBC_Init,
    .do_cipher  = AES_DoCipher,
    .cleanup    = AES_Cleanup,    
};


MCP_CIPHER *MCP_aes_128_cbc(void)
{
    return (MCP_CIPHER*) &aes_128_cbc;
}




/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/

#if 0



static void AES_CipherTest(void)
{   
    MCP_CIPHER* cipher = MCP_aes_128_cbc(); 
    MCP_CIPHER_CTX  ctx;        
                   
    unsigned char key[16] =  {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
    };         
    
    unsigned char iv[16] =  {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };                     
    
    unsigned char buff[1024+8];     
    unsigned char* in  = buff;
    unsigned char* out = buff;
    int step = 8;
    int outl = 0;
    int i;
    
    memset(buff, 0x33, sizeof(buff));               
              
    mcp_dump_data_with_text(buff, sizeof(buff), "Source Data : "); 
    
    ////////////////////////////////////////////////////////////////////
                
    MCP_CIPHER_CTX_init(&ctx);

    MCP_CipherInit(&ctx, cipher, key, iv, 1);                                
                                
    for (i=0; i < sizeof(buff); i+= step)
    {                                        
        outl += MCP_CipherUpdate(&ctx, &in[i], step, &out[outl]);        
    }            

    outl += MCP_CipherFinal(&ctx, &out[outl]);

    MCP_CIPHER_CTX_cleanup(&ctx);
                                
    mcp_dump_data_with_text(out, outl, "Encrypted Data : ");    
    
    ////////////////////////////////////////////////////////////////////            
        
    MCP_CIPHER_CTX_init(&ctx);
        
    MCP_CipherInit(&ctx, cipher, key, iv, 0);     
    
    outl = 0;                           

    for (i=0; i < sizeof(buff); i+= step)
    {                                        
        outl += MCP_CipherUpdate(&ctx, &in[i], step, &out[outl]);        
    }  
         
    outl += MCP_CipherFinal(&ctx, &out[outl]);            

    MCP_CIPHER_CTX_cleanup(&ctx);
                           
    mcp_dump_data_with_text(out, outl, "Decrypted Data : ");                        
    
    ////////////////////////////////////////////////////////////////////    
}



/*------------------------------------------------------------------
 * Func : mcp_aesh_module_init
 *
 * Desc : mcp module init function
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static int __init mcp_aes_module_init(void)
{   
    AES_CipherTest();    
    return 0;        
}


module_init(mcp_aes_module_init);

#endif
