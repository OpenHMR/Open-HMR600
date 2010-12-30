#define _HCI_INTF_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <xmit_osdep.h>
#include <hal_init.h>
#include <rtl8712_efuse.h>

#ifndef CONFIG_USB_HCI

#error "CONFIG_USB_HCI shall be on!\n"

#endif

#include <usb_vendor_req.h>
#include <usb_ops.h>
#include <usb_osintf.h>
#include <usb_hal.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifdef CONFIG_80211N_HT
extern int ht_enable;
extern int cbw40_enable;
extern int ampdu_enable;//for enable tx_ampdu
#endif

static struct usb_interface *pintf;

extern u32 start_drv_threads(_adapter *padapter);
extern void stop_drv_threads (_adapter *padapter);
extern u8 init_drv_sw(_adapter *padapter);
extern u8 free_drv_sw(_adapter *padapter);
extern struct net_device *init_netdev(void);

void r871x_dev_unload(_adapter *padapter);

static int r871xu_drv_init(struct usb_interface *pusb_intf,const struct usb_device_id *pdid);
static void r871xu_dev_remove(struct usb_interface *pusb_intf);

static struct usb_device_id rtl871x_usb_id_tbl[] ={
	//92CU
	// Realtek */
	{USB_DEVICE(0x0bda, 0x8176)},
	//92DU
	// Realtek */
	{USB_DEVICE(0x0bda, 0x8193)},
	{}
};

static struct specific_device_id specific_device_id_tbl[] = {
		{.idVendor=0x0b05, .idProduct=0x1791, .flags=SPEC_DEV_ID_DISABLE_HT},
		{.idVendor=0x13D3, .idProduct=0x3311, .flags=SPEC_DEV_ID_DISABLE_HT},
	{}
};

typedef struct _driver_priv{

	struct usb_driver r871xu_drv;
	int drv_registered;

}drv_priv, *pdrv_priv;


static drv_priv drvpriv = {
	.r871xu_drv.name = "r871x_usb_drv",
	.r871xu_drv.id_table = rtl871x_usb_id_tbl,
	.r871xu_drv.probe = r871xu_drv_init,
	.r871xu_drv.disconnect = r871xu_dev_remove,
	.r871xu_drv.suspend = NULL,
	.r871xu_drv.resume = NULL,

};

#ifdef RTK_DMP_PLATFORM
#define RTL819xU_MODULE_NAME "rtl819xSU"
static struct proc_dir_entry *rtl8712_proc = NULL;
static struct proc_dir_entry *dir_dev = NULL;

void rtl8712_proc_init_one(struct net_device *dev)
{
	dir_dev = create_proc_entry(dev->name, 
					  S_IFDIR | S_IRUGO | S_IXUGO, 
					  rtl8712_proc);
}

void rtl8712_proc_remove_one(struct net_device *dev)
{
	if (dir_dev) {
		remove_proc_entry(dev->name, rtl8712_proc);
		dir_dev = NULL;
	}
}
#endif

MODULE_DEVICE_TABLE(usb, rtl871x_usb_id_tbl);

#define MAX_RECEIVE_BUFFER_SIZE(_ADAPTER) 	32768


static inline int RT_usb_endpoint_dir_in(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN);
}

static inline int RT_usb_endpoint_dir_out(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT);
}

static inline int RT_usb_endpoint_xfer_int(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT);
}

static inline int RT_usb_endpoint_xfer_bulk(const struct usb_endpoint_descriptor *epd)
{
 	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK);
}

static inline int RT_usb_endpoint_is_bulk_in(const struct usb_endpoint_descriptor *epd)
{
	return (RT_usb_endpoint_xfer_bulk(epd) && RT_usb_endpoint_dir_in(epd));
}

static inline int RT_usb_endpoint_is_bulk_out(const struct usb_endpoint_descriptor *epd)
{
	return (RT_usb_endpoint_xfer_bulk(epd) && RT_usb_endpoint_dir_out(epd));
}

static inline int RT_usb_endpoint_is_int_in(const struct usb_endpoint_descriptor *epd)
{
	return (RT_usb_endpoint_xfer_int(epd) && RT_usb_endpoint_dir_in(epd));
}

static inline int RT_usb_endpoint_num(const struct usb_endpoint_descriptor *epd)
{
	return epd->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
}

uint usb_dvobj_init(_adapter *padapter)
{
	int i;
	u8 val8;
	u32 blocksz;
	uint status = _SUCCESS;

	struct usb_device_descriptor 		*pdev_desc;

	struct usb_host_config			*phost_conf;
	struct usb_config_descriptor 		*pconf_desc;

	struct usb_host_interface		*phost_iface;
	struct usb_interface_descriptor		*piface_desc;

	struct usb_host_endpoint		*phost_endp;
	struct usb_endpoint_descriptor		*pendp_desc;

	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);

	struct dvobj_priv *pdvobjpriv = &padapter->dvobjpriv;
	struct usb_device *pusbd = pdvobjpriv->pusbdev;

	//PURB urb = NULL;

_func_enter_;

	pdvobjpriv->padapter = padapter;

	pHalData->RtNumInPipes = 0;
	pHalData->RtNumOutPipes = 0;

	//padapter->EepromAddressSize = 6;
	//pdvobjpriv->nr_endpoint = 6;

	pdev_desc = &pusbd->descriptor;

#if 0
	printk("\n8712_usb_device_descriptor:\n");
	printk("bLength=%x\n", pdev_desc->bLength);
	printk("bDescriptorType=%x\n", pdev_desc->bDescriptorType);
	printk("bcdUSB=%x\n", pdev_desc->bcdUSB);
	printk("bDeviceClass=%x\n", pdev_desc->bDeviceClass);
	printk("bDeviceSubClass=%x\n", pdev_desc->bDeviceSubClass);
	printk("bDeviceProtocol=%x\n", pdev_desc->bDeviceProtocol);
	printk("bMaxPacketSize0=%x\n", pdev_desc->bMaxPacketSize0);
	printk("idVendor=%x\n", pdev_desc->idVendor);
	printk("idProduct=%x\n", pdev_desc->idProduct);
	printk("bcdDevice=%x\n", pdev_desc->bcdDevice);
	printk("iManufacturer=%x\n", pdev_desc->iManufacturer);
	printk("iProduct=%x\n", pdev_desc->iProduct);
	printk("iSerialNumber=%x\n", pdev_desc->iSerialNumber);
	printk("bNumConfigurations=%x\n", pdev_desc->bNumConfigurations);
#endif

	phost_conf = pusbd->actconfig;
	pconf_desc = &phost_conf->desc;

#if 0
	printk("\n8712_usb_configuration_descriptor:\n");
	printk("bLength=%x\n", pconf_desc->bLength);
	printk("bDescriptorType=%x\n", pconf_desc->bDescriptorType);
	printk("wTotalLength=%x\n", pconf_desc->wTotalLength);
	printk("bNumInterfaces=%x\n", pconf_desc->bNumInterfaces);
	printk("bConfigurationValue=%x\n", pconf_desc->bConfigurationValue);
	printk("iConfiguration=%x\n", pconf_desc->iConfiguration);
	printk("bmAttributes=%x\n", pconf_desc->bmAttributes);
	printk("bMaxPower=%x\n", pconf_desc->bMaxPower);
#endif

	//printk("\n/****** num of altsetting = (%d) ******/\n", pintf->num_altsetting);

	phost_iface = &pintf->altsetting[0];
	piface_desc = &phost_iface->desc;

#if 1
	printk("\n8192D_usb_interface_descriptor:\n");
	printk("bLength=%x\n", piface_desc->bLength);
	printk("bDescriptorType=%x\n", piface_desc->bDescriptorType);
	printk("bInterfaceNumber=%x\n", piface_desc->bInterfaceNumber);
	printk("bAlternateSetting=%x\n", piface_desc->bAlternateSetting);
	printk("bNumEndpoints=%x\n", piface_desc->bNumEndpoints);
	printk("bInterfaceClass=%x\n", piface_desc->bInterfaceClass);
	printk("bInterfaceSubClass=%x\n", piface_desc->bInterfaceSubClass);
	printk("bInterfaceProtocol=%x\n", piface_desc->bInterfaceProtocol);
	printk("iInterface=%x\n", piface_desc->iInterface);
#endif
	pHalData->interfaceIndex = piface_desc->bInterfaceNumber;

	pdvobjpriv->nr_endpoint = piface_desc->bNumEndpoints;


	//printk("\ndump 8712_usb_endpoint_descriptor:\n");

	for (i = 0; i < pdvobjpriv->nr_endpoint; i++)
	{
		phost_endp = phost_iface->endpoint + i;
		if (phost_endp)
		{
			pendp_desc = &phost_endp->desc;

			printk("\n8192D_usb_endpoint_descriptor(%d):\n", i);
			printk("bLength=%x\n",pendp_desc->bLength);
			printk("bDescriptorType=%x\n",pendp_desc->bDescriptorType);
			printk("bEndpointAddress=%x\n",pendp_desc->bEndpointAddress);
			//printk("bmAttributes=%x\n",pendp_desc->bmAttributes);
			printk("wMaxPacketSize=%x\n",le16_to_cpu(pendp_desc->wMaxPacketSize));
			printk("bInterval=%x\n",pendp_desc->bInterval);
			//printk("bRefresh=%x\n",pendp_desc->bRefresh);
			//printk("bSynchAddress=%x\n",pendp_desc->bSynchAddress);

			if (RT_usb_endpoint_is_bulk_in(pendp_desc))
			{
				pHalData->RtBulkInPipes = RT_usb_endpoint_num(pendp_desc);
				pHalData->RtNumInPipes++;
				printk("RT_usb_endpoint_is_bulk_in = %x\n",pHalData->RtBulkInPipes);
			}
			else if (RT_usb_endpoint_is_int_in(pendp_desc))
			{
				pHalData->RtIntInPipes = RT_usb_endpoint_num(pendp_desc);
				pHalData->RtNumInPipes++;
				printk("RT_usb_endpoint_is_int_in = %x\n",pHalData->RtIntInPipes);
			}
			else if (RT_usb_endpoint_is_bulk_out(pendp_desc))
			{
				pHalData->RtBulkOutPipes[pHalData->RtNumOutPipes] = RT_usb_endpoint_num(pendp_desc);
				printk("RT_usb_endpoint_is_bulk_out = %x\n",pHalData->RtBulkOutPipes[pHalData->RtNumOutPipes]);
				pHalData->RtNumOutPipes++;
			}
		}
	}
	
	printk("nr_endpoint=%d, in_num=%d, out_num=%d\n\n", pdvobjpriv->nr_endpoint, pHalData->RtNumInPipes, pHalData->RtNumOutPipes);

	if (pusbd->speed == USB_SPEED_HIGH)
	{
		pdvobjpriv->ishighspeed = _TRUE;
		pHalData->UsbBulkOutSize = USB_HIGH_SPEED_BULK_SIZE;//512 bytes
		printk("8192du: USB_SPEED_HIGH\n");
	}
	else
	{
		pdvobjpriv->ishighspeed = _FALSE;
		pHalData->UsbBulkOutSize = USB_FULL_SPEED_BULK_SIZE;//64 bytes
		printk("8192du: NON USB_SPEED_HIGH\n");
	}

#if USB_TX_AGGREGATION_92C
	pHalData->UsbTxAggMode		= 1;
	pHalData->UsbTxAggDescNum	= 0xF;	// only 4 bits
#endif

#if USB_RX_AGGREGATION_92C
	pHalData->UsbRxAggMode		= USB_RX_AGG_DMA_USB;
	pHalData->UsbRxAggBlockCount	= 8; //unit : 512b
	pHalData->UsbRxAggBlockTimeout	= 0x6;
	pHalData->UsbRxAggPageCount	= 16; //uint :128 b //0x0A;	// 10 = MAX_RX_DMA_BUFFER_SIZE/2/pHalData->UsbBulkOutSize
	pHalData->UsbRxAggPageTimeout	= 0x4; //6, absolute time = 34ms/(2^6)
/*
//	RT_ASSERT((MAX_RECEIVE_BUFFER_SIZE(padapter) > MAX_RX_DMA_BUFFER_SIZE), ("Receive buffer in definition is too small!!\n"));
	pHalData->MaxUsbRxAggBlock	= (MAX_RECEIVE_BUFFER_SIZE(padapter) - MAX_RX_DMA_BUFFER_SIZE)/pHalData->UsbBulkOutSize;
	if (pHalData->UsbRxAggBlockCount > pHalData->MaxUsbRxAggBlock) {
		pHalData->UsbRxAggBlockCount = (u8)pHalData->MaxUsbRxAggBlock;
//		RT_ASSERT(0 != pHalData->UsbRxAggBlockCount, ("UsbRxAggBlockCount is zero!!\n"));
	}
*/
#endif

	//.2
	if ((alloc_io_queue(padapter)) == _FAIL)
	{
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,(" \n Can't init io_reqs\n"));
		status = _FAIL;
	}

	//.3 misc
	_init_sema(&(padapter->dvobjpriv.usb_suspend_sema), 0);


	//.4
	HalUsbSetQueuePipeMapping8192CUsb(padapter,
			(u8)pHalData->RtNumInPipes, (u8)pHalData->RtNumOutPipes);

_func_exit_;

	return status;
}

void usb_dvobj_deinit(_adapter * padapter){

	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;

	_func_enter_;


	_func_exit_;
}

void rtl871x_intf_stop(_adapter *padapter)
{
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+rtl871x_intf_stop\n"));

	//disabel_hw_interrupt
	if(padapter->bSurpriseRemoved == _FALSE)
	{
		//device still exists, so driver can do i/o operation
		//TODO:
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("SurpriseRemoved==_FALSE\n"));
	}

	//cancel in irp
	if(padapter->dvobjpriv.inirp_deinit !=NULL)
	{
		padapter->dvobjpriv.inirp_deinit(padapter);
	}

	//cancel out irp
	usb_write_port_cancel(padapter);


	//todo:cancel other irps

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-rtl871x_intf_stop\n"));

}

void r871x_dev_unload(_adapter *padapter)
{
	struct net_device *pnetdev= (struct net_device*)padapter->pnetdev;

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+r871x_dev_unload\n"));

	if(padapter->bup == _TRUE)
	{
		printk("+r871x_dev_unload\n");
		//s1.
/*		if(pnetdev)
		{
			netif_carrier_off(pnetdev);
			netif_stop_queue(pnetdev);
		}

		//s2.
		//s2-1.  issue disassoc_cmd to fw
		disassoc_cmd(padapter);
		//s2-2.  indicate disconnect to os
		indicate_disconnect(padapter);
		//s2-3.
		free_assoc_resources(padapter);
		//s2-4.
		free_network_queue(padapter);*/

		padapter->bDriverStopped = _TRUE;

		//s3.
		rtl871x_intf_stop(padapter);

		//s4.
		stop_drv_threads(padapter);


		//s5.
		if(padapter->bSurpriseRemoved == _FALSE)
		{
			printk("r871x_dev_unload()->rtl871x_hal_deinit()\n");
			rtl871x_hal_deinit(padapter);

			padapter->bSurpriseRemoved = _TRUE;
		}

		//s6.
		if(padapter->dvobj_deinit)
		{
			padapter->dvobj_deinit(padapter);

		}
		else
		{
			RT_TRACE(_module_hci_intfs_c_,_drv_err_,("Initialize hcipriv.hci_priv_init error!!!\n"));
		}

		padapter->bup = _FALSE;

	}
	else
	{
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("r871x_dev_unload():padapter->bup == _FALSE\n" ));
	}

	printk("-r871x_dev_unload\n");

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-r871x_dev_unload\n"));

}

static void disable_ht_for_spec_devid(const struct usb_device_id *pdid)
{
#ifdef CONFIG_80211N_HT
	u16 vid, pid;
	u32 flags;
	int i;
	int num = sizeof(specific_device_id_tbl)/sizeof(struct specific_device_id);

	for(i=0; i<num; i++)
	{
		vid = specific_device_id_tbl[i].idVendor;
		pid = specific_device_id_tbl[i].idProduct;
		flags = specific_device_id_tbl[i].flags;

		if((pdid->idVendor==vid) && (pdid->idProduct==pid) && (flags&SPEC_DEV_ID_DISABLE_HT))
		{
			 ht_enable = 0;
			 cbw40_enable = 0;
			 ampdu_enable = 0;
		}

	}
#endif
}

/*
 * drv_init() - a device potentially for us
 *
 * notes: drv_init() is called when the bus driver has located a card for us to support.
 *        We accept the new device by returning 0.
*/
static int r871xu_drv_init(struct usb_interface *pusb_intf, const struct usb_device_id *pdid)
{
	int i;
	u8 mac[ETH_ALEN];
	uint status;
	_adapter *padapter = NULL;
	struct dvobj_priv *pdvobjpriv;
	struct net_device *pnetdev;

	RT_TRACE(_module_hci_intfs_c_, _drv_err_, ("+871x - drv_init\n"));
	//printk("+871x - drv_init\n");

	//2009.8.13, by Thomas
	// In this probe function, O.S. will provide the usb interface pointer to driver.
	// We have to increase the reference count of the usb device structure by using the usb_get_dev function.
	usb_get_dev(interface_to_usbdev(pusb_intf));

	pintf = pusb_intf;

#ifdef CONFIG_80211N_HT
	//step 0.
	disable_ht_for_spec_devid(pdid);
#endif

	//step 1. set USB interface data
	// init data
	pnetdev = init_netdev();
	if (!pnetdev) goto error;
	SET_NETDEV_DEV(pnetdev, &pusb_intf->dev);

	padapter = netdev_priv(pnetdev);
	pdvobjpriv = &padapter->dvobjpriv;
	pdvobjpriv->padapter = padapter;
	pdvobjpriv->pusbdev = interface_to_usbdev(pusb_intf);

	// set data
	usb_set_intfdata(pusb_intf, pnetdev);

	NicIFAssociateNIC(padapter);

	//step 2.
	padapter->dvobj_init = &usb_dvobj_init;
	padapter->dvobj_deinit = &usb_dvobj_deinit;
	padapter->halpriv.hal_bus_init = &usb_hal_bus_init;
	padapter->halpriv.hal_bus_deinit = &usb_hal_bus_deinit;
	padapter->dvobjpriv.inirp_init = &usb_inirp_init;
	padapter->dvobjpriv.inirp_deinit = &usb_inirp_deinit;


	//step 3.
	//initialize the dvobj_priv
	status = padapter->dvobj_init(padapter);
	if (status != _SUCCESS) {
		RT_TRACE(_module_hci_intfs_c_, _drv_err_, ("initialize device object priv Failed!\n"));
		goto error;
	}


	//step 4.
	status = init_drv_sw(padapter);
	if(status ==_FAIL){
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("Initialize driver software resource Failed!\n"));
		goto error;
	}


	//step 5. read efuse/eeprom data and get mac_addr
	NicIFReadAdapterInfo(padapter);

	_memcpy(mac, padapter->eeprompriv.mac_addr, ETH_ALEN);
	if (((mac[0]==0xff) &&(mac[1]==0xff) && (mac[2]==0xff) &&
	     (mac[3]==0xff) && (mac[4]==0xff) &&(mac[5]==0xff)) ||
	    ((mac[0]==0x0) && (mac[1]==0x0) && (mac[2]==0x0) &&
	     (mac[3]==0x0) && (mac[4]==0x0) &&(mac[5]==0x0)))
	{
		mac[0] = 0x00;
		mac[1] = 0xe0;
		mac[2] = 0x4c;
		mac[3] = 0x87;
		mac[4] = 0x00;
		mac[5] = 0x00;
		_memcpy(padapter->eeprompriv.mac_addr, mac, ETH_ALEN);
	}
	_memcpy(pnetdev->dev_addr, mac, ETH_ALEN);
	printk("MAC Address from efuse= %02x:%02x:%02x:%02x:%02x:%02x\n",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);


	//step 6.
	/* Tell the network stack we exist */
	if (register_netdev(pnetdev) != 0) {
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("register_netdev() failed\n"));
		goto error;
	}

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-drv_init - Adapter->bDriverStopped=%d, Adapter->bSurpriseRemoved=%d\n",padapter->bDriverStopped, padapter->bSurpriseRemoved));
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-871x_drv - drv_init, success!\n"));
	//printk("-871x_drv - drv_init, success!\n");

#ifdef RTK_DMP_PLATFORM
	rtl8712_proc_init_one(pnetdev);
#endif

#ifdef CONFIG_AP_MODE
	hostapd_mode_init(padapter);
#endif

#ifdef CONFIG_PLATFORM_RTD2880B
	printk("wlan link up\n");
	rtd2885_wlan_netlink_sendMsg("linkup", "8712");
#endif

	return 0;

error:

	usb_put_dev(interface_to_usbdev(pusb_intf));//decrease the reference count of the usb device structure if driver fail on initialzation

	usb_set_intfdata(pusb_intf, NULL);

	if (padapter->dvobj_deinit == NULL) {
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("\n Initialize dvobjpriv.dvobj_deinit error!!!\n"));
	} else {
		padapter->dvobj_deinit(padapter);
	}

	if (pnetdev)
	{
		//unregister_netdev(pnetdev);
		free_netdev(pnetdev);
	}

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-871x_sdio - drv_init, fail!\n"));
	//printk("-871x_sdio - drv_init, fail!\n");

	return -ENODEV;
}

/*
 * dev_remove() - our device is being removed
*/
//rmmod module & unplug(SurpriseRemoved) will call r871xu_dev_remove() => how to recognize both
static void r871xu_dev_remove(struct usb_interface *pusb_intf)
{
	struct net_device *pnetdev=usb_get_intfdata(pusb_intf);
	_adapter *padapter = (_adapter*)netdev_priv(pnetdev);

_func_exit_;

	usb_set_intfdata(pusb_intf, NULL);

	if(padapter)
	{
		printk("+r871xu_dev_remove\n");
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+dev_remove()\n"));

#ifdef CONFIG_AP_MODE
		hostapd_mode_unload(padapter);
#endif

		if(drvpriv.drv_registered == _TRUE)
		{
			//printk("r871xu_dev_remove():padapter->bSurpriseRemoved == _TRUE\n");
			padapter->bSurpriseRemoved = _TRUE;
		}
		/*else
		{
			//printk("r871xu_dev_remove():module removed\n");
			padapter->hw_init_completed = _FALSE;
		}*/

		if(pnetdev) {
			unregister_netdev(pnetdev); //will call netdev_close()

#ifdef RTK_DMP_PLATFORM
			rtl8712_proc_remove_one(pnetdev);
#endif
		}

		r871x_dev_unload(padapter);

		printk("+r871xu_dev_remove, hw_init_completed=%d\n", padapter->hw_init_completed);

		free_drv_sw(padapter);

		//after free_drv_sw(), padapter has beed freed, don't refer to it.
		
	}

	usb_put_dev(interface_to_usbdev(pusb_intf));//decrease the reference count of the usb device structure when disconnect

	//If we didn't unplug usb dongle and remove/insert modlue, driver fails on sitesurvey for the first time when device is up .
	//Reset usb port for sitesurvey fail issue. 2009.8.13, by Thomas
	if(interface_to_usbdev(pusb_intf)->state != USB_STATE_NOTATTACHED)
	{
		printk("usb attached..., try to reset usb device\n");
		usb_reset_device(interface_to_usbdev(pusb_intf));
	}	

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-dev_remove()\n"));
	printk("-r871xu_dev_remove, done\n");

#ifdef CONFIG_PLATFORM_RTD2880B
	printk("wlan link down\n");
	rtd2885_wlan_netlink_sendMsg("linkdown", "8712");
#endif


_func_exit_;

	return;

}


static int __init r8712u_drv_entry(void)
{
#ifdef RTK_DMP_PLATFORM
	rtl8712_proc=create_proc_entry(RTL819xU_MODULE_NAME, S_IFDIR, proc_net);
	if (rtl8712_proc == NULL) {
		printk(KERN_ERR "Unable to create rtl8712_proc directory\n");
	}
#endif
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+r8712u_drv_entry\n"));
	drvpriv.drv_registered = _TRUE;
	return usb_register(&drvpriv.r871xu_drv);
}

static void __exit r8712u_drv_halt(void)
{
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+r8712u_drv_halt\n"));
	printk("+r8712u_drv_halt\n");
	drvpriv.drv_registered = _FALSE;
	usb_deregister(&drvpriv.r871xu_drv);
	printk("-r8712u_drv_halt\n");
#ifdef RTK_DMP_PLATFORM
	if(rtl8712_proc){
		remove_proc_entry(RTL819xU_MODULE_NAME, proc_net);
		rtl8712_proc = NULL;
	}
#endif
}


module_init(r8712u_drv_entry);
module_exit(r8712u_drv_halt);


/*
init (driver module)-> r8712u_drv_entry
probe (sd device)-> r871xu_drv_init(dev_init)
open (net_device) ->netdev_open
close (net_device) ->netdev_close
remove (sd device) ->r871xu_dev_remove
exit (driver module)-> r8712u_drv_halt
*/


/*
r8711s_drv_entry()
r8711u_drv_entry()
r8712s_drv_entry()
r8712u_drv_entry()
*/

