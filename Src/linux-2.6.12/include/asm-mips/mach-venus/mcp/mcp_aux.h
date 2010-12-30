#ifndef __MCP_AUX_H__
#define __MCP_AUX_H__

#include <asm/mach-venus/mcp.h>

int mcp_is_pli_address(void* addr);

#define pli_to_kernel(addr)           ((void*) (((u32)addr & 0x3FFFFFFF) | 0x80000000))

#endif //__MCP_AUX_H__ 
