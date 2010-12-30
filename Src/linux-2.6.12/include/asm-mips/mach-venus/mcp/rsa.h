#ifndef __RSA_H__
#define __RSA_H__

#include <asm/mach-venus/mcp/bi.h>

int rsa_verify(BI *signature, BI* pub_exp, BI* mod, BI *dgst);

#endif // __RSA_H__