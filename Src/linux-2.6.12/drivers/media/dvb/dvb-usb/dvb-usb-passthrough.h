/* dvb-usb-passthrough.h is part of the DVB USB library.
 *
 * Copyright (C) 2010 kevin_wang (kevin_wang@realtek.com.tw) 
 *
 * This file contains functions for DVB USB direct ts passthrough 
 */
 
#ifndef __DVB_USB_PASSTHROUGH_H__
#define __DVB_USB_PASSTHROUGH_H__

#include <linux/spinlock.h> 

#define STREAMER_INIT_RDY               0x01
#define STREAMER_BUFFER_RDY             0x02
#define STREAMER_IS_RUNING              0x04

struct dvb_usb_streamer
{                    
    spinlock_t              lock;
    unsigned char           state;   
    dvb_passthrough_mode    mode;     
    struct dvb_usb_device*  d;
    int                     urb_size;    
    
    // data buffer    
    unsigned char*      p_base;  
    unsigned char*      p_limit;  
    unsigned char*      p_rp;  
    unsigned char*      p_wp;  
    unsigned char*      p_urb_wp;  
};

#define init_streamer_lock(streamer)    spin_lock_init(&streamer->lock)
#define lock_streamer(streamer)         spin_lock(&streamer->lock)
#define unlock_streamer(streamer)       spin_unlock(&streamer->lock)

void dvb_usb_streamer_init(struct dvb_usb_streamer* streamer, struct dvb_usb_device* d);
void dvb_usb_streamer_uninit(struct dvb_usb_streamer* streamer);
int  dvb_usb_streamer_control(struct dvb_usb_streamer* streamer, dvb_passthrough_cmd cmd, dvb_passthrough_mode mode);
int  dvb_usb_streamer_set_buffer(struct dvb_usb_streamer* streamer, unsigned char* pbuff, unsigned long size); 
int  dvb_usb_streamer_read_data(struct dvb_usb_streamer* streamer, unsigned char** ppbuff, unsigned long* psize); 
int  dvb_usb_streamer_release_data(struct dvb_usb_streamer* streamer, unsigned char* pbuff, unsigned long size);




#endif  //__DVB_USB_PASSTHROUGH_H__


