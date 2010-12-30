/******************************************************************************
* mlme_linux.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2007, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/


#define _MLME_OSDEP_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <mlme_osdep.h>

#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/smp_lock.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/circ_buf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/atomic.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <net/iw_handler.h>


#ifdef RTK_DMP_PLATFORM
void Linkup_workitem_callback(struct work_struct *work)
{
	struct mlme_priv *pmlmepriv = container_of(work, struct mlme_priv, Linkup_workitem);
	_adapter *padapter = container_of(pmlmepriv, _adapter, mlmepriv);

_func_enter_;

	RT_TRACE(_module_mlme_osdep_c_,_drv_info_,("+ Linkup_workitem_callback\n"));

	kobject_hotplug(&padapter->pnetdev->class_dev.kobj, KOBJ_LINKUP);

_func_exit_;
}

void Linkdown_workitem_callback(struct work_struct *work)
{
	struct mlme_priv *pmlmepriv = container_of(work, struct mlme_priv, Linkdown_workitem);
	_adapter *padapter = container_of(pmlmepriv, _adapter, mlmepriv);

_func_enter_;

	RT_TRACE(_module_mlme_osdep_c_,_drv_info_,("+ Linkdown_workitem_callback\n"));

	kobject_hotplug(&padapter->pnetdev->class_dev.kobj, KOBJ_LINKDOWN);

_func_exit_;
}
#endif


/*
void sitesurvey_ctrl_handler(void *FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;

	_sitesurvey_ctrl_handler(adapter);

	_set_timer(&adapter->mlmepriv.sitesurveyctrl.sitesurvey_ctrl_timer, 3000);
}
*/

void join_timeout_handler (void *FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;
	_join_timeout_handler(adapter);
}


void _scan_timeout_handler (void *FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;
	scan_timeout_handler(adapter);
}

void dhcp_timeout_handler (void *FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;
	_dhcp_timeout_handler(adapter);
}

void _dynamic_check_timer_handlder (void *FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;
		 
	dynamic_check_timer_handlder(adapter);
	
	_set_timer(&adapter->mlmepriv.dynamic_chk_timer, 2000);
}


void init_mlme_timer(_adapter *padapter)
{
	struct	mlme_priv *pmlmepriv = &padapter->mlmepriv;

	_init_timer(&(pmlmepriv->assoc_timer), padapter->pnetdev, join_timeout_handler, (pmlmepriv->nic_hdl));
	//_init_timer(&(pmlmepriv->sitesurveyctrl.sitesurvey_ctrl_timer), padapter->pnetdev, sitesurvey_ctrl_handler, (u8 *)(pmlmepriv->nic_hdl));
	_init_timer(&(pmlmepriv->scan_to_timer), padapter->pnetdev, _scan_timeout_handler, (pmlmepriv->nic_hdl));
#ifdef CONFIG_PWRCTRL
	_init_timer(&(pmlmepriv->dhcp_timer), padapter->pnetdev, dhcp_timeout_handler, (u8 *)(pmlmepriv->nic_hdl));
#endif

	_init_timer(&(pmlmepriv->dynamic_chk_timer), padapter->pnetdev, _dynamic_check_timer_handlder, (u8 *)(pmlmepriv->nic_hdl));

#ifdef RTK_DMP_PLATFORM
	_init_workitem(&(pmlmepriv->Linkup_workitem), Linkup_workitem_callback, padapter);
	_init_workitem(&(pmlmepriv->Linkdown_workitem), Linkdown_workitem_callback, padapter);
#endif

}

extern void indicate_wx_assoc_event(_adapter *padapter);
extern void indicate_wx_disassoc_event(_adapter *padapter);

void os_indicate_connect(_adapter *adapter)
{

_func_enter_;	

	indicate_wx_assoc_event(adapter);
	netif_carrier_on(adapter->pnetdev);

#ifdef RTK_DMP_PLATFORM
	_set_workitem(&adapter->mlmepriv.Linkup_workitem);
#endif

_func_exit_;	

}

static RT_PMKID_LIST   backupPMKIDList[ NUM_PMKID_CACHE ];
void os_indicate_disconnect( _adapter *adapter )
{
   //RT_PMKID_LIST   backupPMKIDList[ NUM_PMKID_CACHE ];
   u8              backupPMKIDIndex = 0;
   u8              backupTKIPCountermeasure = 0x00;
      
_func_enter_;

	indicate_wx_disassoc_event(adapter);	
	netif_carrier_off(adapter->pnetdev);

#ifdef RTK_DMP_PLATFORM
	_set_workitem(&adapter->mlmepriv.Linkdown_workitem);
#endif
   
	if(adapter->securitypriv.dot11AuthAlgrthm == 2)//802.1x
	{		 
		// Added by Albert 2009/02/18
		// We have to backup the PMK information for WiFi PMK Caching test item.
		//
		// Backup the btkip_countermeasure information.
		// When the countermeasure is trigger, the driver have to disconnect with AP for 60 seconds.

		_memset( &backupPMKIDList[ 0 ], 0x00, sizeof( RT_PMKID_LIST ) * NUM_PMKID_CACHE );

		_memcpy( &backupPMKIDList[ 0 ], &adapter->securitypriv.PMKIDList[ 0 ], sizeof( RT_PMKID_LIST ) * NUM_PMKID_CACHE );
		backupPMKIDIndex = adapter->securitypriv.PMKIDIndex;
		backupTKIPCountermeasure = adapter->securitypriv.btkip_countermeasure;

		_memset((unsigned char *)&adapter->securitypriv, 0, sizeof (struct security_priv));
		_init_timer(&(adapter->securitypriv.tkip_timer),adapter->pnetdev, use_tkipkey_handler, adapter);

		// Added by Albert 2009/02/18
		// Restore the PMK information to securitypriv structure for the following connection.
		_memcpy( &adapter->securitypriv.PMKIDList[ 0 ], &backupPMKIDList[ 0 ], sizeof( RT_PMKID_LIST ) * NUM_PMKID_CACHE );
		adapter->securitypriv.PMKIDIndex = backupPMKIDIndex;
		adapter->securitypriv.btkip_countermeasure = backupTKIPCountermeasure;

	}
	else //reset values in securitypriv 
	{
		//if(adapter->mlmepriv.fw_state & WIFI_STATION_STATE)
		//{
		struct security_priv *psec_priv=&adapter->securitypriv;

		psec_priv->dot11AuthAlgrthm = 0; //open system
		psec_priv->dot11PrivacyAlgrthm = _NO_PRIVACY_;
		psec_priv->dot11PrivacyKeyIndex = 0;

		psec_priv->dot118021XGrpPrivacy = _NO_PRIVACY_;
		psec_priv->dot118021XGrpKeyid = 1;

		psec_priv->ndisauthtype = Ndis802_11AuthModeOpen;
		psec_priv->ndisencryptstatus = Ndis802_11WEPDisabled;
		//}
	}

_func_exit_;

}

void report_sec_ie(_adapter *adapter,u8 authmode,u8 *sec_ie)
{
		uint len;
		u8 *buff,*p,i;
		union iwreq_data wrqu;

_func_enter_;

	RT_TRACE(_module_mlme_osdep_c_,_drv_info_,("+report_sec_ie, authmode=%d\n", authmode));

	buff = NULL;
	if(authmode==_WPA_IE_ID_)
	{
		RT_TRACE(_module_mlme_osdep_c_,_drv_info_,("report_sec_ie, authmode=%d\n", authmode));
		
		buff = _malloc(IW_CUSTOM_MAX);
		
		_memset(buff,0,IW_CUSTOM_MAX);
		
		p=buff;
		
		p+=sprintf(p,"ASSOCINFO(ReqIEs=");

		len = sec_ie[1]+2;
		len =  (len < IW_CUSTOM_MAX) ? len:IW_CUSTOM_MAX;
			
		for(i=0;i<len;i++){
			p+=sprintf(p,"%02x",sec_ie[i]);
		}

		p+=sprintf(p,")");
		
		_memset(&wrqu,0,sizeof(wrqu));
		
		wrqu.data.length=p-buff;
		
		wrqu.data.length = (wrqu.data.length<IW_CUSTOM_MAX) ? wrqu.data.length:IW_CUSTOM_MAX;
		
		wireless_send_event(adapter->pnetdev,IWEVCUSTOM,&wrqu,buff);

		if(buff)
		    _mfree(buff, IW_CUSTOM_MAX);
		
	}

_func_exit_;

}

void _survey_timer_hdl (void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	survey_timer_hdl(padapter);
}

void _link_timer_hdl (void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	link_timer_hdl(padapter);
}

void _addba_timer_hdl(void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	addba_timer_hdl(padapter);
}

/*
void _reauth_timer_hdl(void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	reauth_timer_hdl(padapter);
}

void _reassoc_timer_hdl(void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	reassoc_timer_hdl(padapter);
}
*/

void init_mlme_ext_timer(_adapter *padapter)
{	
	struct	mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;

	_init_timer(&pmlmeext->survey_timer, padapter->pnetdev, _survey_timer_hdl, padapter);
	_init_timer(&pmlmeext->link_timer, padapter->pnetdev, _link_timer_hdl, padapter);
	_init_timer(&pmlmeext->ADDBA_timer, padapter->pnetdev, _addba_timer_hdl, padapter);

	//_init_timer(&pmlmeext->reauth_timer, padapter->pnetdev, _reauth_timer_hdl, padapter);
	//_init_timer(&pmlmeext->reassoc_timer, padapter->pnetdev, _reassoc_timer_hdl, padapter);
}

#ifdef CONFIG_AP_MODE

#ifdef CONFIG_HOSTAPD_MLME

extern int ffaddr2pipehdl(struct dvobj_priv *pdvobj, u32 addr);

static void mgnt_xmit_cb(struct urb *urb)
{	
	struct sk_buff *skb = (struct sk_buff *)urb->context;

	//printk("%s\n", __FUNCTION__);


	dev_kfree_skb_any(skb);
	
}

static int mgnt_xmit_entry(struct sk_buff *skb, struct net_device *pnetdev)
{
	u16 fc;
	int rc, len, pipe;	
	unsigned int bmcst, tid, qsel;
	struct sk_buff *pxmit_skb;
	struct urb *urb;
	unsigned char *pxmitbuf;
	struct tx_desc *ptxdesc;
	struct ieee80211_hdr *tx_hdr;
	struct hostapd_priv *phostapdpriv = netdev_priv(pnetdev);
	_adapter *padapter = (_adapter *)phostapdpriv->padapter;
	struct dvobj_priv *pdvobj = &padapter->dvobjpriv;	

	
	//printk("%s\n", __FUNCTION__);

	
	len = skb->len;
	tx_hdr = (struct ieee80211_hdr *)(skb->data);
	fc = le16_to_cpu(tx_hdr->frame_ctl);
	bmcst = IS_MCAST(tx_hdr->addr1);

	if ((fc & IEEE80211_FCTL_FTYPE) != IEEE80211_FTYPE_MGMT)
		goto _exit;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)) // http://www.mail-archive.com/netdev@vger.kernel.org/msg17214.html
	pxmit_skb = dev_alloc_skb(len + TXDESC_SIZE);			
#else			
	pxmit_skb = netdev_alloc_skb(pnetdev, len + TXDESC_SIZE);
#endif		

	if(!pxmit_skb)
		goto _exit;

	pxmitbuf = pxmit_skb->data;

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		goto _exit;
	}

	// ----- fill tx desc -----	
	ptxdesc = (struct tx_desc *)pxmitbuf;	
	_memset(ptxdesc, 0, sizeof(*ptxdesc));
		
	//offset 0	
	ptxdesc->txdw0 |= cpu_to_le32(len&0x0000ffff); 
	ptxdesc->txdw0 |= cpu_to_le32(((TXDESC_SIZE+OFFSET_SZ)<<OFFSET_SHT)&0x00ff0000);//default = 32 bytes for TX Desc
	ptxdesc->txdw0 |= cpu_to_le32(OWN | FSG | LSG);

	//offset 4	
	ptxdesc->txdw1 |= (0x05)&0x1f;//CAM_ID(MAC_ID), default=5;
	ptxdesc->txdw1 |= cpu_to_le32(BIT(16));//Non-QoS

	qsel = tid = 0x06;
	qsel = qsel&0x0000001f;			
	ptxdesc->txdw1 |= cpu_to_le32((qsel<<QSEL_SHT)&0x00001f00);

	//offset 8			
	if(bmcst)	
	{
		ptxdesc->txdw2 |= cpu_to_le32(BMC);
	}	

	//offset 12		
	ptxdesc->txdw3 = cpu_to_le32((tid<<SEQ_SHT)&0x0fff0000);

	//offset 16		
	ptxdesc->txdw4 = cpu_to_le32(0x80002040);//gtest
		
	//offset 20		
	ptxdesc->txdw5 = cpu_to_le32(0x001f8000);//gtest//1M


	// ----- end of fill tx desc -----

	//
	skb_put(pxmit_skb, len + TXDESC_SIZE);
	pxmitbuf = pxmitbuf + TXDESC_SIZE;
	_memcpy(pxmitbuf, skb->data, len);

	//printk("mgnt_xmit, len=%x\n", pxmit_skb->len);


	// ----- prepare urb for submit -----
	
	//translate DMA FIFO addr to pipehandle		
	//pipe = ffaddr2pipehdl(pdvobj, RTL8712_DMA_MGTQ);
	//pipe = ffaddr2pipehdl(pdvobj, RTL8712_DMA_VOQ);
	pipe = usb_sndbulkpipe(pdvobj->pusbdev, 0x04);

#if 0	
		switch(addr)
		{		       
	     		case RTL8712_DMA_BEQ:
		 	//case RTL8712_DMA_BKQ:
	 			//pipe=usb_sndbulkpipe(pusbd, 0x05);
			 	pipe=usb_sndbulkpipe(pusbd, 0x06);	
				break;			
	     		//case RTL8712_DMA_VIQ:
		 	case RTL8712_DMA_VOQ:					
		 		pipe=usb_sndbulkpipe(pusbd, 0x04);
				break;
			case RTL8712_DMA_RX0FF:
			case RTL8712_DMA_C2HCMD:		 	
				pipe=usb_rcvbulkpipe(pusbd, 0x03);//in
				break;
			case RTL8712_DMA_H2CCMD:	
			case RTL8712_DMA_BCNQ:					
			case RTL8712_DMA_BMCQ:	
			case RTL8712_DMA_MGTQ:				
				pipe=usb_sndbulkpipe(pusbd, 0x0d);
				break;	
		}
#endif	
	
	usb_fill_bulk_urb(urb, pdvobj->pusbdev, pipe,
			  pxmit_skb->data, pxmit_skb->len, mgnt_xmit_cb, pxmit_skb);
	
	urb->transfer_flags |= URB_ZERO_PACKET;
	usb_anchor_urb(urb, &phostapdpriv->anchored);
	rc = usb_submit_urb(urb, GFP_ATOMIC);
	if (rc < 0) {
		usb_unanchor_urb(urb);
		kfree_skb(skb);
	}
	usb_free_urb(urb);

	
_exit:	
	
	dev_kfree_skb_any(skb);

	return 0;
	
}

static int mgnt_netdev_open(struct net_device *pnetdev)
{
	struct hostapd_priv *phostapdpriv = netdev_priv(pnetdev);

	printk("mgnt_netdev_open: MAC Address:" MACSTR "\n", MAC2STR(pnetdev->dev_addr));


	init_usb_anchor(&phostapdpriv->anchored);
	
 	if(!netif_queue_stopped(pnetdev))
      		netif_start_queue(pnetdev);
	else
		netif_wake_queue(pnetdev);


	netif_carrier_on(pnetdev);
		
	//write16(phostapdpriv->padapter, 0x0116, 0x0100);//only excluding beacon 
		
	return 0;	
}
static int mgnt_netdev_close(struct net_device *pnetdev)
{
	struct hostapd_priv *phostapdpriv = netdev_priv(pnetdev);

	printk("%s\n", __FUNCTION__);

	usb_kill_anchored_urbs(&phostapdpriv->anchored);

	netif_carrier_off(pnetdev);

	if (!netif_queue_stopped(pnetdev))
		netif_stop_queue(pnetdev);
	
	//write16(phostapdpriv->padapter, 0x0116, 0x3f3f);
	
	return 0;	
}

#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,29))
static const struct net_device_ops rtl871x_mgnt_netdev_ops = {
	.ndo_open = mgnt_netdev_open,
       .ndo_stop = mgnt_netdev_close,
       .ndo_start_xmit = mgnt_xmit_entry,
       //.ndo_set_mac_address = r871x_net_set_mac_address,
       //.ndo_get_stats = r871x_net_get_stats,
       //.ndo_do_ioctl = r871x_mp_ioctl,
};
#endif

int hostapd_mode_init(_adapter *padapter)
{
	unsigned char mac[ETH_ALEN];
	struct hostapd_priv *phostapdpriv;
	struct net_device *pnetdev;
	
	pnetdev = alloc_etherdev(sizeof(struct hostapd_priv));	
	if (!pnetdev)
	   return -ENOMEM;

	//SET_MODULE_OWNER(pnetdev);
       ether_setup(pnetdev);

	//pnetdev->type = ARPHRD_IEEE80211;
	
	phostapdpriv = netdev_priv(pnetdev);
	phostapdpriv->pmgnt_netdev = pnetdev;
	phostapdpriv->padapter= padapter;
	padapter->phostapdpriv = phostapdpriv;
	
	//pnetdev->init = NULL;
	
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,29))

	printk("register rtl871x_mgnt_netdev_ops to netdev_ops\n");

	pnetdev->netdev_ops = &rtl871x_mgnt_netdev_ops;
	
#else

	pnetdev->open = mgnt_netdev_open;

	pnetdev->stop = mgnt_netdev_close;	
	
	pnetdev->hard_start_xmit = mgnt_xmit_entry;
	
	//pnetdev->set_mac_address = r871x_net_set_mac_address;
	
	//pnetdev->get_stats = r871x_net_get_stats;

	//pnetdev->do_ioctl = r871x_mp_ioctl;
	
#endif

	pnetdev->watchdog_timeo = HZ; /* 1 second timeout */	

	//pnetdev->wireless_handlers = NULL;

#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_TX
	pnetdev->features |= NETIF_F_IP_CSUM;
#endif	

	
	
	if(dev_alloc_name(pnetdev,"mgnt.wlan%d") < 0)
	{
		printk("hostapd_mode_init(): dev_alloc_name, fail! \n");		
	}


	//SET_NETDEV_DEV(pnetdev, pintfpriv->udev);


	mac[0]=0x00;
	mac[1]=0xe0;
	mac[2]=0x4c;
	mac[3]=0x87;
	mac[4]=0x11;
	mac[5]=0x12;
				
	_memcpy(pnetdev->dev_addr, mac, ETH_ALEN);
	

	netif_carrier_off(pnetdev);


	/* Tell the network stack we exist */
	if (register_netdev(pnetdev) != 0)
	{
		printk("hostapd_mode_init(): register_netdev fail!\n");
		
		if(pnetdev)
      		{	 
			free_netdev(pnetdev);
      		}
	}
	
	return 0;
	
}
void hostapd_mode_unload(_adapter *padapter)
{
	struct hostapd_priv *phostapdpriv = padapter->phostapdpriv;
	struct net_device *pnetdev = phostapdpriv->pmgnt_netdev;

	unregister_netdev(pnetdev);
	free_netdev(pnetdev);
	
}

#else

int hostapd_mode_init(_adapter *padapter)
{
	struct hostapd_priv *phostapdpriv = padapter->phostapdpriv;
	
	return 0;
}
void hostapd_mode_unload(_adapter *padapter)
{
	return;
}

#endif
#endif

