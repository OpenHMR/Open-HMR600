/******************************************************************************
* xmit_linux.c                                                                                                                                 *
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
#define _XMIT_OSDEP_C_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


#include <if_ether.h>
#include <ip.h>
#include <rtl871x_byteorder.h>
#include <wifi.h>
#include <mlme_osdep.h>
#include <xmit_osdep.h>
#include <osdep_intf.h>
#include <circ_buf.h>
 #ifdef CONFIG_SDIO_HCI
#include <linux/mmc/sdio_func.h> 
#endif

#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_TX
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#endif

uint remainder_len(struct pkt_file *pfile)
{

	return (pfile->buf_len - ((u32)(pfile->cur_addr) - (u32)(pfile->buf_start)));

}

void _open_pktfile (_pkt *pktptr, struct pkt_file *pfile)
{
_func_enter_;

	pfile->pkt = pktptr;
	pfile->cur_addr = pfile->buf_start = pktptr->data;
	pfile->pkt_len = pfile->buf_len = pktptr->len;

	pfile->cur_buffer = pfile->buf_start ;
	
_func_exit_;
}

uint _pktfile_read (struct pkt_file *pfile, u8 *rmem, uint rlen)
{	
	uint	len = 0;
	
_func_enter_;

       len =  remainder_len(pfile);
      	len = (rlen > len)? len: rlen;

       if(rmem)
	  skb_copy_bits(pfile->pkt, pfile->buf_len-pfile->pkt_len, rmem, len);

       pfile->cur_addr += len;
       pfile->pkt_len -= len;
	   
_func_exit_;	       		

	return len;	
}

sint endofpktfile (struct pkt_file *pfile)
{
_func_enter_;
	if (pfile->pkt_len == 0){
_func_exit_;	
		return _TRUE;
	}
	else{
_func_exit_;	
		return _FALSE;
	}
}

void set_tx_chksum_offload(_pkt *pkt, struct pkt_attrib *pattrib)
{

#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_TX
	struct sk_buff *skb = (struct sk_buff *)pkt;
	pattrib->hw_tcp_csum = 0;
	
	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		if (skb_shinfo(skb)->nr_frags == 0)
		{	
                        const struct iphdr *ip = ip_hdr(skb);
                        if (ip->protocol == IPPROTO_TCP) {
                                // TCP checksum offload by HW
                                printk("CHECKSUM_PARTIAL TCP\n");
                                pattrib->hw_tcp_csum = 1;
                                //skb_checksum_help(skb);
                        } else if (ip->protocol == IPPROTO_UDP) {
                                //printk("CHECKSUM_PARTIAL UDP\n");
#if 1                       
                                skb_checksum_help(skb);
#else
                                // Set UDP checksum = 0 to skip checksum check
                                struct udphdr *udp = skb_transport_header(skb);
                                udp->check = 0;
#endif
                        } else {
				printk("%s-%d TCP CSUM offload Error!!\n", __FUNCTION__, __LINE__);
                                WARN_ON(1);     /* we need a WARN() */
			    }
		}
		else { // IP fragmentation case
			printk("%s-%d nr_frags != 0, using skb_checksum_help(skb);!!\n", __FUNCTION__, __LINE__);
                	skb_checksum_help(skb);
		}
		
	}
#endif	
	
}

void set_qos(struct pkt_file *ppktfile, struct pkt_attrib *pattrib)
{
	int i;
	struct ethhdr etherhdr;
	struct iphdr ip_hdr; 
	u16 UserPriority=0;
	
	_open_pktfile(ppktfile->pkt, ppktfile);	
	_pktfile_read(ppktfile, (unsigned char*)&etherhdr, ETH_HLEN);	
	
	//i = _pktfile_read (ppktfile, (u8*)&ip_hdr, sizeof(ip_hdr));

	// get UserPriority from IP hdr
	if(pattrib->ether_type== 0x0800)
	{		
		i = _pktfile_read (ppktfile, (u8*)&ip_hdr, sizeof(ip_hdr));
		//UserPriority = (ntohs(ip_hdr.tos) >> 5) & 0x3 ;
		UserPriority = ip_hdr.tos >> 5;
	}
	else
	{
		// "When priority processing of data frames is supported, 
		//a STA's SME should send EAPOL-Key frames at the highest priority."
		
		if(pattrib->ether_type == 0x888e)
			UserPriority = 7;
	}	
	
	pattrib->priority = UserPriority;
	pattrib->hdrlen = WLAN_HDR_A3_QOS_LEN;
	pattrib->subtype = WIFI_QOS_DATA_TYPE;
	
}

int os_xmit_resource_alloc(_adapter *padapter, struct xmit_buf *pxmitbuf)
{

#ifdef CONFIG_USB_HCI
	int i;
	
       for(i=0; i<8; i++)
      	{
      		pxmitbuf->pxmit_urb[i] = usb_alloc_urb(0, GFP_KERNEL);
             	if(pxmitbuf->pxmit_urb[i] == NULL) 
             	{
             		printk("pxmitbuf->pxmit_urb[i]==NULL");
	        	return _FAIL;	 
             	}      		  	
	
      	}
#endif

#ifdef CONFIG_SDIO_HCI

#endif

	return _SUCCESS;
	
}

void os_xmit_resource_free(_adapter *padapter, struct xmit_buf *pxmitbuf)
{

#ifdef CONFIG_USB_HCI
	int i;
	
	for(i=0; i<8; i++)
	{
		if(pxmitbuf->pxmit_urb[i])
		{
			//usb_kill_urb(pxmitbuf->pxmit_urb[i]);
			usb_free_urb(pxmitbuf->pxmit_urb[i]);
		}	
	}
#endif

#ifdef CONFIG_SDIO_HCI

#endif

}

void os_xmit_complete(_adapter *padapter, struct xmit_frame *pxframe)
{
	if(pxframe->pkt)
	{
		RT_TRACE(_module_xmit_osdep_c_,_drv_err_,("linux : os_xmit_complete, dev_kfree_skb()\n"));	

		dev_kfree_skb_any(pxframe->pkt);	
	}	

	pxframe->pkt = NULL;

}

int xmit_entry(_pkt *pkt, _nic_hdl pnetdev)
{	
	struct	xmit_frame	*pxmitframe = NULL;	
	_adapter *padapter = (_adapter *)netdev_priv(pnetdev);	
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);
	int ret=0;

_func_enter_;	

	RT_TRACE(_module_rtl871x_mlme_c_, _drv_info_, ("+xmit_enry\n"));

	if (if_up(padapter) == _FALSE)
	{
		RT_TRACE(_module_xmit_osdep_c_,_drv_err_,("xmit_entry: if_up fail \n" ));		
		ret = 0;
		goto _xmit_entry_drop;		
	}
		
	pxmitframe = alloc_xmitframe(pxmitpriv);
	if (pxmitframe == NULL)
	{	
		//printk("pxmitframe == NULL \n");
		RT_TRACE(_module_xmit_osdep_c_,_drv_err_,("pxmitframe == NULL \n"));
		
		//if (!netif_queue_stopped(pnetdev))
		//       netif_stop_queue(pnetdev);		
		
		ret = 0;			
		goto _xmit_entry_drop;
	}

	if ((update_attrib(padapter, pkt, &pxmitframe->attrib)) == _FAIL)
	{
		RT_TRACE(_module_xmit_osdep_c_,_drv_err_,("drop xmit pkt for update fail\n"));		
		ret = 0;
		goto _xmit_entry_drop;
	}	
	
	padapter->ledpriv.LedControlHandler(padapter, LED_CTL_TX);

	pxmitframe->pkt = pkt;
	if(pre_xmitframe(padapter, pxmitframe) == _TRUE)
	{		
		//dump xmitframe directly or drop xframe		
		RT_TRACE(_module_xmit_osdep_c_,_drv_err_,("xmit_entry(): dev_kfree_skb()\n"));			

		dev_kfree_skb_any(pkt);	
		
	}
			
	pxmitpriv->tx_pkts++;
	
	RT_TRACE(_module_xmit_osdep_c_,_drv_notice_,("xmit_entry:tx_pkts=%d\n", (u32)pxmitpriv->tx_pkts));		   
		
_func_exit_;	
	
	return ret;
	
_xmit_entry_drop:
	
	RT_TRACE(_module_xmit_osdep_c_,_drv_err_,("_xmit_etnry_drop\n"));

	if (pxmitframe) {
		free_xmitframe(pxmitpriv, pxmitframe);
	}
	
	pxmitpriv->tx_drop++;
	
	dev_kfree_skb_any(pkt);
	
_func_exit_;        

	return ret;
}

