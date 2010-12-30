#ifndef __AES_H__H__
#define __AES_H__H__

#include <asm/mach-venus/mcp/mcp.h>

typedef struct {    
    mcp_buff*               tmp;
    unsigned long long      byte_count;
    unsigned char           hash[16];
}AES_H_CTX;

#endif // __AES_H__H__ 
