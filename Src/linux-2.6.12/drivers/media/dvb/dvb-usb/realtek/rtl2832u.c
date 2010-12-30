
#include <linux/module.h>
#include <linux/version.h>

#include "rtl2832u.h"
#include "rtl2832u_io.h"
#include "rtl2832u_fe.h"

int dvb_usb_rtl2832u_debug;
module_param_named(debug,dvb_usb_rtl2832u_debug, int, 0644);
MODULE_PARM_DESC(debug, "set debugging level (1=info,xfer=2 (or-able))." DVB_USB_DEBUG_STATUS);

#define	USB_EPA_CTL	0x0148

extern int usb_epa_fifo_reset(struct rtl2832_state*	p_state);




/*=======================================================
 * Func : rtl2832u_streaming_ctrl( 
 *
 * Desc : ask demod to start/stop streaming
 *
 * Parm : d :   handle of dvb usb device  
 *        onoff : 0 : off, others : on 
 *
 * Retn : 0 for success, others for failed
 *=======================================================*/
static 
int rtl2832u_streaming_ctrl(
    struct dvb_usb_device*  d, 
    int                     onoff
    )
{
    u8 data[2];
	
    if (onoff)          	    	    
    {		   
        printk("[RTL2832] start streaming....\n");

        data[0] = data[1] = 0;
		
        usb_epa_fifo_reset((struct rtl2832_state*) d->fe->demodulator_priv);		
        
        usb_clear_halt(d->udev,usb_rcvbulkpipe(d->udev,d->props.urb.endpoint));
        
        if (write_usb_sys_char_bytes(d, RTD2832U_USB, USB_EPA_CTL, data, 2)) 
            return -1;
    }
    else
    {
        printk("[RTL2832] stop streaming....\n");

        data[0] = 0x10;	// stall epa, set bit 4 to 1
        data[1] = 0x02;	// reset epa, set bit 9 to 1
		
        if (write_usb_sys_char_bytes(d, RTD2832U_USB , USB_EPA_CTL, data , 2) ) 
            return -1;
    }

    return 0;	
}



/*=======================================================
 * Func : rtl2832u_pid_filter_ctrl
 *
 * Desc : disable/enable pid filters
 *
 * Parm : d :   handle of dvb usb device  
 *        onoff : 0 : off, others : on 
 *
 * Retn : 0 for success, others for failed
 *=======================================================*/
static 
int rtl2832u_pid_filter_ctrl(
    struct dvb_usb_device*  d, 
    int                     onoff
    )
{	
    unsigned char data;
    
    if (read_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR,0, DEMOD_MODE_PID, &data,1)) 
        return -1;            

    data |= 0xE0; 

    if (onoff)
        data &= ~0x40;
    
    if (write_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR,0, DEMOD_MODE_PID, &data,1)) 
        return -1;
                	  
    printk("[RTL2832] pid filter %s\n", (onoff) ? "enabled" : "disabled");

    return 0;	
}



/*=======================================================
 * Func : rtl2832u_pid_filter 
 *
 * Desc : set pid filter
 *
 * Parm : d     : handle of dvb usb device  
 *        id    : pid entry
 *        pid   : pid value
 *        onoff : 0 : disable this pid, others : enable this pid
 *
 * Retn : 0 for success, others for failed
 *=======================================================*/
static 
int rtl2832u_pid_filter(
    struct dvb_usb_device*  d, 
    int                     id,
    u16                     pid,
    int                     onoff
    )
{	
    unsigned char data;
    unsigned char en_bit = (0x1 << (id & 0x7));
    
    if (id < 0 || id >= RTL2832U_PID_COUNT)	  
        goto err_invlid_pid_id;        

	rtl2832u_dbg("set pid filter, id=%d, pid=%04x, ret=%04x, en=%d\n", id, pid, cpu_to_be16(pid), onoff);

    pid = cpu_to_be16(pid);         // convert to big endian

    if (write_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR,0,DEMOD_VALUE_PID0+(id<<1), (unsigned char*) &pid, 2)) 
        goto err_access_reg_failed;

    if (read_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR,0, DEMOD_EN_PID + (id>>3), &data, 1)) 
  	    goto err_access_reg_failed;

    data = (onoff) ? (data | en_bit) : (data & ~en_bit);

  	if (write_rtl2832_demod_register(d, RTL2832_DEMOD_ADDR,0, DEMOD_EN_PID + (id>>3), &data, 1)) 
  	    goto err_access_reg_failed;

    return 0;    
		
err_invlid_pid_id:		
    rtl2832u_warning("set PID filter failed, invalid pid id (%d)\n", id);        
    return -1;    
                	  
err_access_reg_failed:
    rtl2832u_warning("set PID filter failed, read/write register failed\n");        
    return -1;     	
}




/*=======================================================
 * Func : rtl2832u_frontend_attach 
 *
 * Desc : attach frontend to dvb usb device
 *
 * Parm : d     : handle of dvb usb device   
 *
 * Retn : 0 for success, others for failed
 *=======================================================*/
static int rtl2832u_frontend_attach(
    struct dvb_usb_device*  d
    )
{
	d->fe = rtl2832u_fe_attach(d); 
	return 0;
}



static struct usb_device_id rtl2832u_usb_table [] = {
	{ USB_DEVICE(USB_VID_REALTEK,      USB_PID_RTD2832U_WARM) },
	{ USB_DEVICE(USB_VID_AZUREWAVE,    USB_PID_AZUREWAVE_USB_WARM) },
	{ USB_DEVICE(USB_VID_KWORLD_1ST,   USB_PID_KWORLD_WARM) },
	{ USB_DEVICE(USB_VID_DEXATEK,      USB_PID_DEXATEK_USB_WARM) },
	{ USB_DEVICE(USB_VID_DEXATEK,      USB_PID_DEXATEK_MINIUSB_WARM) },
	{ USB_DEVICE(USB_VID_DEXATEK,      USB_PID_DEXATEK_5217_WARM) },

	{ USB_DEVICE(USB_VID_REALTEK,      USB_PID_RTD2832U_2ND_WARM) },
	{ USB_DEVICE(USB_VID_GOLDENBRIDGE, USB_PID_GOLDENBRIDGE_WARM) },
	{ USB_DEVICE(USB_VID_YUAN,         USB_PID_YUAN_WARM) },
	{ USB_DEVICE(USB_VID_AZUREWAVE,    USB_PID_AZUREWAVE_MINI_WARM) },
	{ USB_DEVICE(USB_VID_AZUREWAVE,    USB_PID_AZUREWAVE_GPS_WARM) },	
	{ USB_DEVICE(USB_VID_REALTEK,      USB_PID_RTD2839U_WARM) },
	{ 0 },
};


MODULE_DEVICE_TABLE(usb, rtl2832u_usb_table);



static struct dvb_usb_properties rtl2832u_1st_properties = 
{			
    .caps             = DVB_USB_HAS_PID_FILTER | DVB_USB_NEED_PID_FILTERING | DVB_USB_PID_FILTER_CAN_BE_TURNED_OFF,
    .pid_filter_count = 32,
	.streaming_ctrl   = rtl2832u_streaming_ctrl,
	.frontend_attach  = rtl2832u_frontend_attach,		
	.pid_filter_ctrl  = rtl2832u_pid_filter_ctrl,
	.pid_filter       = rtl2832u_pid_filter,
	
	/* parameter for the MPEG2-data transfer */
	.urb = {
		.type       = DVB_USB_BULK,
		.count      = RTD2831_URB_NUMBER,
		.endpoint   = 0x01,                 // data pipe
		.u = {
			.bulk   = {
				.buffersize = RTD2831_URB_SIZE,
			}
		}
	},
    
    // device descriptor lists..............
	.num_device_descs = 7,
	.devices = {
		{ .name = "RTL2832 DVB-T USB DEVICE driver v 2.5",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[0], NULL },
		},
		{ .name = "DVB-T USB Dongle",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[1], NULL },
		},
		{ .name = "UB396-T",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[2], NULL },
		},
		{ .name = "DK DVBT DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[3], NULL },
		},
		{ .name = "DK mini DVBT DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[4], NULL },
		},
		{ .name = "DK 5217 DVBT DONGLE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[5], NULL },
		},		
		{ .name = "RTL2839 DVB-T USB DEVICE driver v 2.5",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[11], NULL },
		},		
		{ NULL },
	}
};


static struct dvb_usb_properties rtl2832u_2nd_properties = {
	
	.caps            = DVB_USB_HAS_PID_FILTER | DVB_USB_NEED_PID_FILTERING | DVB_USB_PID_FILTER_CAN_BE_TURNED_OFF,
	.pid_filter_count= 32,
	.streaming_ctrl  = rtl2832u_streaming_ctrl,	
	.frontend_attach = rtl2832u_frontend_attach,
	.pid_filter_ctrl = rtl2832u_pid_filter_ctrl,
	.pid_filter      = rtl2832u_pid_filter,
	
	/* parameter for the MPEG2-data transfer */
	.urb = {
		.type       = DVB_USB_BULK,
		.count      = RTD2831_URB_NUMBER,
		.endpoint   = 0x01,                 // data pipe
		.u = {
			.bulk   = {
				.buffersize = RTD2831_URB_SIZE,
			}
		}
	},
	
	// device descriptor lists..............
	.num_device_descs = 5,
	.devices = {
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[6], NULL },
		},
		{ .name = "RTL2832U DVB-T USB DEVICE",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[7], NULL },
		},
		{ .name = "Digital TV Tuner Card",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[8], NULL },
		},
		{ .name = "DVB-T FTA USB Half Minicard",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[9], NULL },
		},
		{ .name = "DVB-T + GPS Minicard",
		  .cold_ids = { NULL, NULL },
		  .warm_ids = { &rtl2832u_usb_table[10], NULL },
		},
		{ NULL },
	}
};



////////////////////////////////////////////////////////////////////////////////////////
// USB Part Driver
////////////////////////////////////////////////////////////////////////////////////////



/*=======================================================
 * Func : rtl2832u_usb_disconnect 
 *
 * Desc : disconnect rtl2832u usb device
 *
 * Parm : intf : handle of usb interface
 *
 * Retn : N/A
 *=======================================================*/
static 
void rtl2832u_usb_disconnect(
    struct usb_interface*   intf
    )
{
	dvb_usb_device_exit(intf);	
}



/*=======================================================
 * Func : rtl2832u_usb_disconnect 
 *
 * Desc : disconnect rtl2832u usb device
 *
 * Parm : intf : handle of usb interface
 *        id   : usb device id
 *
 * Retn : N/A
 *=======================================================*/
static 
int rtl2832u_usb_probe(
    struct usb_interface*       intf,
    const struct usb_device_id* id
    )
{
	if ((dvb_usb_device_init(intf,&rtl2832u_1st_properties,THIS_MODULE,NULL)==0)||
		(dvb_usb_device_init(intf,&rtl2832u_2nd_properties,THIS_MODULE,NULL)==0))
		return 0;

	return -ENODEV;
}


static struct usb_driver rtl2832u_usb_driver = 
{
	.name		= "dvb_usb_rtl2832u",
	.probe		= rtl2832u_usb_probe,
	.disconnect	= rtl2832u_usb_disconnect,
	.id_table	= rtl2832u_usb_table,
};



/*=======================================================
 * Func : rtl2832u_usb_module_init 
 *
 * Desc : module init function
 *
 * Parm : N/A
 *
 * Retn : 0
 *=======================================================*/
static 
int __init rtl2832u_usb_module_init(void)
{
	int result;
	if ((result = usb_register(&rtl2832u_usb_driver))) {
		err("usb_register failed. (%d)",result);
		return result;
	}

	return 0;
}



/*=======================================================
 * Func : rtl2832u_usb_module_exit 
 *
 * Desc : module exit function
 *
 * Parm : N/A
 *
 * Retn : 0
 *=======================================================*/
static void __exit rtl2832u_usb_module_exit(void)
{
	usb_deregister(&rtl2832u_usb_driver);
}



module_init(rtl2832u_usb_module_init);
module_exit(rtl2832u_usb_module_exit);



MODULE_AUTHOR("ChiaLing");
MODULE_DESCRIPTION("Driver for the RTL2832U DVB-T USB2.0 devices");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");

