/******************************************************************************
* usb_ops_linux.c                                                                                                                                 *
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
#define _HCI_OPS_OS_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>
#include <usb_ops.h>
#include <circ_buf.h>
#include <recv_osdep.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#define	RTL871X_VENQT_READ	0xc0
#define	RTL871X_VENQT_WRITE	0x40

struct zero_bulkout_context
{
	void *pbuf;
	void *purb;
	void *pirp;
	void *padapter;
};


#define usb_write_cmd usb_write_mem
//#define usb_read_cmd usb_read_mem
#define usb_write_cmd_complete usb_write_mem_complete
//#define usb_read_cmd_complete usb_read_mem_complete

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)) || (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18))
#define usb_bulkout_zero_complete(purb, regs)	usb_bulkout_zero_complete(purb)
#define usb_write_mem_complete(purb, regs)	usb_write_mem_complete(purb)
#define usb_write_port_complete(purb, regs)	usb_write_port_complete(purb)
#define usb_read_port_complete(purb, regs)	usb_read_port_complete(purb)
#define _usbctrl_vendorreq_async_callback(urb, regs)	_usbctrl_vendorreq_async_callback(urb)
#endif

uint usb_init_intf_priv(struct intf_priv *pintfpriv)
{
	struct dvobj_priv *pdvobj = (struct dvobj_priv *)pintfpriv->intf_dev;
	_adapter * padapter=pdvobj->padapter;

_func_enter_;

	pintfpriv->intf_status = _IOREADY;
	//pintfpriv->max_xmitsz =  512;
	//pintfpriv->max_recvsz =  512;
	pintfpriv->io_wsz = 0;
	pintfpriv->io_rsz = 0;

	if(pdvobj->ishighspeed)
		pintfpriv->max_iosz =  128;
	else
		pintfpriv->max_iosz =  64;


       //_rwlock_init(&pintfpriv->rwlock);

	pintfpriv->allocated_io_rwmem = _malloc(pintfpriv->max_iosz +4);

    	if (pintfpriv->allocated_io_rwmem == NULL)
    		goto usb_init_intf_priv_fail;

	pintfpriv->io_rwmem = pintfpriv->allocated_io_rwmem +  4\
						- ( (u32)(pintfpriv->allocated_io_rwmem) & 3);


	_init_timer(&pintfpriv->io_timer, padapter->pnetdev, io_irp_timeout_handler, pintfpriv);

	pintfpriv->piorw_urb = usb_alloc_urb(0, GFP_ATOMIC);
	if(pintfpriv->piorw_urb==NULL)
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("pintfpriv->piorw_urb==NULL!!!\n"));
		goto usb_init_intf_priv_fail;
	}

	_init_sema(&(pintfpriv->io_retevt), 0);

	pintfpriv->io_irp_cnt = 1;
	pintfpriv->bio_irp_pending = _FALSE;

_func_exit_;

	return _SUCCESS;

usb_init_intf_priv_fail:

	if (pintfpriv->allocated_io_rwmem)
	{
		_mfree((u8 *)(pintfpriv->allocated_io_rwmem), pintfpriv->max_iosz +4);
	}

	if(pintfpriv->piorw_urb)
	{
		usb_free_urb(pintfpriv->piorw_urb);
	}

_func_exit_;

	return _FAIL;

}

void usb_unload_intf_priv(struct intf_priv *pintfpriv)
{

_func_enter_;

	RT_TRACE(_module_hci_ops_os_c_,_drv_info_,("+usb_unload_intf_priv\n"));

	RT_TRACE(_module_hci_ops_os_c_,_drv_info_,("pintfpriv->io_irp_cnt=%d\n", pintfpriv->io_irp_cnt));

	pintfpriv->io_irp_cnt--;
	if(pintfpriv->io_irp_cnt)
	{
		if(pintfpriv->bio_irp_pending==_TRUE)
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("kill iorw_urb\n"));
			usb_kill_urb(pintfpriv->piorw_urb);
		}

		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("wait io_retevt\n"));

		_down_sema(&(pintfpriv->io_retevt));
	}

	RT_TRACE(_module_hci_ops_os_c_,_drv_info_,("cancel io_urb ok\n"));

	_mfree((u8 *)(pintfpriv->allocated_io_rwmem), pintfpriv->max_iosz+4);

	if(pintfpriv->piorw_urb)
	{
		usb_free_urb(pintfpriv->piorw_urb);
	}

	RT_TRACE(_module_hci_ops_os_c_,_drv_info_,("-usb_unload_intf_priv\n"));

_func_exit_;

}

int ffaddr2pipehdl(struct dvobj_priv *pdvobj, u32 addr)
{
	int pipe=0, ep_num=0;
	_adapter *padapter = pdvobj->padapter;
	struct usb_device *pusbd = pdvobj->pusbdev;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);

	if(addr == RECV_BULK_IN_ADDR)
	{
		pipe=usb_rcvbulkpipe(pusbd, pHalData->RtBulkInPipes);

		return pipe;
	}

	if(addr == RECV_INT_IN_ADDR)
	{
		pipe=usb_rcvbulkpipe(pusbd, pHalData->RtIntInPipes);

		return pipe;
	}

	if(addr < HW_QUEUE_ENTRY)
	{
		//ep_num = (pHalData->Queue2EPNum[(u8)addr] & 0x0f);
		ep_num = pHalData->Queue2EPNum[addr];

		pipe = usb_sndbulkpipe(pusbd, ep_num);

		return pipe;
	}

	return pipe;

}

int urb_zero_packet_chk(_adapter *padapter, int sz)
{
	int blnSetTxDescOffset;
	struct dvobj_priv	*pdvobj = (struct dvobj_priv*)&padapter->dvobjpriv;	

	if ( pdvobj->ishighspeed )
	{
		if ( ( (sz + TXDESC_SIZE) % 512 ) == 0 ) {
			blnSetTxDescOffset = 1;
		} else {
			blnSetTxDescOffset = 0;
		}
	}
	else
	{
		if ( ( (sz + TXDESC_SIZE) % 64 ) == 0 ) 	{
			blnSetTxDescOffset = 1;
		} else {
			blnSetTxDescOffset = 0;
		}
	}
	
	return blnSetTxDescOffset;
	
}

static void usb_bulkout_zero_complete(struct urb *purb, struct pt_regs *regs)
{
	struct zero_bulkout_context *pcontext = (struct zero_bulkout_context *)purb->context;

	//printk("+usb_bulkout_zero_complete\n");

	if(pcontext)
	{
		if(pcontext->pbuf)
		{
			_mfree(pcontext->pbuf, sizeof(int));
		}

		if(pcontext->purb && (pcontext->purb==purb))
		{
			usb_free_urb(pcontext->purb);
		}


		_mfree((u8*)pcontext, sizeof(struct zero_bulkout_context));
	}


}

u32 usb_bulkout_zero(struct intf_hdl *pintfhdl, u32 addr)
{
	int pipe, status, len;
	u32 ret;
	unsigned char *pbuf;
	struct zero_bulkout_context *pcontext;
	PURB	purb = NULL;
	_adapter *padapter = (_adapter *)pintfhdl->adapter;
	struct dvobj_priv *pdvobj = (struct dvobj_priv *)&padapter->dvobjpriv;
	struct usb_device *pusbd = pdvobj->pusbdev;

	//printk("+usb_bulkout_zero\n");


	if((padapter->bDriverStopped) || (padapter->bSurpriseRemoved) ||(padapter->pwrctrlpriv.pnp_bstop_trx))
	{
		return _FAIL;
	}


	pcontext = (struct zero_bulkout_context *)_malloc(sizeof(struct zero_bulkout_context));

	pbuf = (unsigned char *)_malloc(sizeof(int));
	purb = usb_alloc_urb(0, GFP_ATOMIC);

	len = 0;
	pcontext->pbuf = pbuf;
	pcontext->purb = purb;
	pcontext->pirp = NULL;
	pcontext->padapter = padapter;


	//translate DMA FIFO addr to pipehandle
	//pipe = ffaddr2pipehdl(pdvobj, addr);

	usb_fill_bulk_urb(purb, pusbd, pipe,
					pbuf,
					len,
					usb_bulkout_zero_complete,
					pcontext);//context is pcontext

	status = usb_submit_urb(purb, GFP_ATOMIC);

	if (!status)
	{
		ret= _SUCCESS;
	}
	else
	{
		ret= _FAIL;
	}


	return _SUCCESS;

}

void usb_read_mem(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	_func_enter_;



	_func_exit_;
}

static void usb_write_mem_complete(struct urb *purb, struct pt_regs *regs)
{
	_irqL irqL;
	_list	*head, *plist;
	struct io_req	*pio_req;
	struct io_queue *pio_q = (struct io_queue *)purb->context;
	struct intf_hdl *pintf = &(pio_q->intf);
	struct intf_priv *pintfpriv = pintf->pintfpriv;
	_adapter *padapter = (_adapter *)pintf->adapter;

_func_enter_;

	RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("+usb_write_mem_complete\n"));

	head = &(pio_q->processing);

	_enter_critical(&(pio_q->lock), &irqL);

	pintfpriv->bio_irp_pending=_FALSE;
	pintfpriv->io_irp_cnt--;
	if(pintfpriv->io_irp_cnt ==0)
	{
		_up_sema(&(pintfpriv->io_retevt));
	}

	if(padapter->bSurpriseRemoved || padapter->bDriverStopped)
	{
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("usb_write_mem_complete:bDriverStopped(%d) OR bSurpriseRemoved(%d)", padapter->bDriverStopped, padapter->bSurpriseRemoved));
	}

	if(purb->status==0)
	{

	}
	else
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_mem_complete : purb->status(%d) != 0 \n", purb->status));

		if(purb->status == (-ESHUTDOWN))
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_mem_complete: ESHUTDOWN\n"));

			padapter->bDriverStopped=_TRUE;

			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_mem_complete:bDriverStopped=TRUE\n"));

		}
		else
		{
			padapter->bSurpriseRemoved=_TRUE;

			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_mem_complete:bSurpriseRemoved=TRUE\n"));
		}

	}


	//free irp in processing list...
	while(is_list_empty(head) != _TRUE)
	{
		plist = get_next(head);
		list_delete(plist);
		pio_req = LIST_CONTAINOR(plist, struct io_req, list);
		_up_sema(&pio_req->sema);
	}

	_exit_critical(&(pio_q->lock), &irqL);

	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("-usb_write_mem_complete\n"));

_func_exit_;

}
void usb_write_mem(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{
	_irqL	irqL;
	int status, pipe;
	struct io_req *pio_req = NULL;
	_adapter *padapter = (_adapter *)pintfhdl->adapter;
	struct intf_priv *pintfpriv = pintfhdl->pintfpriv;
	struct io_queue *pio_queue = (struct io_queue *)padapter->pio_queue;
	struct dvobj_priv *pdvobj = (struct dvobj_priv *)pintfpriv->intf_dev;
	struct usb_device *pusbd = pdvobj->pusbdev;
	PURB piorw_urb = pintfpriv->piorw_urb;

_func_enter_;

	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("+usb_write_mem\n"));

	if((padapter->bDriverStopped) || (padapter->bSurpriseRemoved) ||(padapter->pwrctrlpriv.pnp_bstop_trx))
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_mem:( padapter->bDriverStopped ||padapter->bSurpriseRemoved ||adapter->pwrctrlpriv.pnp_bstop_trx)!!!\n"));
		goto exit;
	}

	pio_req = alloc_ioreq(pio_queue);
	if(pio_req == NULL)
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_mem(), pio_req==NULL\n"));
		goto exit;
	}

	//translate DMA FIFO addr to pipehandle
	//pipe = ffaddr2pipehdl(pdvobj, addr);
	if(pipe==0)
	{
	   RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_mem, pipe=%x\n", pipe));
		goto exit;
	}

	_enter_critical(&(pio_queue->lock), &irqL);

	list_insert_tail(&(pio_req->list), &(pio_queue->processing));

	pintfpriv->io_irp_cnt++;
	pintfpriv->bio_irp_pending=_TRUE;


	usb_fill_bulk_urb(piorw_urb, pusbd, pipe,
				    wmem,
				    cnt,
				    usb_write_mem_complete,
				    pio_queue);


	piorw_urb->transfer_flags |= URB_ZERO_PACKET;

	status = usb_submit_urb(piorw_urb, GFP_ATOMIC);

	if (!status)
	{

	}
	else
	{
		//TODO:
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("usb_write_mem(): usb_submit_urb err, status=%x\n", status));
	}

	_exit_critical(&(pio_queue->lock), &irqL);

	_down_sema(&pio_req->sema);

	free_ioreq(pio_req, pio_queue);

	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("-usb_write_mem\n"));

exit:

_func_exit_;

}

static int recvbuf2recvframe(_adapter *padapter, struct sk_buff *pskb)
{
	u8 *pbuf;
	uint pkt_len, pkt_offset;
	int transfer_len;
	struct recv_stat *prxstat;
	u16 pkt_cnt, drvinfo_sz;
	_pkt *pkt_copy = NULL;
	union recv_frame *precvframe = NULL;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	struct recv_priv *precvpriv = &padapter->recvpriv;
	_queue *pfree_recv_queue = &precvpriv->free_recv_queue;


	transfer_len = pskb->len;
	pbuf = pskb->data;

	prxstat = (struct recv_stat *)pbuf;
	pkt_cnt = (le32_to_cpu(prxstat->rxdw2)>>16) & 0xff;

#if 0 //temp remove when disable usb rx aggregation
	if((pkt_cnt > 10) || (pkt_cnt < 1) || (transfer_len<RXDESC_SIZE) ||(pkt_len<=0))
	{
		return _FAIL;
	}
#endif

	do {
		RT_TRACE(_module_rtl871x_recv_c_, _drv_info_,
			 ("recvbuf2recvframe: rxdesc=offsset 0:0x%08x, 4:0x%08x, 8:0x%08x, C:0x%08x\n",
			  prxstat->rxdw0, prxstat->rxdw1, prxstat->rxdw2, prxstat->rxdw4));

		prxstat = (struct recv_stat *)pbuf;
		pkt_len =  le32_to_cpu(prxstat->rxdw0) & 0x00003fff;
		
		if((pkt_len<=0) || (pkt_len>transfer_len))
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("recvbuf2recvframe: pkt_len<=0\n"));
			goto _exit_recvbuf2recvframe;
		}

		drvinfo_sz = (le32_to_cpu(prxstat->rxdw0) & 0x000f0000) >> 16;//uint 2^3 = 8 bytes
		drvinfo_sz = drvinfo_sz << 3;
		RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("recvbuf2recvframe: DRV_INFO_SIZE=%d\n", drvinfo_sz));

		precvframe = alloc_recvframe(pfree_recv_queue);
		if (precvframe == NULL)
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("recvbuf2recvframe: precvframe==NULL\n"));
			goto _exit_recvbuf2recvframe;
		}

		_init_listhead(&precvframe->u.hdr.list);
		precvframe->u.hdr.precvbuf = NULL;	//can't access the precvbuf for new arch.
		precvframe->u.hdr.len = 0;

#if USB_RX_AGGREGATION_92C
		switch (pHalData->UsbRxAggMode)
		{
			case USB_RX_AGG_DMA:
			case USB_RX_AGG_DMA_USB:
				pkt_offset = (u16)_RND128(pkt_len + drvinfo_sz + RXDESC_SIZE);
				break;
			case USB_RX_AGG_USB:
				pkt_offset = (u16)_RND4(pkt_len + drvinfo_sz + RXDESC_SIZE);
				break;
			case USB_RX_AGG_DISABLE:
			default:
				pkt_offset = pkt_len + drvinfo_sz + RXDESC_SIZE;
				break;
		}
#else
		pkt_offset = pkt_len + drvinfo_sz + RXDESC_SIZE;
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)) // http://www.mail-archive.com/netdev@vger.kernel.org/msg17214.html
		pkt_copy = dev_alloc_skb(pkt_offset>1600?pkt_offset:1600);
#else
		pkt_copy = netdev_alloc_skb(padapter->pnetdev, pkt_offset>1600?pkt_offset:1600);
#endif
		if (pkt_copy)
		{
			_memcpy(pkt_copy->data, pbuf, pkt_offset);
			precvframe->u.hdr.pkt = pkt_copy;
			precvframe->u.hdr.rx_head = precvframe->u.hdr.rx_data = precvframe->u.hdr.rx_tail = pkt_copy->data;
			precvframe->u.hdr.rx_end = pkt_copy->data + (pkt_offset>1600?pkt_offset:1600);
		}
		else
		{
			//printk("recvbuf2recvframe:can not allocate memory for skb copy\n");
			precvframe->u.hdr.pkt = skb_clone(pskb, GFP_ATOMIC);
			precvframe->u.hdr.rx_head = precvframe->u.hdr.rx_data = precvframe->u.hdr.rx_tail = pbuf;
			precvframe->u.hdr.rx_end = pbuf + (pkt_offset>1600?pkt_offset:1600);
		}

		recvframe_put(precvframe, pkt_len + drvinfo_sz + RXDESC_SIZE);
		recvframe_pull(precvframe, drvinfo_sz + RXDESC_SIZE);

		//because the endian issue, driver avoid reference to the rxstat after calling update_recvframe_attrib_from_recvstat();
		update_recvframe_attrib_from_recvstat(precvframe, prxstat);

		if (recv_entry(precvframe) != _SUCCESS)
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("recvbuf2recvframe: recv_entry(precvframe) != _SUCCESS\n"));
		}

		pkt_cnt--;
		transfer_len -= pkt_offset;
		pbuf += pkt_offset;
		precvframe = NULL;
		pkt_copy = NULL;

		if(transfer_len>0 && pkt_cnt==0)
			pkt_cnt = (le32_to_cpu(prxstat->rxdw2)>>16) & 0xff;

		
	}while ((transfer_len > 0) && (pkt_cnt > 0));

_exit_recvbuf2recvframe:

	return _SUCCESS;
}

void recv_tasklet(void *priv)
{
	struct sk_buff *pskb;
	_adapter *padapter = (_adapter*)priv;
	struct recv_priv *precvpriv = &padapter->recvpriv;

	while (NULL != (pskb = skb_dequeue(&precvpriv->rx_skb_queue)))
	{
		recvbuf2recvframe(padapter, pskb);

#ifdef CONFIG_PREALLOC_RECV_SKB

		pskb->tail = pskb->data;
		pskb->len = 0;

		skb_queue_tail(&precvpriv->free_recv_skb_queue, pskb);

#else
		dev_kfree_skb_any(pskb);
#endif

	}
	
}


static void usb_read_port_complete(struct urb *purb, struct pt_regs *regs)
{
	_irqL irqL;
	uint isevt, *pbuf;
	struct recv_buf	*precvbuf = (struct recv_buf *)purb->context;
	_adapter			*padapter = (_adapter *)precvbuf->adapter;
	struct recv_priv	*precvpriv = &padapter->recvpriv;

	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete!!!\n"));

	//_enter_critical(&precvpriv->lock, &irqL);
	//precvbuf->irp_pending = _FALSE;
	//precvpriv->rx_pending_cnt--;
	//_exit_critical(&precvpriv->lock, &irqL);

	//if (precvpriv->rx_pending_cnt == 0)
	//{
	//	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete: rx_pending_cnt== 0, set allrxreturnevt!\n"));
	//	_up_sema(&precvpriv->allrxreturnevt);
	//}

	if(padapter->bSurpriseRemoved || padapter->bDriverStopped)
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete:bDriverStopped(%d) OR bSurpriseRemoved(%d)\n", padapter->bDriverStopped, padapter->bSurpriseRemoved));
		goto exit;
	}

	if (purb->status == 0)//SUCCESS
	{
		if ((purb->actual_length > MAX_RECVBUF_SZ) || (purb->actual_length < RXDESC_SIZE))
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete: (purb->actual_length > MAX_RECVBUF_SZ) || (purb->actual_length < RXDESC_SIZE)\n"));
			precvbuf->reuse = _TRUE;
			read_port(padapter, precvpriv->ff_hwaddr, 0, (unsigned char *)precvbuf);
		}
		else
		{
			precvbuf->transfer_len = purb->actual_length;

			skb_put(precvbuf->pskb, purb->actual_length);
			skb_queue_tail(&precvpriv->rx_skb_queue, precvbuf->pskb);

			if (skb_queue_len(&precvpriv->rx_skb_queue)<=1)
				tasklet_hi_schedule(&precvpriv->recv_tasklet);

			precvbuf->pskb = NULL;
			precvbuf->reuse = _FALSE;
			read_port(padapter, precvpriv->ff_hwaddr, 0, (unsigned char *)precvbuf);
		}
	}
	else
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete : purb->status(%d) != 0 \n", purb->status));

		if (purb->status == (-EPROTO))
		{
			precvbuf->reuse = _TRUE;
			read_port(padapter, precvpriv->ff_hwaddr, 0, (unsigned char *)precvbuf);
		}
		else if (purb->status == (-ESHUTDOWN))
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete: ESHUTDOWN\n"));

			padapter->bDriverStopped=_TRUE;

			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete:bDriverStopped=TRUE\n"));
		}
		else
		{
			padapter->bSurpriseRemoved=_TRUE;

			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port_complete:bSurpriseRemoved=TRUE\n"));
		}
	}

exit:

_func_exit_;

}

u32 usb_read_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *rmem)
{
	_irqL irqL;
	int err, pipe;
	u32 tmpaddr = 0;
        int alignment = 0;
	u32 ret = _SUCCESS;
	PURB purb = NULL;
	struct recv_buf	*precvbuf = (struct recv_buf *)rmem;
	struct intf_priv	*pintfpriv = pintfhdl->pintfpriv;
	struct dvobj_priv	*pdvobj = (struct dvobj_priv *)pintfpriv->intf_dev;
	_adapter			*adapter = (_adapter *)pdvobj->padapter;
	struct recv_priv	*precvpriv = &adapter->recvpriv;
	struct usb_device	*pusbd = pdvobj->pusbdev;


_func_enter_;

	if (adapter->bDriverStopped || adapter->bSurpriseRemoved || adapter->pwrctrlpriv.pnp_bstop_trx)
	{
		RT_TRACE(_module_hci_ops_os_c_, _drv_err_, ("usb_read_port:( padapter->bDriverStopped ||padapter->bSurpriseRemoved || adapter->pwrctrlpriv.pnp_bstop_trx)!!!\n"));
		return _FAIL;
	}

#ifdef CONFIG_PREALLOC_RECV_SKB
	if ((precvbuf->reuse == _FALSE) || (precvbuf->pskb == NULL))
	{
		if (NULL != (precvbuf->pskb = skb_dequeue(&precvpriv->free_recv_skb_queue)))
		{
			precvbuf->reuse = _TRUE;
		}
	}
#endif


	if (precvbuf != NULL)
	{
		init_recvbuf(adapter, precvbuf);

		//re-assign for linux based on skb
		if ((precvbuf->reuse == _FALSE) || (precvbuf->pskb == NULL))
		{
			//precvbuf->pskb = alloc_skb(MAX_RECVBUF_SZ, GFP_ATOMIC);//don't use this after v2.6.25
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)) // http://www.mail-archive.com/netdev@vger.kernel.org/msg17214.html
			precvbuf->pskb = dev_alloc_skb(MAX_RECVBUF_SZ + RECVBUFF_ALIGN_SZ);
#else
			precvbuf->pskb = netdev_alloc_skb(adapter->pnetdev, MAX_RECVBUF_SZ + RECVBUFF_ALIGN_SZ);
#endif
			if (precvbuf->pskb == NULL)
			{
				RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("init_recvbuf(): alloc_skb fail!\n"));
				return _FAIL;
			}

			tmpaddr = (u32)precvbuf->pskb->data;
			alignment = tmpaddr & (RECVBUFF_ALIGN_SZ-1);
			skb_reserve(precvbuf->pskb, (RECVBUFF_ALIGN_SZ - alignment));

			precvbuf->phead = precvbuf->pskb->head;
			precvbuf->pdata = precvbuf->pskb->data;
			precvbuf->ptail = precvbuf->pskb->tail;
			precvbuf->pend = precvbuf->pskb->end;

			precvbuf->pbuf = precvbuf->pskb->data;
		}
		else//reuse skb
		{
			precvbuf->phead = precvbuf->pskb->head;
			precvbuf->pdata = precvbuf->pskb->data;
			precvbuf->ptail = precvbuf->pskb->tail;
			precvbuf->pend = precvbuf->pskb->end;

			precvbuf->pbuf = precvbuf->pskb->data;

			precvbuf->reuse = _FALSE;
		}

		//_enter_critical(&precvpriv->lock, &irqL);
		//precvpriv->rx_pending_cnt++;
		//precvbuf->irp_pending = _TRUE;
		//_exit_critical(&precvpriv->lock, &irqL);

		purb = precvbuf->purb;

		//translate DMA FIFO addr to pipehandle
		pipe = ffaddr2pipehdl(pdvobj, addr);

		usb_fill_bulk_urb(purb, pusbd, pipe,
						precvbuf->pbuf,
						MAX_RECVBUF_SZ,
						usb_read_port_complete,
						precvbuf);//context is precvbuf

		err = usb_submit_urb(purb, GFP_ATOMIC);
		if((err) && (err != (-EPERM)))
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("cannot submit rx in-token(err=0x%.8x), URB_STATUS =0x%.8x", err, purb->status));
			ret = _FAIL;
		}
	}
	else
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_read_port:precvbuf ==NULL\n"));
		ret = _FAIL;
	}

_func_exit_;

	return ret;
}

void usb_read_port_cancel(_adapter *padapter)
{
	int i;	

	struct recv_buf *precvbuf;	

	precvbuf = (struct recv_buf *)padapter->recvpriv.precv_buf;	

	printk("usb_read_port_cancel \n");
		
	for(i=0; i < NR_RECVBUFF ; i++)	
	{		
		precvbuf->reuse == _TRUE;		
		if(precvbuf->purb)		
		{
			//printk("usb_read_port_cancel : usb_kill_urb \n");			
			usb_kill_urb(precvbuf->purb);		
		}		

		precvbuf++;		
	}

}

void xmit_tasklet(void *priv)
{
	int ret = _FALSE;
	_adapter *padapter = (_adapter*)priv;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;

	while(1)
	{
		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{
			printk("xmit_tasklet => bDriverStopped or bSurpriseRemoved \n");
			break;
		}

		ret = xmitframe_complete(padapter, pxmitpriv, NULL);

		if(ret==_FALSE)
			break;

	}

}


static void usb_write_port_complete(struct urb *purb, struct pt_regs *regs)
{
	_irqL irqL;
	int i;
	struct xmit_buf *pxmitbuf = (struct xmit_buf *)purb->context;
	struct xmit_frame *pxmitframe = (struct xmit_frame *)pxmitbuf->priv_data;
	_adapter			*padapter = pxmitframe->padapter;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	struct pkt_attrib *pattrib = &pxmitframe->attrib;

_func_enter_;

	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("+usb_write_port_complete\n"));

/*
	_enter_critical(&pxmitpriv->lock, &irqL);

	pxmitpriv->txirp_cnt--;

	switch(pattrib->priority)
	{
		case 1:
		case 2:
			pxmitpriv->bkq_cnt--;
			//printk("pxmitpriv->bkq_cnt=%d\n", pxmitpriv->bkq_cnt);
			break;
		case 4:
		case 5:
			pxmitpriv->viq_cnt--;
			//printk("pxmitpriv->viq_cnt=%d\n", pxmitpriv->viq_cnt);
			break;
		case 6:
		case 7:
			pxmitpriv->voq_cnt--;
			//printk("pxmitpriv->voq_cnt=%d\n", pxmitpriv->voq_cnt);
			break;
		case 0:
		case 3:
		default:
			pxmitpriv->beq_cnt--;
			//printk("pxmitpriv->beq_cnt=%d\n", pxmitpriv->beq_cnt);
			break;

	}

	_exit_critical(&pxmitpriv->lock, &irqL);


	if(pxmitpriv->txirp_cnt==0)
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete: txirp_cnt== 0, set allrxreturnevt!\n"));
		_up_sema(&(pxmitpriv->tx_retevt));
	}
*/

	if(padapter->bSurpriseRemoved || padapter->bDriverStopped)
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete:bDriverStopped(%d) OR bSurpriseRemoved(%d)", padapter->bDriverStopped, padapter->bSurpriseRemoved));
		goto exit;
	}

	if(purb->status==0)
	{

	}
	else
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete : purb->status(%d) != 0 \n", purb->status));

		if((purb->status==-EPIPE)||(purb->status==-EPROTO))
		{
			//usb_clear_halt(pusbdev, purb->pipe);
			//msleep(10);
		}
		else if(purb->status == (-ESHUTDOWN))
		{
			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete: ESHUTDOWN\n"));

			padapter->bDriverStopped=_TRUE;

			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete:bDriverStopped=TRUE\n"));

			goto exit;
		}
		else
		{
			padapter->bSurpriseRemoved=_TRUE;

			RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port_complete:bSurpriseRemoved=TRUE\n"));

			goto exit;
		}



	}

	free_xmitframe_ex(pxmitpriv, pxmitframe);

	free_xmitbuf(pxmitpriv, pxmitbuf);


	if(txframes_pending(padapter))
	{
		tasklet_hi_schedule(&pxmitpriv->xmit_tasklet);
	}



	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("-usb_write_port_complete\n"));

exit:

_func_exit_;

}

u32 usb_write_port(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *wmem)
{
	_irqL irqL;
	int pipe, status;
	u32 ret, bwritezero = _FALSE;
	PURB	purb = NULL;
	_adapter *padapter = (_adapter *)pintfhdl->adapter;
	struct dvobj_priv	*pdvobj = (struct dvobj_priv   *)&padapter->dvobjpriv;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	struct xmit_buf *pxmitbuf = (struct xmit_buf *)wmem;
	struct xmit_frame *pxmitframe = (struct xmit_frame *)pxmitbuf->priv_data;
	struct usb_device *pusbd = pdvobj->pusbdev;
	struct pkt_attrib *pattrib = &pxmitframe->attrib;

_func_enter_;

	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("+usb_write_port\n"));

	if((padapter->bDriverStopped) || (padapter->bSurpriseRemoved) ||(padapter->pwrctrlpriv.pnp_bstop_trx))
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port:( padapter->bDriverStopped ||padapter->bSurpriseRemoved ||adapter->pwrctrlpriv.pnp_bstop_trx)!!!\n"));
		return _FAIL;
	}

/*
	_enter_critical(&pxmitpriv->lock, &irqL);

	//total irp
	pxmitpriv->txirp_cnt++;

	//per ac irp
	switch(pattrib->priority)
	{
		case 1:
		case 2:
			pxmitpriv->bkq_cnt++;
			break;
		case 4:
		case 5:
			pxmitpriv->viq_cnt++;
			break;
		case 6:
		case 7:
			pxmitpriv->voq_cnt++;
			break;
		case 0:
		case 3:
		default:
			pxmitpriv->beq_cnt++;
			break;
	}


	_exit_critical(&pxmitpriv->lock, &irqL);
*/

	purb	= pxmitbuf->pxmit_urb[0];

#if 0
	if(pdvobj->ishighspeed)
	{
		if(cnt> 0 && cnt%512 == 0)
		{
			//printk("ishighspeed, cnt=%d\n", cnt);
			bwritezero = _TRUE;
		}
	}
	else
	{
		if(cnt > 0 && cnt%64 == 0)
		{
			//printk("cnt=%d\n", cnt);
			bwritezero = _TRUE;
		}
	}
#endif

	//translate DMA FIFO addr to pipehandle
	pipe = ffaddr2pipehdl(pdvobj, addr);

#ifdef CONFIG_REDUCE_USB_TX_INT
	if ( pxmitpriv->free_xmitbuf_cnt%NR_XMITBUFF == 0 )
	{
		purb->transfer_flags  &=  (~URB_NO_INTERRUPT);
	} else {
		purb->transfer_flags  |=  URB_NO_INTERRUPT;
		//printk("URB_NO_INTERRUPT ");
	}
#endif


	usb_fill_bulk_urb(purb, pusbd, pipe,
       				pxmitframe->buf_addr,
					cnt,
					usb_write_port_complete,
					pxmitbuf);//context is pxmitbuf
#if 0
	if (bwritezero)
	{
		purb->transfer_flags |= URB_ZERO_PACKET;
	}
#endif

	status = usb_submit_urb(purb, GFP_ATOMIC);

	if (!status)
	{
		ret= _SUCCESS;
	}
	else
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("usb_write_port(): usb_submit_urb, status=%x\n", status));
		ret= _FAIL;
	}

//   Commented by Albert 2009/10/13
//   We add the URB_ZERO_PACKET flag to urb so that the host will send the zero packet automatically.
/*
	if(bwritezero == _TRUE)
	{
		usb_bulkout_zero(pintfhdl, addr);
	}
*/

_func_exit_;

	RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("-usb_write_port\n"));

	return ret;

}

void usb_write_port_cancel(_adapter *padapter)
{
	int i, j;
	struct xmit_buf *pxmitbuf = (struct xmit_buf *)padapter->xmitpriv.pxmitbuf;

	printk("usb_write_port_cancel \n");
	
	for(i=0; i<NR_XMITBUFF; i++)
	{
		for(j=0; j<8; j++)
		{
		        if(pxmitbuf->pxmit_urb[j])
		        {
		                usb_kill_urb(pxmitbuf->pxmit_urb[j]);
		        }
		}
		
		pxmitbuf++;
	}

}

int usbctrl_vendorreq(struct intf_priv *pintfpriv, u8 request, u16 value, u16 index, void *pdata, u16 len, u8 requesttype)
{
	unsigned int pipe;
	int status;
	u8 reqtype;

	struct dvobj_priv  *pdvobjpriv = (struct dvobj_priv  *)pintfpriv->intf_dev;
	struct usb_device *udev=pdvobjpriv->pusbdev;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pdvobjpriv->padapter);

	// Added by Albert 2010/02/09
	// For mstar platform, mstar suggests the address for USB IO should be 16 bytes alignment.
	// Trying to fix it here.

	u8 *palloc_buf, *pIo_buf;

	palloc_buf = _malloc( (u32) len + 16);
	
	if ( palloc_buf== NULL)
	{
		printk( "[%s] Can't alloc memory for vendor request\n", __FUNCTION__ );
		return(-1);
	}
	
	pIo_buf = palloc_buf + 16 -((uint)(palloc_buf) & 0x0f );

	if(IS_HARDWARE_TYPE_8192DU(pHalData))
	{
		if((value & 0xff00) == 0xff00)
		{
			//Temply for pomelo read/write 0x00-0x100 ,will removed when the 0xffxx register eanble  zhiyuan 2009/10/23 
			value &= 0x00ff;
		}
		if (pHalData->interfaceIndex!=0)
		{
			if(value<0x1000)
				value|=0x4000;

			index = 0;

			if (pHalData->bDuringMac1InitRadioA)
			{
				value &= 0x0fff;
			}
			else if (!IS_NORMAL_CHIP( pHalData->VersionID))	
			{
				// temply add for cck control/dual-phy ad-da clock, which only valid at 0x884/0x888
				if ((value&0xffff)==0x4884 || (value&0xffff)==0x4888)
					value &= 0x0fff;
			}
		}
	}
		
	if (requesttype == 0x01)
	{
		pipe = usb_rcvctrlpipe(udev, 0);//read_in
		reqtype =  RTL871X_VENQT_READ;
	}
	else
	{
		pipe = usb_sndctrlpipe(udev, 0);//write_out
		reqtype =  RTL871X_VENQT_WRITE;
		_memcpy( pIo_buf, pdata, len);
	}

	status = usb_control_msg(udev, pipe, request, reqtype, value, index, pIo_buf, len, HZ/2);

	if (status < 0)
	{
		RT_TRACE(_module_hci_ops_os_c_,_drv_err_,("reg 0x%x, usb_read8 TimeOut! status:0x%x value=0x%x\n", value, status, *(u32*)pdata));
	}
	else if ( status > 0 )   // Success this control transfer.
	{
               if ( requesttype == 0x01 )
               {   // For Control read transfer, we have to copy the read data from pIo_buf to pdata.
                       _memcpy( pdata, pIo_buf,  status );
               }
	}

	_mfree( palloc_buf, (u32) len + 16 );

	return status;

}

#define REALTEK_USB_VENQT_MAX_BUF_SIZE	254
#define REALTEK_USB_VENQT_READ			0xC0
#define REALTEK_USB_VENQT_WRITE			0x40
#define REALTEK_USB_VENQT_CMD_REQ		0x05
#define REALTEK_USB_VENQT_CMD_IDX		0x00

enum{
	VENDOR_WRITE = 0x00,
	VENDOR_READ = 0x01,
};

static void _usbctrl_vendorreq_async_callback(struct urb *urb, struct pt_regs *regs)
{
	if(urb){
		if(urb->context){
			kfree(urb->context);
		}
	}
}

static int _usbctrl_vendorreq_async_write(struct usb_device *udev, u8 request, u16 value, u16 index, void *pdata, u16 len, u8 requesttype)
{
	int rc;
	unsigned int pipe;	
	u8 reqtype;
	struct usb_ctrlrequest *dr;
	struct urb *urb;
	struct rtl819x_async_write_data {
		u8 data[REALTEK_USB_VENQT_MAX_BUF_SIZE];
		struct usb_ctrlrequest dr;
	} *buf;
	
				
	if (requesttype == VENDOR_READ){
		pipe = usb_rcvctrlpipe(udev, 0);//read_in
		reqtype =  REALTEK_USB_VENQT_READ;		
	} 
	else {
		pipe = usb_sndctrlpipe(udev, 0);//write_out
		reqtype =  REALTEK_USB_VENQT_WRITE;		
	}		
	
	buf = kmalloc(sizeof(*buf), GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		kfree(buf);
		return -ENOMEM;
	}

	dr = &buf->dr;

	dr->bRequestType = reqtype;
	dr->bRequest = request;
	dr->wValue = cpu_to_le16(value);
	dr->wIndex = cpu_to_le16(index);
	dr->wLength = cpu_to_le16(len);

	memcpy(buf, pdata, len);

	usb_fill_control_urb(urb, udev, pipe,
			     (unsigned char *)dr, buf, len,
			     _usbctrl_vendorreq_async_callback, buf);

	rc = usb_submit_urb(urb, GFP_ATOMIC);
	if (rc < 0) {
		kfree(buf);
	}
	
	usb_free_urb(urb);

	return rc;

}

void usb_write_async(struct usb_device *udev, u32 addr, u32 val, u16 len)
{
	u8 request;
	u8 requesttype;
	u16 wvalue;
	u16 index;
	u32 data;
	
	requesttype = VENDOR_WRITE;//write_out	
	request = REALTEK_USB_VENQT_CMD_REQ;
	index = REALTEK_USB_VENQT_CMD_IDX;//n/a

	wvalue = (u16)(addr&0x0000ffff);
	data = val & (0xffffffff >> ((4 - len) * 8));
	data = cpu_to_le32(data);
	
	_usbctrl_vendorreq_async_write(udev, request, wvalue, index, &data, len, requesttype);
}
