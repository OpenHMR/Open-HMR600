/******************************************************************************
* rtl8192cu_xmit.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2008, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _RTL8192CU_XMIT_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtl871x_byteorder.h>
#include <wifi.h>
#include <osdep_intf.h>
#include <circ_buf.h>
#include <usb_ops.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)
#error "Shall be Linux or Windows, but not both!\n"
#endif

u32 get_ff_hwaddr(struct xmit_frame	*pxmitframe)
{
	u32 addr;
	struct pkt_attrib *pattrib = &pxmitframe->attrib;	
	
	if((pxmitframe->frame_tag&0x0f) == DATA_FRAMETAG)
	{
		switch(pattrib->qsel)
		{
			case 0:
			case 3:
				addr = BE_QUEUE_INX;
			 	break;
			case 1:
			case 2:
				addr = BK_QUEUE_INX;
				break;				
			case 4:
			case 5:
				addr = VI_QUEUE_INX;
				break;		
			case 6:
			case 7:
				addr = VO_QUEUE_INX;
				break;

			default:
				addr = BE_QUEUE_INX;
				break;		
				
		}
		
	}	
	else if((pxmitframe->frame_tag&0x0f) == MGNT_FRAMETAG)
	{
		addr = MGT_QUEUE_INX;
	}
	else
	{
		addr = BE_QUEUE_INX;
	}

	return addr;

}

void count_tx_stats(_adapter *padapter, struct xmit_frame *pxmitframe, int sz)
{
	struct sta_info *psta = NULL;
	struct stainfo_stats *pstats = NULL;
	struct pkt_attrib *pattrib = &pxmitframe->attrib;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;

	if((pxmitframe->frame_tag&0x0f) == DATA_FRAMETAG)
	{
		pxmitpriv->tx_bytes += sz;

		psta = pxmitframe->attrib.psta;

		if(psta)
		{
			pstats = &psta->sta_stats;

			pstats->tx_pkts++;
			pstats->tx_bytes += sz;
		}
	}
	
}

struct xmit_frame *dequeue_one_xmitframe(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit, struct tx_servq *ptxservq, _queue *pframe_queue)
{	
	_irqL irqL;
	struct sta_info *psta = NULL;
	_list	*xmitframe_plist, *xmitframe_phead;
	struct	xmit_frame	*pxmitframe=NULL;
	_adapter *padapter = pxmitpriv->adapter;
	
	xmitframe_phead = get_list_head(pframe_queue);
	xmitframe_plist = get_next(xmitframe_phead);

	while ((end_of_queue_search(xmitframe_phead, xmitframe_plist)) == _FALSE)
	{			
		pxmitframe = LIST_CONTAINOR(xmitframe_plist, struct xmit_frame, list);

		xmitframe_plist = get_next(xmitframe_plist);

		list_delete(&pxmitframe->list);
		
		psta = pxmitframe->attrib.psta;
				
		if(psta && psta->state == WIFI_SLEEP_STATE)
		{			
			_enter_critical(&psta->sleep_q.lock, &irqL);		
			list_insert_tail(&pxmitframe->list, get_list_head(&psta->sleep_q));
			psta->sleepq_len++;		
			_exit_critical(&psta->sleep_q.lock, &irqL);		
		}
		else
		{				
			//list_insert_tail(&pxmitframe->list, &phwxmit->pending);
	
			ptxservq->qcnt--;
			phwxmit->txcmdcnt++;	

			break;
		}	
		
		
	}
		
	return pxmitframe;
	
}

struct xmit_frame *dequeue_xframe(struct xmit_priv *pxmitpriv, struct hw_xmit *phwxmit_i, sint entry)
{	
	_irqL irqL0;
	_list	*sta_plist, *sta_phead;
	struct hw_xmit *phwxmit;
	struct tx_servq *ptxservq = NULL;
	_queue *pframe_queue = NULL;
	struct	xmit_frame	*pxmitframe=NULL;
	_adapter *padapter = pxmitpriv->adapter;
	int i, inx[4];
#ifdef CONFIG_USB_HCI
	int j, tmp, acirp_cnt[4];
#endif
	
_func_enter_;

	inx[0] = 0; inx[1] = 1; inx[2] = 2; inx[3] = 3;

/*
#ifdef CONFIG_USB_HCI
	//entry indx: 0->vo, 1->vi, 2->be, 3->bk.
	acirp_cnt[0] = pxmitpriv->voq_cnt;
	acirp_cnt[1] = pxmitpriv->viq_cnt;
	acirp_cnt[2] = pxmitpriv->beq_cnt;
	acirp_cnt[3] = pxmitpriv->bkq_cnt;

	for(i=0; i<4; i++)
	{
		for(j=i+1; j<4; j++)
		{
			if(acirp_cnt[j]<acirp_cnt[i])
			{
				tmp = acirp_cnt[i];
				acirp_cnt[i] = acirp_cnt[j];
				acirp_cnt[j] = tmp;
				
				tmp = inx[i];
				inx[i] = inx[j];
				inx[j] = tmp;
			}
		}
	}
#endif
*/

	_enter_critical_bh(&pxmitpriv->lock, &irqL0);
	
	for(i = 0; i < entry; i++) 
	{
		phwxmit = phwxmit_i + inx[i];
		
		//_enter_critical_ex(&phwxmit->sta_queue->lock, &irqL0);

		sta_phead = get_list_head(phwxmit->sta_queue);
		sta_plist = get_next(sta_phead);
		
		while ((end_of_queue_search(sta_phead, sta_plist)) == _FALSE)
		{
	
			ptxservq= LIST_CONTAINOR(sta_plist, struct tx_servq, tx_pending);
		
			pframe_queue = &ptxservq->sta_pending;
	
			pxmitframe = dequeue_one_xmitframe(pxmitpriv, phwxmit, ptxservq, pframe_queue);
			
			if(pxmitframe)
			{			
				phwxmit->accnt--;

				//Remove sta node when there is no pending packets.
				if(_queue_empty(pframe_queue)) //must be done after get_next and before break
					list_delete(&ptxservq->tx_pending);
											
				//_exit_critical_ex(&phwxmit->sta_queue->lock, &irqL0);
				
				goto exit_dequeue_xframe_ex;					
			}
						
			sta_plist = get_next(sta_plist);

		}
		
		//_exit_critical_ex(&phwxmit->sta_queue->lock, &irqL0);

	}	

exit_dequeue_xframe_ex:
	
	_exit_critical_bh(&pxmitpriv->lock, &irqL0);
	
_func_exit_;	
	
	return pxmitframe;	
}

void do_queue_select(_adapter	*padapter, struct pkt_attrib *pattrib)
{
	unsigned int qsel;
		
	qsel = (uint)pattrib->priority;	
	RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("### do_queue_select priority=%d ,qsel = %d\n",pattrib->priority ,qsel));
	pattrib->qsel = qsel;

}

void cal_txdesc_chksum(struct tx_desc	*ptxdesc)
{
		u16	*usPtr = (u16*)ptxdesc;
		u32 count = 16;		// (32 bytes / 2 bytes per XOR) => 16 times
		u32 index;
		u16 checksum = 0;

		//Clear first
		ptxdesc->txdw7 &= cpu_to_le32(0xffff0000);
	
		for(index = 0 ; index < count ; index++){
			checksum = checksum ^ *(usPtr + index);
		}

		ptxdesc->txdw7 |= cpu_to_le32(0x0000ffff&checksum);	

}

void fill_txdesc_for_mp(struct xmit_frame *pxmitframe, struct tx_desc *ptxdesc)
{		
#ifdef CONFIG_MP_INCLUDED

		struct pkt_attrib	*pattrib = &pxmitframe->attrib;
		sint bmcst = IS_MCAST(pattrib->ra);

		if (pattrib->pctrl == 1) // mp tx packets
		{

			struct tx_desc txdesc, *ptxdesc_mp;
			struct pkt_file pktfile;

			ptxdesc_mp = &txdesc;
			_open_pktfile(pxmitframe->pkt, &pktfile);
			_pktfile_read(&pktfile, NULL, ETH_HLEN);
			_pktfile_read(&pktfile, (u8*)ptxdesc_mp, TXDESC_SIZE);

			//offset 8
			ptxdesc->txdw2 = cpu_to_le32(ptxdesc_mp->txdw2);
			if (bmcst) ptxdesc->txdw2 |= cpu_to_le32(BMC);
			ptxdesc->txdw2 |= cpu_to_le32(BK);	
			//RT_TRACE(_module_rtl871x_xmit_c_,_drv_alert_,("mp pkt offset8-txdesc=0x%8x\n", ptxdesc->txdw2));			

			ptxdesc->txdw4 = cpu_to_le32(ptxdesc_mp->txdw4);
			//RT_TRACE(_module_rtl871x_xmit_c_,_drv_alert_,("mp pkt offset16-txdesc=0x%8x\n", ptxdesc->txdw4));

			//offset 20
			ptxdesc->txdw5 = cpu_to_le32(ptxdesc_mp->txdw5);
			//RT_TRACE(_module_rtl871x_xmit_c_,_drv_alert_,("mp pkt offset20-txdesc=0x%8x\n", ptxdesc->txdw5));				

			pattrib->pctrl = 0;//reset to zero;				
		}
		
#endif
}

void fill_txdesc_sectype(struct pkt_attrib *pattrib, struct tx_desc *ptxdesc)
{
	if ((pattrib->encrypt > 0) && !pattrib->bswenc)
	{
		switch (pattrib->encrypt)
		{	
			//SEC_TYPE
			case _WEP40_:
			case _WEP104_:
					ptxdesc->txdw1 |= cpu_to_le32((0x01<<22)&0x00c00000);
					break;				
			case _TKIP_:
			case _TKIP_WTMIC_:	
					//ptxdesc->txdw1 |= cpu_to_le32((0x02<<22)&0x00c00000);
					ptxdesc->txdw1 |= cpu_to_le32((0x01<<22)&0x00c00000);
					break;
			case _AES_:
					ptxdesc->txdw1 |= cpu_to_le32((0x03<<22)&0x00c00000);
					break;
			case _NO_PRIVACY_:
			default:
					break;
		
		}
		
	}

}

void fill_txdesc_vcs(struct pkt_attrib *pattrib, u32 *pdw)
{
	//DBG_8192C("cvs_mode=%d\n", pattrib->vcs_mode);	

	switch(pattrib->vcs_mode)
	{
		case RTS_CTS:
			*pdw |= cpu_to_le32(BIT(12));
			break;
		case CTS_TO_SELF:
			*pdw |= cpu_to_le32(BIT(11));
			break;
		case NONE_VCS:
		default:
			break;		
	}

}

void fill_txdesc_phy(struct pkt_attrib *pattrib, u32 *pdw)
{
	//DBG_8192C("bwmode=%d, ch_off=%d\n", pattrib->bwmode, pattrib->ch_offset);

	if(pattrib->ht_en)
	{
		*pdw |= (pattrib->bwmode&HT_CHANNEL_WIDTH_40)?	cpu_to_le32(BIT(25)):0;

		if(pattrib->ch_offset == HAL_PRIME_CHNL_OFFSET_LOWER)
			*pdw |= cpu_to_le32((0x01<<20)&0x003f0000);
		else if(pattrib->ch_offset == HAL_PRIME_CHNL_OFFSET_UPPER)
			*pdw |= cpu_to_le32((0x02<<20)&0x003f0000);
		else if(pattrib->ch_offset == HAL_PRIME_CHNL_OFFSET_DONT_CARE)
			*pdw |= 0;
		else
			*pdw |= cpu_to_le32((0x03<<20)&0x003f0000);
	}
}

int update_txdesc(struct xmit_frame *pxmitframe, uint *pmem, int sz)
{
	int pull=0;
	uint qsel;
	_adapter		*padapter = pxmitframe->padapter;
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;		
	struct pkt_attrib	*pattrib = &pxmitframe->attrib;
	struct tx_desc	*ptxdesc = (struct tx_desc *)pmem;
	sint bmcst = IS_MCAST(pattrib->ra);
	struct ht_priv *phtpriv = &pmlmepriv->htpriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &padapter->mlmeextpriv.mlmext_info;

	if(urb_zero_packet_chk(padapter, sz)==0)
	{
		ptxdesc = (struct tx_desc *)(pmem+(PACKET_OFFSET_SZ>>2));
		pull = 1;
	}
	
		
	_memset(ptxdesc, 0, sizeof(struct tx_desc));
	
	//offset 0
	ptxdesc->txdw0 |= cpu_to_le32(sz&0x0000ffff);
	ptxdesc->txdw0 |= cpu_to_le32(OWN | FSG | LSG);
	ptxdesc->txdw0 |= cpu_to_le32(((TXDESC_SIZE+OFFSET_SZ)<<OFFSET_SHT)&0x00ff0000);//32 bytes for TX Desc
	
	if(bmcst)	
	{
		ptxdesc->txdw0 |= cpu_to_le32(BIT(24));
	}	

	RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("offset0-txdesc=0x%x\n", ptxdesc->txdw0));

	//offset 4
	if(!pull) ptxdesc->txdw1 |= cpu_to_le32((0x01<<26)&0xff000000);//pkt_offset, unit:8 bytes padding


	if((pxmitframe->frame_tag&0x0f) == DATA_FRAMETAG)
	{
		//printk("pxmitframe->frame_tag == DATA_FRAMETAG\n");			

		//offset 4
		ptxdesc->txdw1 |= cpu_to_le32((pattrib->mac_id-4)&0x1f);//CAM_ID(MAC_ID)

		qsel = (uint)(pattrib->qsel & 0x0000001f);
		ptxdesc->txdw1 |= cpu_to_le32((qsel << QSEL_SHT) & 0x00001f00);

		ptxdesc->txdw1 |= cpu_to_le32((pattrib->raid<< 16) & 0x000f0000);

		fill_txdesc_sectype(pattrib, ptxdesc);


		if(phtpriv->ht_option==1) //B/G/N Mode for sta mode only
		{
			if ((phtpriv->ampdu_enable == _TRUE) &&
			    (phtpriv->baddbareq_issued[pattrib->priority] == _TRUE) &&
			    (pmlmeinfo->agg_enable_bitmap & (1 << pattrib->priority)))
			{
				ptxdesc->txdw1 |= cpu_to_le32(BIT(5));//AGG EN
			}		
			else
			{
				ptxdesc->txdw1 |= cpu_to_le32(BIT(6));//AGG BK
			}
		}
		
		//offset 8
		

		//offset 12
		ptxdesc->txdw3 |= cpu_to_le32((pattrib->seqnum<<16)&0xffff0000);


		//offset 16 , offset 20
		if (pattrib->qos_en)
			ptxdesc->txdw4 |= cpu_to_le32(BIT(6));//QoS

		if ((pattrib->ether_type != 0x888e) && (pattrib->ether_type != 0x0806) && (pattrib->dhcp_pkt != 1))
		{
              	//Non EAP & ARP & DHCP type data packet
              	
			fill_txdesc_vcs(pattrib, &ptxdesc->txdw4);
			fill_txdesc_phy(pattrib, &ptxdesc->txdw4);

			ptxdesc->txdw5 |= cpu_to_le32(0x0001ff00);//
				
              	if(0)//for driver dbg
			{
				ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate
				
				if(pattrib->ht_en)
					ptxdesc->txdw5 |= cpu_to_le32(BIT(6));//SGI

				ptxdesc->txdw5 |= cpu_to_le32(0x00000013);//init rate - mcs7
			}

		}
		else
		{
			// EAP data packet and ARP packet.
			// Use the 1M data rate to send the EAP/ARP packet.
			// This will maybe make the handshake smooth.

			ptxdesc->txdw1 |= cpu_to_le32(BIT(6));//AGG BK
			
		   	ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate

			if(padapter->halpriv.CurrentBandType92D == BAND_ON_5G)
				ptxdesc->txdw5 |= cpu_to_le32(BIT(2));// use OFDM 6Mbps

		}
		
		//offset 24

#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_TX
		if ( pattrib->hw_tcp_csum == 1 ) {
			// ptxdesc->txdw6 = 0; // clear TCP_CHECKSUM and IP_CHECKSUM. It's zero already!!
			u8 ip_hdr_offset = 32 + pattrib->hdrlen + pattrib->iv_len + 8;
			ptxdesc->txdw7 = (1 << 31) | (ip_hdr_offset << 16);
			printk("ptxdesc->txdw7 = %08x\n", ptxdesc->txdw7);
		}
#endif

		
		fill_txdesc_for_mp(pxmitframe, ptxdesc);

                  

	}
	else if((pxmitframe->frame_tag&0x0f)== MGNT_FRAMETAG)
	{
		//printk("pxmitframe->frame_tag == MGNT_FRAMETAG\n");	
		
		//offset 4		
		ptxdesc->txdw1 |= cpu_to_le32((pattrib->mac_id-4)&0x1f);//CAM_ID(MAC_ID)
		
		qsel = (uint)(pattrib->qsel&0x0000001f);
		ptxdesc->txdw1 |= cpu_to_le32((qsel<<QSEL_SHT)&0x00001f00);

		ptxdesc->txdw1 |= cpu_to_le32((pattrib->raid<< 16) & 0x000f0000);
		
		//fill_txdesc_sectype(pattrib, ptxdesc);
		
		//offset 8		

		//offset 12
		ptxdesc->txdw3 |= cpu_to_le32((pattrib->seqnum<<16)&0xffff0000);
		
		//offset 16
		ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate
		
		//offset 20
		if(padapter->halpriv.CurrentBandType92D == BAND_ON_5G)
			ptxdesc->txdw5 |= cpu_to_le32(BIT(2));// use OFDM 6Mbps
		
	}
	else if((pxmitframe->frame_tag&0x0f) == TXAGG_FRAMETAG)
	{
		DBG_8192C("pxmitframe->frame_tag == TXAGG_FRAMETAG\n");
	}
	else
	{
		DBG_8192C("pxmitframe->frame_tag = %d\n", pxmitframe->frame_tag);
		
		//offset 4	
		ptxdesc->txdw1 |= cpu_to_le32((4)&0x1f);//CAM_ID(MAC_ID)
		
		ptxdesc->txdw1 |= cpu_to_le32((6<< 16) & 0x000f0000);//raid
		
		//offset 8		

		//offset 12
		ptxdesc->txdw3 |= cpu_to_le32((pattrib->seqnum<<16)&0xffff0000);
		
		//offset 16
		ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate
		
		//offset 20
	}

	cal_txdesc_chksum(ptxdesc);
		
	return pull;
		
}

int xmitframe_complete(_adapter *padapter, struct xmit_priv *pxmitpriv, struct xmit_buf *pxmitbuf)
{		

	struct hw_xmit *phwxmits;
	sint hwentry;
	struct xmit_frame *pxmitframe=NULL;	
	int res=_SUCCESS, xcnt = 0;

	phwxmits = pxmitpriv->hwxmits;
	hwentry = pxmitpriv->hwxmit_entry;

	RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("xmitframe_complete()\n"));

	if(pxmitbuf==NULL)
	{
		pxmitbuf = alloc_xmitbuf(pxmitpriv);		
		if(!pxmitbuf)
		{
			return _FALSE;
		}			
	}	


	do
	{		
		pxmitframe =  dequeue_xframe(pxmitpriv, phwxmits, hwentry);
		
		if(pxmitframe)
		{
			pxmitframe->pxmitbuf = pxmitbuf;				

			pxmitframe->buf_addr = pxmitbuf->pbuf;

			pxmitbuf->priv_data = pxmitframe;	

			if((pxmitframe->frame_tag&0x0f) == DATA_FRAMETAG)
			{	
				if(pxmitframe->attrib.priority<=15)//TID0~15
				{
					res = xmitframe_coalesce(padapter, pxmitframe->pkt, pxmitframe);
				}	
							
				os_xmit_complete(padapter, pxmitframe);//always return ndis_packet after xmitframe_coalesce 			
			}	

				
			RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("xmitframe_complete(): dump_xframe\n"));

			
			if(res == _SUCCESS)
			{
				dump_xframe(padapter, pxmitframe);		 
			}
			else
			{
				free_xmitframe_ex(pxmitpriv, pxmitframe);	
			}
	 			 		
			xcnt++;
			
		}
		else
		{			
			free_xmitbuf(pxmitpriv, pxmitbuf);
			return _FALSE;
		}

		break;
		
	}while(0/*xcnt < (NR_XMITFRAME >> 3)*/);

	return _TRUE;
	
}

void dump_xframe(_adapter *padapter, struct xmit_frame *pxmitframe)
{
	int t, sz, w_sz, pull=0;
	u8 *mem_addr;
	u32 ff_hwaddr;
	struct xmit_buf *pxmitbuf = pxmitframe->pxmitbuf;
	struct pkt_attrib *pattrib = &pxmitframe->attrib;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	struct security_priv *psecuritypriv = &padapter->securitypriv;


	if ((pxmitframe->frame_tag == DATA_FRAMETAG) &&
	    (pxmitframe->attrib.ether_type != 0x0806) &&
	    (pxmitframe->attrib.ether_type != 0x888e) &&
	    (pxmitframe->attrib.dhcp_pkt != 1))
		issue_addbareq_cmd(padapter, pattrib->priority);

	mem_addr = pxmitframe->buf_addr;

       RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("dump_xframe()\n"));
	
	for (t = 0; t < pattrib->nr_frags; t++)
	{
		if (t != (pattrib->nr_frags - 1))
		{
			RT_TRACE(_module_rtl871x_xmit_c_,_drv_err_,("pattrib->nr_frags=%d\n", pattrib->nr_frags));

			sz = pxmitpriv->frag_len;
			sz = sz - 4 - (psecuritypriv->sw_encrypt ? 0 : pattrib->icv_len);					
		}
		else //no frag
		{
			sz = pattrib->last_txcmdsz;
		}

		pull = update_txdesc(pxmitframe, (uint*)mem_addr, sz);
		
		if(pull)
		{
			mem_addr += PACKET_OFFSET_SZ; //pull txdesc head
			
			//pxmitbuf ->pbuf = mem_addr;			
			pxmitframe->buf_addr = mem_addr;

		w_sz = sz + TXDESC_SIZE;
		}
		else
		{
			w_sz = sz + TXDESC_SIZE + PACKET_OFFSET_SZ;
		}	

		ff_hwaddr = get_ff_hwaddr(pxmitframe);
		
#ifdef PLATFORM_OS_CE
		write_port(padapter, ff_hwaddr, w_sz, (unsigned char*)mem_addr);
#else		
		write_port(padapter, ff_hwaddr, w_sz, (unsigned char*)pxmitbuf);
#endif

		count_tx_stats(padapter, pxmitframe, sz);


		RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("write_port, w_sz=%d\n", w_sz));
		//printk("write_port, w_sz=%d, sz=%d, txdesc_sz=%d, tid=%d\n", w_sz, sz, w_sz-sz, pattrib->priority);      

		mem_addr += w_sz;

		mem_addr = (u8 *)RND4(((uint)(mem_addr)));

	}
	
}

int xmitframe_direct(_adapter *padapter, struct xmit_frame *pxmitframe)
{
	int res = _SUCCESS;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;


	res = xmitframe_coalesce(padapter, pxmitframe->pkt, pxmitframe);

	pxmitframe->pkt = NULL;

	if (res == _SUCCESS) {
		dump_xframe(padapter, pxmitframe);
	}
	
	return res;
}

int xmitframe_enqueue(_adapter *padapter, struct xmit_frame *pxmitframe)
{
	if (xmit_classifier(padapter, pxmitframe) == _FAIL)
	{
		RT_TRACE(_module_rtl871x_xmit_c_,_drv_err_,("drop xmit pkt for classifier fail\n"));		
		pxmitframe->pkt = NULL;
		return _FAIL;
	}

	return _SUCCESS;
}

int pre_xmitframe(_adapter *padapter, struct xmit_frame *pxmitframe)
{
	_irqL irqL;
	int ret;
	struct xmit_buf *pxmitbuf = NULL;
	struct xmit_priv *pxmitpriv = &(padapter->xmitpriv);
	struct pkt_attrib *pattrib = &pxmitframe->attrib;


	do_queue_select(padapter, pattrib);
	

	_enter_critical(&pxmitpriv->lock, &irqL);
	
	if(txframes_sta_ac_pending(padapter, pattrib) > 0)//enqueue packet	
	{
		ret = _FALSE;		
		
		xmitframe_enqueue(padapter, pxmitframe);	

		_exit_critical(&pxmitpriv->lock, &irqL);
		
		return ret;
	}
	

	pxmitbuf = alloc_xmitbuf(pxmitpriv);	
	
	if(pxmitbuf == NULL)//enqueue packet
	{
		ret = _FALSE;		

		xmitframe_enqueue(padapter, pxmitframe);	

		_exit_critical(&pxmitpriv->lock, &irqL);
	}
	else //dump packet directly
	{
		_exit_critical(&pxmitpriv->lock, &irqL);

		ret = _TRUE;

		pxmitframe->pxmitbuf = pxmitbuf;

		pxmitframe->buf_addr = pxmitbuf->pbuf;

		pxmitbuf->priv_data = pxmitframe;	

		xmitframe_direct(padapter, pxmitframe); 
		
	}

	return ret;
}

