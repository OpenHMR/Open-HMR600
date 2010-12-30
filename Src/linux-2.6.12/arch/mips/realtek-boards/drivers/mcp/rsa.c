#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/mach-venus/mcp/mcp.h>




#define BI_MAX_BYTE_LEN         (BI_MAXLEN * sizeof (unsigned int))



/*------------------------------------------------------------------
 * Func : rsa_strip
 *
 * Desc : strip rsa header 
 *
 * Parm : bi  : bi 
 *         
 * Retn : 0 if successed.
 *------------------------------------------------------------------*/
int rsa_strip(
    BI*                     bi
    )
{               
    unsigned char*  ptr = (unsigned char*) bi->m_ulValue;     
    int header_start = -1;
    int header_tail  = -1;                
    int i = bi->m_nLength * sizeof(unsigned int);        
    
    // looking for header
    for(; i>=0 && header_start < 0; i--)
    {     
        switch(ptr[i])
        {
        case 0x1F:
        case 0x01:
            header_start = i;     // header detected
            break;            
            
        case 0x00:
            continue;
            
        default:                                                
            printk("[MCP][RSA] strip rsa header failed - no rsa padding header detected\n");        
            return -1;
        }
    }
    
    if (header_start<0)
    {        
        printk("[MCP][RSA] strip rsa header failed - no rsa padding header detected\n");        
        return -1;
    }
    
    // looking for tail
    for(; i>=0 && header_tail < 0; i--)
    {
        switch(ptr[i])
        {        
        case 0x00:
            header_tail = i;     // header detected
            break;    
            
        case 0xFF:            
            continue;
            
        default:                                                            
            printk("[MCP][RSA] strip rsa header failed - no rsa padding tail detected\n");        
            return -1;
        }
    }
    
    if (header_tail<0)
    {        
        printk("[MCP][RSA] strip rsa header failed- no rsa padding tail detected\n");        
        return -1;
    }        
    
    // strip data    
    memset(&ptr[header_tail], 0, header_start - header_tail);
        
    bi->m_nLength = header_tail / sizeof(unsigned int);
    if (header_tail % sizeof(unsigned int))
        bi->m_nLength++;    
        
    return 0;        
}    



/*------------------------------------------------------------------
 * Func : rsa_verify
 *
 * Desc : do verify rsa. In this operation, we fixed the public key to 
 *        65537
 *
 * Parm : signature  : signature
 *        pub_exp    : public exponent
 *        mod        : modulous
 *        dgst       : digest
 *         
 * Retn : 0 if successed.
 *------------------------------------------------------------------*/
int rsa_verify(BI *signature, BI* pub_exp, BI* mod, BI *dgst)
{
	BI *res;	

	if ((signature == NULL) || (pub_exp == NULL) || (mod == NULL) || (dgst == NULL)) {
		printk("[MCP][RSA] do rsa verify failed - no enugh arguments\n");        
		return -1;
	}
	
    //dump_bi(signature);
    //dump_bi(pub_exp);
    //dump_bi(dgst);

	res = Exp_Mod(signature, pub_exp, mod);      // compute data
	rsa_strip(res);	
    //dump_bi(res);
    memcpy(dgst, res, sizeof(BI));			
	free_BI(res);
	return 0;
}



#if 0       // for test only

/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/


static void RSATest(void)
{   
	char modulus_str[] = "ce812d6f1979ed7839b895d4b4b034319a630213b4b4528a0100f6ef961ffd050c775e9e9ebc9ca09d422c948d63698acf3f7c3615eb2cceac2c46bdfc8016c1988803ebdd5177ade1195c5caa6630ab2c8708e9cd430aedba92b26ba52e18e4e1b6c4f429dfa53c20c7b756f9622beb40c00e481e1dcdf0afa5e83ca7e4e443";	
	char signature_str[] = "c70bf6a79679af36ca72b7f6a9ce3b7a669860e22dffbcd6fe3075fab07e0ba6eb68f620653ba17144c431d9fa527f40ba5c83cf6b4154ae4ff9e604a2b74d8f4fb2f57a9a190ce3916e6176dbfb045485ad05109cc6ae7423900e12ccdc6dbd76e6e094e94543609c0427527ca7fa5fe5928f86053e05c419cd52cea7573596";
	char digest_str[]    = "026946989674B77821CE9EBCB0F66445ACC6A154";
    BI *signature, *pub_exp, *modulous, *res;
    
    printk("signature: %s\n", signature_str);
    printk("original checksum: %s\n", digest_str);
    
    // Note: public exponent is always 65537
	signature = InPutFromStr(signature_str, HEX);
	pub_exp   = move_p(65537);
	modulous  = InPutFromStr(modulus_str, HEX);
    res       = init_BI();	
	
	rsa_verify(signature, pub_exp, modulous, res);
	
	printk("recovered checksum:");
	dump_bi(res);	
		
	free_BI(signature);
	free_BI(pub_exp);
	free_BI(modulous);
	free_BI(res);
	
	/*
	BI* signature = move_p(65537);
	BI* pub_exp   = move_p(3);	
	BI* res       = NULL;
	printk("--------------start mod--------------\n");
	res =  Mod(signature, pub_exp);
	printk("--------------stop mode--------------\n");	
	free_BI(signature);
	free_BI(pub_exp);
	free_BI(res);		 
	*/
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
static int __init mcp_rsa_module_init(void)
{   
    RSATest();    
    return 0;        
}


module_init(mcp_rsa_module_init);

#endif
