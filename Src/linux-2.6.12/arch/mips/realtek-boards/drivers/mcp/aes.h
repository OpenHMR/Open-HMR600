#ifndef __AES_H__
#define __AES_H__

typedef struct {    
    unsigned char       mode;
    unsigned char       enc;
    unsigned char*      key;
    unsigned char*      iv;
    unsigned char       buff[2][16];
}AES_CTX;


#endif
