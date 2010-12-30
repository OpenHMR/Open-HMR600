/* dvb-usb-passthrough.c is part of the DVB USB library.
 *
 * Copyright (C) 2010 kevin_wang (kevin_wang@realtek.com.tw) 
 *
 * This file contains functions for DVB USB direct ts passthrough 
 */
#include "dvb-usb-common.h"
#include "dvb-usb-passthrough.h"


static void dvb_usb_streamer_urb_complete(struct urb *urb, struct pt_regs *ptregs);

#define dvb_streamer_dbg(fmt, args...)       printk("[dvb usb streamer] dubeg, " fmt, ## args)  
#define dvb_streamer_warning(fmt, args...)   printk("[dvb usb streamer] warning, " fmt, ## args)  

//#define DEBUG_STREAMER_URB
#ifdef DEBUG_STREAMER_URB
#define dvb_streamer_urb_dbg                  dvb_streamer_dbg
#else
#define dvb_streamer_urb_dbg(args...) 
#endif

//#define DEBUG_STREAMER_READ
#ifdef DEBUG_STREAMER_READ
#define dvb_streamer_read_dbg                  dvb_streamer_dbg
#else
#define dvb_streamer_read_dbg(args...) 
#endif






/*----------------------------------------------------------
 * Func : dvb_usb_streamer_setup_urb
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
static
void dvb_usb_streamer_setup_urb(
    struct dvb_usb_streamer*    streamer,
    struct urb*                 urb
    )
{        
    struct dvb_usb_device *d = streamer->d;           
    
    usb_fill_bulk_urb(
                urb, 
                d->udev,
                usb_rcvbulkpipe(d->udev,d->props.urb.endpoint),
                streamer->p_urb_wp,
    		    streamer->urb_size,
    		    dvb_usb_streamer_urb_complete,
    		    streamer);
    		    
    urb->transfer_flags = 0;          		    
    		    
    streamer->p_urb_wp += streamer->urb_size;
        
    if (streamer->p_urb_wp >= streamer->p_limit)
        streamer->p_urb_wp = streamer->p_base;

    dvb_streamer_urb_dbg("urb, buf = %p, size = %d\n", urb->transfer_buffer, urb->transfer_buffer_length);     
}



/*----------------------------------------------------------
 * Func : dvb_usb_streamer_setup_urb
 *
 * Desc : 
 *
 * Parm : 
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
static void dvb_usb_streamer_urb_complete(struct urb *urb, struct pt_regs *ptregs)
{    
	struct dvb_usb_streamer *streamer = urb->context;	

    
    dvb_streamer_urb_dbg(" urb complete, buf = %p, size = %d/%d, status=%d\n", 
        urb->transfer_buffer, 
        urb->actual_length,
        urb->transfer_buffer_length,
        urb->status);        
    
	switch (urb->status) {
	case 0:             /* success */
	case -ETIMEDOUT:    /* NAK */
		break;
	case -ECONNRESET:   /* kill */
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	default:        /* error */
		deb_ts("urb completition error %d.", urb->status);
		break;
	}
	
	lock_streamer(streamer);
	
	streamer->p_wp = urb->transfer_buffer + urb->transfer_buffer_length;     // update wp

	if (streamer->p_wp >= streamer->p_limit)
	    streamer->p_wp = streamer->p_base;				
	           
    if (!(streamer->state & STREAMER_IS_RUNING))
    {
        unlock_streamer(streamer);		
    }        
    else
    {         
        urb->transfer_buffer = streamer->p_urb_wp;
	
	    urb->transfer_flags = 0; 		
	
	    //dvb_streamer_urb_dbg("reload urb, buf = %p, size = %d\n", urb->transfer_buffer, urb->transfer_buffer_length);
	
        streamer->p_urb_wp += streamer->urb_size;                
    
        if (streamer->p_urb_wp >= streamer->p_limit)
	        streamer->p_urb_wp = streamer->p_base;			
	    
	    unlock_streamer(streamer);
	            
    	usb_submit_urb(urb,GFP_ATOMIC);     // requeue urb
	}		
}


/*----------------------------------------------------------
 * Func : dvb_usb_streamer_start
 *
 * Desc : start streaming
 *
 * Parm : streamer  : handle of dvb streamer 
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
int dvb_usb_streamer_start(
    struct dvb_usb_streamer*    streamer,
    dvb_passthrough_mode        mode
    )
{
    struct dvb_usb_device *d = streamer->d;                   
    int i;
    
    lock_streamer(streamer);  
        
    if (streamer->state & STREAMER_IS_RUNING)
    {
        if (mode != streamer->mode)        
        {            
            i = streamer->urb_size;
            streamer->urb_size = ((d->props.caps & DVB_USB_HAS_PID_FILTER)&& (mode==SCANCHANNEL_MODE)) ? 512 : d->props.urb.u.bulk.buffersize;
            streamer->mode = mode;
            dvb_streamer_dbg("streaming mode changed, urb_size=%d-->%d\n", i, streamer->urb_size);      
        }                
        
        unlock_streamer(streamer);    
    }
    else        
    {                                          
        streamer->urb_size = ((d->props.caps & DVB_USB_HAS_PID_FILTER)&& (mode==SCANCHANNEL_MODE)) ? 512 : d->props.urb.u.bulk.buffersize;                
        
        streamer->p_rp     = streamer->p_base;
        streamer->p_wp     = streamer->p_base;
        streamer->p_urb_wp = streamer->p_base;   
        
        for (i = 0; i < d->urbs_initialized; i++)
            dvb_usb_streamer_setup_urb(streamer, d->urb_list[i]);
    
        streamer->state |= STREAMER_IS_RUNING;
        streamer->mode   = mode;        
        
        unlock_streamer(streamer);
                
        if (d->props.streaming_ctrl)
            d->props.streaming_ctrl(d, 1);                    
        
        dvb_usb_urb_submit(d);                
        
        dvb_streamer_dbg("streaming started, urb_size=%d\n", streamer->urb_size);
    }                    
            
    return 0;
}



/*----------------------------------------------------------
 * Func : dvb_usb_streamer_stop
 *
 * Desc : stop streaming
 *
 * Parm : streamer  : handle of dvb streamer 
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
int dvb_usb_streamer_stop(
    struct dvb_usb_streamer*    streamer    
    )
{
    struct dvb_usb_device *d = streamer->d;               
    
    lock_streamer(streamer);  
        
    if (streamer->state & STREAMER_IS_RUNING)
    {            
        streamer->state &= ~STREAMER_IS_RUNING;                                                           
        
        unlock_streamer(streamer);
        
        if (d->props.streaming_ctrl)
            d->props.streaming_ctrl(d, 0);             
    
        dvb_usb_urb_kill(d);             
        
        dvb_streamer_dbg("streaming stopped\n");                      
        
        return 0;
    }    
                
    unlock_streamer(streamer);    
            
    return 0;
}




/*----------------------------------------------------------
 * Func : dvb_usb_streamer_flush
 *
 * Desc : flush streaming
 *
 * Parm : streamer  : handle of dvb streamer 
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
int dvb_usb_streamer_flush(
    struct dvb_usb_streamer*    streamer    
    )
{        
    lock_streamer(streamer);  
        
    if (streamer->state & STREAMER_IS_RUNING)
    {                                                            
        unlock_streamer(streamer);
        
        dvb_usb_streamer_stop(streamer);
    
        lock_streamer(streamer);      
        
        //streamer->p_wp     = streamer->p_rp;
        //streamer->p_urb_wp = streamer->p_rp;       
        
        dvb_streamer_dbg("flush streaming\n");
        
        unlock_streamer(streamer);
        
        dvb_usb_streamer_start(streamer, streamer->mode);                
    }    
    else
    {        
        //streamer->p_wp     = streamer->p_rp;
        //streamer->p_urb_wp = streamer->p_rp;        
        streamer->p_rp     = streamer->p_base;
        streamer->p_wp     = streamer->p_base;
        streamer->p_urb_wp = streamer->p_base;     
        unlock_streamer(streamer); 
    }                            
            
    return 0;
}


/*----------------------------------------------------------
 * Func : dvb_usb_streamer_control
 *
 * Desc : streaming_control
 *
 * Parm : fe        : handle of dvb frontend
 *        cmd       : passthrough command
 *        mode      : passthrough mode
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
int dvb_usb_streamer_control(
    struct dvb_usb_streamer*    streamer,
    dvb_passthrough_cmd     cmd, 
    dvb_passthrough_mode    mode
    )
{                
    switch(cmd) 
    {             
    case START_PASSTHROUGH: 
        
        return dvb_usb_streamer_start(streamer, mode);                        
        
    case STOP_PASSTHROUGH:  
        
        return dvb_usb_streamer_stop(streamer);                                    
        
    case FLUSH_PASSTHROUGH_BUFFER:    
        
        return dvb_usb_streamer_flush(streamer);         
    }    
            
	return -1;
}




/*----------------------------------------------------------
 * Func : dvb_usb_streamer_set_buffer
 *
 * Desc : set buffer to a dvb usb streamer
 *
 * Parm : fe        : handle of dvb frontend
 *        p_buff    : passthrough buffer
 *        size      : size of passthrough buffer
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
int dvb_usb_streamer_set_buffer(
    struct dvb_usb_streamer* streamer, 
    unsigned char*                       p_buff, 
    unsigned long                        size
    )
{    
    struct dvb_usb_device* d = streamer->d;
    int ret = 0;    
    
    lock_streamer(streamer);    
        
    if (streamer->state & STREAMER_IS_RUNING)
    {        
        dvb_streamer_warning("set passthrough buffer failed, streamer is runing, please stop it first\n");        
        ret = -1;        
    }        
    else
    {
        dvb_streamer_dbg("set buffer : buff=%p, size  = %lu\n",p_buff, size);
        
        size -= size % (d->urbs_initialized * d->props.urb.u.bulk.buffersize);  // check aligment issue

        if (size)
        {    
            streamer->p_base  = p_buff;
            streamer->p_limit = p_buff + size;
            streamer->p_rp    = p_buff;
            streamer->p_wp    = p_buff;      
            streamer->p_urb_wp= p_buff;               
            
            dvb_streamer_dbg("set buffer : base  = %p\n",streamer->p_base);
            dvb_streamer_dbg("set buffer : limit = %p\n",streamer->p_limit);
            dvb_streamer_dbg("set buffer : rp = %p\n",streamer->p_rp);
            dvb_streamer_dbg("set buffer : wp = %p\n",streamer->p_wp);
            dvb_streamer_dbg("set buffer : p_urb_wp = %p\n",streamer->p_urb_wp);
            dvb_streamer_dbg("set buffer : size  = %lu\n",size);

            streamer->state |= STREAMER_BUFFER_RDY;
        }
        else
        {            
            streamer->state &= ~STREAMER_BUFFER_RDY;
            streamer->p_base   = 0;
            streamer->p_limit  = 0;
            streamer->p_rp     = 0;
            streamer->p_wp     = 0;            
            streamer->p_urb_wp = 0;            
            
            dvb_streamer_dbg("streaming buffer removed\n");
        }   
        ret = 0;     
    }    
    
    unlock_streamer(streamer);
	
	return ret;
}




/*----------------------------------------------------------
 * Func : dvb_usb_streamer_read_data
 *
 * Desc : read data from passthrough buffer
 *
 * Parm : fe        : handle of dvb frontend
 *        pp_buff   : readable data of passthrough buffer
 *        p_size    : size of readable data area
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
int dvb_usb_streamer_read_data(
    struct dvb_usb_streamer* streamer,
    unsigned char**                      pp_buff, 
    unsigned long*                       p_size
    )
{        
    int ret  = 0;
    *pp_buff = 0;
    *p_size  = 0;
        
    lock_streamer(streamer);        
    
    if (streamer->state & STREAMER_IS_RUNING)
    {        
        *pp_buff = streamer->p_rp;
        *p_size  = 0;
                                                    
        if (streamer->p_wp >= streamer->p_rp)                                                
        {
            *p_size = (unsigned long) streamer->p_wp - (unsigned long) streamer->p_rp ;
        }
        else
        {
            if (streamer->p_wp == streamer->p_base)
            {
                if (streamer->p_limit > (streamer->p_rp + 188))
                    *p_size = (unsigned long) streamer->p_limit - 188 - (unsigned long) streamer->p_rp;
            }            
            else
            {            
                *p_size = (unsigned long) streamer->p_limit - (unsigned long) streamer->p_rp;
            }                        
        }
                
        dvb_streamer_read_dbg("read data, data=%p, len=%lu\n", *pp_buff, *p_size);
        ret = 0;
    }
    else
    {
        dvb_streamer_warning("warning, read data while streamins stopped\n");             
        ret = -1;
    }
            
    unlock_streamer(streamer);        
    
	return ret;
}




/*----------------------------------------------------------
 * Func : dvb_usb_streamer_release_data
 *
 * Desc : release data from dvb usb streamer
 *
 * Parm : fe        : handle of dvb frontend
 *        p_buff    : start address of data to release
 *        size      : size of data to release
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
int dvb_usb_streamer_release_data(
    struct dvb_usb_streamer* streamer,
    unsigned char*                       p_buff, 
    unsigned long                        size
    )
{        
    int ret = 0;
    
    lock_streamer(streamer);   
    
    dvb_streamer_read_dbg("release data, data=%p, len=%lu\n", p_buff, size);
    
    if (streamer->state & STREAMER_IS_RUNING)
    {        
        if (streamer->p_rp == p_buff)
        {
            streamer->p_rp += size;
            
            if (streamer->p_rp >= streamer->p_limit)
                streamer->p_rp  = streamer->p_base;
        }
        else
        {
            dvb_streamer_warning("release data failed, rp mismatch %p/%p, len=%lu\n", p_buff, streamer->p_rp, size);
            ret = -1;
        }
    }
    
    unlock_streamer(streamer);
    
	return ret;
}



/*----------------------------------------------------------
 * Func : dvb_usb_streamer_init
 *
 * Desc : init pass through streamer
 *
 * Parm : dvb_        : handle of dvb frontend
 *        cmd       : passthrough command
 *        mode      : passthrough mode
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
void dvb_usb_streamer_init(
    struct dvb_usb_streamer*    streamer,
    struct dvb_usb_device*      d
    )
{
    memset(streamer, 0, sizeof(struct dvb_usb_streamer));
    
    init_streamer_lock(streamer);
    
    streamer->d       = d;
    streamer->state  |= STREAMER_INIT_RDY;    
}




/*----------------------------------------------------------
 * Func : dvb_usb_streamer_uninit
 *
 * Desc : uninit dvb usb streamer
 *
 * Parm : dvb_        : handle of dvb frontend 
 *
 * Retn : 0 success, others failed
 *----------------------------------------------------------*/
void dvb_usb_streamer_uninit(
    struct dvb_usb_streamer*    streamer
    )
{        
    dvb_usb_streamer_stop(streamer); 
}
