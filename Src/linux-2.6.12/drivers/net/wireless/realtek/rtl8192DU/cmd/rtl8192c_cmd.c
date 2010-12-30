/******************************************************************************
* rtl8192c_cmd.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2010, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _RTL8192C_CMD_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <cmd_osdep.h>
#include <mlme_osdep.h>
#include <rtl871x_byteorder.h>
#include <circ_buf.h>
#include <rtl871x_ioctl_set.h>


#ifdef PLATFORM_LINUX
#ifdef CONFIG_SDIO_HCI
#include <linux/mmc/sdio_func.h>
#endif
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
#include <linux/usb.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
#include <linux/usb_ch9.h>
#else
#include <linux/usb/ch9.h>
#endif
#include <linux/circ_buf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/atomic.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <linux/rtnetlink.h>
#endif


u32 read_macreg(_adapter *padapter, u32 addr, u32 sz)
{
	u32 val = 0;

	switch(sz)
	{
		case 1:
			val = read8(padapter, addr);
			break;
		case 2:
			val = read16(padapter, addr);
			break;
		case 4:
			val = read32(padapter, addr);
			break;
		default:
			val = 0xffffffff;
			break;
	}

	return val;
	
}

void write_macreg(_adapter *padapter, u32 addr, u32 val, u32 sz)
{
	switch(sz)
	{
		case 1:
			write8(padapter, addr, (u8)val);
			break;
		case 2:
			write16(padapter, addr, (u16)val);
			break;
		case 4:
			write32(padapter, addr, val);
			break;
		default:
			break;
	}

}

u32 read_bbreg(_adapter *padapter, u32 addr, u32 bitmask)
{
	return PHY_QueryBBReg(padapter, addr, bitmask);
}

void write_bbreg(_adapter *padapter, u32 addr, u32 bitmask, u32 val)
{
	PHY_SetBBReg(padapter, addr, bitmask, val);
}

u32 read_rfreg(_adapter *padapter, u8 rfpath, u32 addr, u32 bitmask)
{
	return PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)rfpath, addr, bitmask);
}

void write_rfreg(_adapter *padapter, u8 rfpath, u32 addr, u32 bitmask, u32 val)
{
	PHY_SetRFReg(padapter, (RF90_RADIO_PATH_E)rfpath, addr, bitmask, val);	
}

u8 r871x_NULL_hdl(_adapter *padapter, u8 *pbuf)
{
	return H2C_SUCCESS;
}

u8 r871x_setopmode_hdl(_adapter *padapter, u8 *pbuf)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct setopmode_parm *psetop = (struct setopmode_parm *)pbuf;

	write8(padapter, REG_BCN_MAX_ERR, 0xff);
	
	if(psetop->mode == Ndis802_11APMode)
	{
		pmlmeinfo->state = WIFI_FW_AP_STATE;
		Set_NETYPE0_MSR(padapter, _HW_STATE_AP_);	
	}
	else if(psetop->mode == Ndis802_11Infrastructure)
	{
		write8(padapter,REG_BCN_CTRL, 0x12);
		Set_NETYPE0_MSR(padapter, _HW_STATE_STATION_);	
	}
	else if(psetop->mode == Ndis802_11IBSS)
	{
		Set_NETYPE0_MSR(padapter, _HW_STATE_ADHOC_);	
	}
	else
	{
		write8(padapter,REG_BCN_CTRL, 0x12);
		Set_NETYPE0_MSR(padapter, _HW_STATE_NOLINK_);	
	}

	return H2C_SUCCESS;
	
}

u8 r871x_join_cmd_hdl(_adapter *padapter, u8 *pbuf)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX	*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
	struct joinbss_parm *pparm = (struct joinbss_parm *)pbuf;
	
	//check already connecting to AP or not
	if (pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS)
	{
		if (pmlmeinfo->state & WIFI_FW_STATION_STATE)
		{
			issue_deauth(padapter, pnetwork->MacAddress, WLAN_REASON_DEAUTH_LEAVING);
		}

		pmlmeinfo->state = WIFI_FW_NULL_STATE;
		
		//clear CAM
		flush_all_cam_entry(padapter);		
		
		_cancel_timer_ex(&pmlmeext->link_timer);
		
		//set MSR to nolink		
		Set_NETYPE0_MSR(padapter, _HW_STATE_NOLINK_);	

		//Set RCR to not to receive data frame when NO LINK state
		write32(padapter, REG_RCR, read32(padapter, REG_RCR) & ~RCR_ADF);
		
		//reset TSF
		write8(padapter, REG_DUAL_TSF_RST, (BIT(0)|BIT(1)));

		//disable update TSF
		if(IS_NORMAL_CHIP(pHalData->VersionID)||IS_HARDWARE_TYPE_8192DU(pHalData))
		{
			write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)|BIT(4));	
		}
		else
		{
			//write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)&(~(BIT(4)|BIT(5))));	
			write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)|BIT(4)|BIT(5));				
	}

	}


	joinbss_reset(padapter);

	pmlmeext->linked_to = 0;
	
	pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
	pmlmeext->cur_ch_offset= HAL_PRIME_CHNL_OFFSET_DONT_CARE;	
	pmlmeinfo->ERP_enable = 0;
	pmlmeinfo->WMM_enable = 0;
	pmlmeinfo->HT_enable = 0;
	pmlmeinfo->HT_caps_enable = 0;
	pmlmeinfo->HT_info_enable = 0;
	pmlmeinfo->agg_enable_bitmap = 0;
	pmlmeinfo->candidate_tid_bitmap = 0;
	
	//pmlmeinfo->assoc_AP_vendor = maxAP;
	
	if (padapter->registrypriv.wifi_spec) {
		// for WiFi test, follow WMM test plan spec
		write32(padapter, REG_EDCA_VO_PARAM, 0x002F431C);
		write32(padapter, REG_EDCA_VI_PARAM, 0x005E541C);
		write32(padapter, REG_EDCA_BE_PARAM, 0x0000A525);
		write32(padapter, REG_EDCA_BK_PARAM, 0x0000A549);
	
                // for WiFi test, mixed mode with intel STA under bg mode throughput issue
	        if (padapter->mlmepriv.htpriv.ht_option == 0)
		     write32(padapter, REG_EDCA_BE_PARAM, 0x00004320);

	} else {
	        write32(padapter, REG_EDCA_VO_PARAM, 0x002F3217);
	        write32(padapter, REG_EDCA_VI_PARAM, 0x005E4317);
	        write32(padapter, REG_EDCA_BE_PARAM, 0x00105320);
	        write32(padapter, REG_EDCA_BK_PARAM, 0x0000A444);
	}
	
	//disable dynamic functions, such as high power, DIG
	//Switch_DM_Func(padapter, DYNAMIC_FUNC_DISABLE, _FALSE);

	//config the initial gain under linking, need to write the BB registers
	write_bbreg(padapter, rOFDM0_XAAGCCore1, bMaskByte0, 0x32);
	write_bbreg(padapter, rOFDM0_XBAGCCore1, bMaskByte0, 0x32);

	//set MSR to nolink		
	Set_NETYPE0_MSR(padapter, _HW_STATE_NOLINK_);		
			
	if(IS_NORMAL_CHIP(pHalData->VersionID) ||IS_HARDWARE_TYPE_8192DU(pHalData))
	{
		//config RCR to receive different BSSID & not to receive data frame during linking				
		u32 v = read32(padapter, REG_RCR);
		v &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN | RCR_ADF);
		write32(padapter, REG_RCR, v);
	}	
	else
	{
		//config RCR to receive different BSSID & not to receive data frame during linking	
		write32(padapter, REG_RCR, read32(padapter, REG_RCR) & 0xfffff7bf);
	}

	
	//cancel link timer 
	_cancel_timer_ex(&pmlmeext->link_timer);


	_memcpy(pnetwork, pbuf, FIELD_OFFSET(WLAN_BSSID_EX, IELength)); 
	pnetwork->IELength = ((WLAN_BSSID_EX *)pbuf)->IELength;
	
	if(pnetwork->IELength>MAX_IE_SZ)//Check pbuf->IELength
		return H2C_PARAMETERS_ERROR;	
		
	_memcpy(pnetwork->IEs, ((WLAN_BSSID_EX *)pbuf)->IEs, pnetwork->IELength); 
	
	start_clnt_join(padapter);
	
	//only for cisco's AP
	if(pmlmeinfo->assoc_AP_vendor == ciscoAP)				
	{	
		int ie_len;
		struct registry_priv	 *pregpriv = &padapter->registrypriv;
		u8 *p = get_ie((pmlmeinfo->network.IEs + sizeof(NDIS_802_11_FIXED_IEs)), _HT_ADD_INFO_IE_, &ie_len, (pmlmeinfo->network.IELength - sizeof(NDIS_802_11_FIXED_IEs)));
		if( p && ie_len)
		{
			struct HT_info_element *pht_info = (struct HT_info_element *)(p+2);
					
			if ((pregpriv->cbw40_enable) &&	 (pht_info->infos[0] & BIT(2)))
			{
				//switch to the 40M Hz mode according to the AP
				pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_40;
				switch (pht_info->infos[0] & 0x3)
				{
					case 1:
						pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_LOWER;
						break;
			
					case 3:
						pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_UPPER;
						break;
				
					default:
						pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
						break;
				}		
						
				DBG_871X("set ch/bw for cisco's ap before connected\n");
						
				set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
						
			}
					
		}
		
	}
	
	pmlmeinfo->assoc_AP_vendor = maxAP;
	
	return H2C_SUCCESS;
	
}

u8 r871x_disconnect_hdl(_adapter *padapter, unsigned char *pbuf)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct setauth_parm		*pparm = (struct setauth_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
	
	if (is_client_associated_to_ap(padapter))
	{
		issue_deauth(padapter, pnetwork->MacAddress, WLAN_REASON_DEAUTH_LEAVING);
	}

	//set_opmode_cmd(padapter, infra_client_with_mlme);

	pmlmeinfo->state = WIFI_FW_NULL_STATE;
	
	//switch to the 20M Hz mode after disconnect
	pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
	pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
	
		
	//set MSR to no link state
	Set_NETYPE0_MSR(padapter, _HW_STATE_NOLINK_);		
		
	//Set RCR to not to receive data frame when NO LINK state
	write32(padapter, REG_RCR, read32(padapter, REG_RCR) & ~RCR_ADF);
	
	//reset TSF
	write8(padapter, REG_DUAL_TSF_RST, (BIT(0)|BIT(1)));

	//disable update TSF
	if(IS_NORMAL_CHIP(pHalData->VersionID)||IS_HARDWARE_TYPE_8192DU(pHalData))
	{
		write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)|BIT(4));	
	}
	else
	{
		write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)|BIT(4)|BIT(5));
	}
	
	if((pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE)
	{
		//Stop BCN		
		write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)&(~(EN_BCN_FUNCTION | EN_TXBCN_RPT)));
	}

	pmlmeinfo->state = WIFI_FW_NULL_STATE;
	
	set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);

	flush_all_cam_entry(padapter);
		
	_cancel_timer_ex(&pmlmeext->link_timer);
	pmlmeext->linked_to = 0;
	
	return 	H2C_SUCCESS;
}

u8 r871x_sitesurvey_cmd_hdl(_adapter *padapter, u8 *pbuf)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct sitesurvey_parm	*pparm = (struct sitesurvey_parm *)pbuf;
		
	if (pmlmeext->sitesurvey_res.state == _FALSE)
	{
		//for first time sitesurvey_cmd
		pmlmeext->sitesurvey_res.state = _TRUE;
		pmlmeext->sitesurvey_res.bss_cnt = 0;
		pmlmeext->sitesurvey_res.channel_idx = 0;
		
		if (le32_to_cpu(pparm->ss_ssidlen))
		{
			_memcpy(pmlmeext->sitesurvey_res.ss_ssid, pparm->ss_ssid, le32_to_cpu(pparm->ss_ssidlen));
		}	
		else
		{
			_memset(pmlmeext->sitesurvey_res.ss_ssid, 0, (IW_ESSID_MAX_SIZE + 1));
		}	
		
		pmlmeext->sitesurvey_res.ss_ssidlen = le32_to_cpu(pparm->ss_ssidlen);
	
		pmlmeext->sitesurvey_res.active_mode = le32_to_cpu(pparm->passive_mode);		

		//disable dynamic functions, such as high power, DIG
		Save_DM_Func_Flag(padapter);
		Switch_DM_Func(padapter, DYNAMIC_FUNC_DISABLE, _FALSE);

		//config the initial gain under scaning, need to write the BB registers
		write_bbreg(padapter, rOFDM0_XAAGCCore1, bMaskByte0, 0x17);
		write_bbreg(padapter, rOFDM0_XBAGCCore1, bMaskByte0, 0x17);
		
		
		//set MSR to no link state
		Set_NETYPE0_MSR(padapter, _HW_STATE_NOLINK_);		
		
		
		if(IS_NORMAL_CHIP(pHalData->VersionID)||IS_HARDWARE_TYPE_8192DU(pHalData))
		{
			//config RCR to receive different BSSID & not to receive data frame
			//pHalData->ReceiveConfig &= (~(RCR_CBSSID_DATA | RCR_CBSSID_BCN));			
			u32 v = read32(padapter, REG_RCR);
			v &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN | RCR_ADF);
			write32(padapter, REG_RCR, v);

			//disable update TSF
			write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)|BIT(4));
		}	
		else
		{
			//config RCR to receive different BSSID & not to receive data frame			
			write32(padapter, REG_RCR, read32(padapter, REG_RCR) & 0xfffff7bf);


			//disable update TSF
			write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)|BIT(4)|BIT(5));
		}

		//issue null data if associating to the AP
		if (is_client_associated_to_ap(padapter) == _TRUE)
		{
			issue_nulldata(padapter, 1);
		}
		
	}
		
	site_survey(padapter);

	return H2C_SUCCESS;
	
}

u8 r871x_setauth_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct setauth_parm		*pparm = (struct setauth_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	if (pparm->mode < 4)
	{
		pmlmeinfo->auth_algo = pparm->mode;
	}

	return 	H2C_SUCCESS;
}

u8 r871x_setkey_hdl(_adapter *padapter, u8 *pbuf)
{
	unsigned short				ctrl;
	struct setkey_parm		*pparm = (struct setkey_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	unsigned char					null_sta[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	pmlmeinfo->key_index = pparm->keyid;
	
	//write cam
	ctrl = BIT(15) | ((pparm->algorithm) << 2) | pparm->keyid;	
	
	write_cam(padapter, pparm->keyid, ctrl, null_sta, pparm->key);
	
	return H2C_SUCCESS;
}

u8 r871x_set_stakey_hdl(_adapter *padapter, u8 *pbuf)
{
	unsigned short ctrl=0;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct set_stakey_parm	*pparm = (struct set_stakey_parm *)pbuf;
	
	ctrl = BIT(15) | ((pparm->algorithm) << 2);	

	write_cam(padapter, 5, ctrl, pparm->addr, pparm->key);

	pmlmeinfo->enc_algo = pparm->algorithm;
	
	return H2C_SUCCESS;
}

u8 r871x_add_ba_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct addBaReq_parm 	*pparm = (struct addBaReq_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	if ((pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS) && (pmlmeinfo->HT_enable))
	{
		pmlmeinfo->ADDBA_retry_count = 0;
		pmlmeinfo->candidate_tid_bitmap |= (0x1 << pparm->tid);
		issue_action_BA(padapter, 3, 0, (u16)pparm->tid);
		_set_timer(&pmlmeext->ADDBA_timer, ADDBA_TO);
	}
	
	return 	H2C_SUCCESS;
}

u8 set_tx_beacon_cmd(_adapter* padapter)
{
	struct cmd_obj*		ph2c;
	struct Tx_Beacon_param 	*ptxBeacon_parm;	
	struct cmd_priv	*pcmdpriv = &(padapter->cmdpriv);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u8	res = _SUCCESS;
	
_func_enter_;	
	
	if ((ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj))) == NULL)
	{
		res= _FAIL;
		goto exit;
	}
	
	if ((ptxBeacon_parm = (struct Tx_Beacon_param *)_malloc(sizeof(struct Tx_Beacon_param))) == NULL)
	{
		_mfree((unsigned char *)ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	_memcpy(&(ptxBeacon_parm->network), &(pmlmeinfo->network), sizeof(WLAN_BSSID_EX));
	init_h2fwcmd_w_parm_no_rsp(ph2c, ptxBeacon_parm, GEN_CMD_CODE(_TX_Beacon));

	enqueue_cmd_ex(pcmdpriv, ph2c);

	
exit:
	
_func_exit_;

	return res;
}


thread_return cmd_thread(thread_context context)
{
	u8 ret;
	struct cmd_obj *pcmd;
	u8 *pcmdbuf, *prspbuf;
	u8 (*cmd_hdl)(_adapter *padapter, u8* pbuf);
	void (*pcmd_callback)(_adapter *dev, struct cmd_obj *pcmd);
       _adapter *padapter = (_adapter *)context;
	struct cmd_priv *pcmdpriv = &(padapter->cmdpriv);
	
_func_enter_;

	thread_enter(padapter);

	pcmdbuf = pcmdpriv->cmd_buf;
	prspbuf = pcmdpriv->rsp_buf;

	RT_TRACE(_module_rtl871x_cmd_c_,_drv_info_,("start r871x cmd_thread !!!!\n"));

	while(1)
	{
		if ((_down_sema(&(pcmdpriv->cmd_queue_sema))) == _FAIL)
			break;

		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{			
			RT_TRACE(_module_rtl871x_cmd_c_, _drv_info_, ("cmd_thread:bDriverStopped(%d) OR bSurpriseRemoved(%d)", padapter->bDriverStopped, padapter->bSurpriseRemoved));		
			break;
		}
		
		if (register_cmd_alive(padapter) != _SUCCESS)
		{
			continue;
		}
		
_next:
		if(!(pcmd = dequeue_cmd(&(pcmdpriv->cmd_queue)))) {
			unregister_cmd_alive(padapter);
			continue;
		}

		pcmdpriv->cmd_issued_cnt++;

		pcmd->cmdsz = _RND4((pcmd->cmdsz));//_RND4

		_memcpy(pcmdbuf, pcmd->parmbuf, pcmd->cmdsz);

		if(pcmd->cmdcode <= (sizeof(wlancmds) /sizeof(struct cmd_hdl)))
		{
			cmd_hdl = wlancmds[pcmd->cmdcode].h2cfuns;

			if (cmd_hdl)
			{			
				ret = cmd_hdl(padapter, pcmdbuf);
				pcmd->res = ret;
			}

			//invoke cmd->callback function		
			pcmd_callback = cmd_callback[pcmd->cmdcode].callback;
			if(pcmd_callback == NULL)
			{
				RT_TRACE(_module_rtl871x_cmd_c_,_drv_info_,("mlme_cmd_hdl(): pcmd_callback=0x%p, cmdcode=0x%x\n", pcmd_callback, pcmd->cmdcode));
				free_cmd_obj(pcmd);
			}	
			else
			{	
				//todo: !!! fill rsp_buf to pcmd->rsp if (pcmd->rsp!=NULL)
				
				pcmd_callback(padapter, pcmd);//need conider that free cmd_obj in cmd_callback
			}

			pcmdpriv->cmd_seq++;
			
		}
		
		cmd_hdl = NULL;

		flush_signals_thread();
		
		goto _next;
				
	}
	

	// free all cmd_obj resources
	do{

		pcmd = dequeue_cmd(&(pcmdpriv->cmd_queue));
		if(pcmd==NULL)
			break;

		free_cmd_obj(pcmd);
		
	}while(1);


	_up_sema(&pcmdpriv->terminate_cmdthread_sema);

_func_exit_;	

	thread_exit();

}

u8 r871x_mlme_evt_hdl(_adapter *padapter, unsigned char *pbuf)
{
	u8 evt_code, evt_seq;
	u16 evt_sz;
	uint 	*peventbuf;
	void (*event_callback)(_adapter *dev, u8 *pbuf);
	struct evt_priv *pevt_priv = &(padapter->evtpriv);

	peventbuf = (uint*)pbuf;
	evt_sz = (u16)(*peventbuf&0xffff);
	evt_seq = (u8)((*peventbuf>>24)&0x7f);
	evt_code = (u8)((*peventbuf>>16)&0xff);
	
		
	// checking event sequence...		
	if ((evt_seq & 0x7f) != pevt_priv->event_seq)
	{
		RT_TRACE(_module_rtl871x_cmd_c_,_drv_info_,("Evetn Seq Error! %d vs %d\n", (evt_seq & 0x7f), pevt_priv->event_seq));
	
		pevt_priv->event_seq = (evt_seq+1)&0x7f;

		goto _abort_event_;
	}

	// checking if event code is valid
	if (evt_code >= MAX_C2HEVT)
	{
		RT_TRACE(_module_rtl871x_cmd_c_,_drv_err_,("\nEvent Code(%d) mismatch!\n", evt_code));
		goto _abort_event_;
	}

	// checking if event size match the event parm size	
	if ((wlanevents[evt_code].parmsize != 0) && 
			(wlanevents[evt_code].parmsize != evt_sz))
	{
			
		RT_TRACE(_module_rtl871x_cmd_c_,_drv_err_,("\nEvent(%d) Parm Size mismatch (%d vs %d)!\n", 
			evt_code, wlanevents[evt_code].parmsize, evt_sz));
		goto _abort_event_;	
			
	}

	pevt_priv->event_seq = (pevt_priv->event_seq+1)&0x7f;//update evt_seq

	peventbuf += 2;
				
	if(peventbuf)
	{
		event_callback = wlanevents[evt_code].event_callback;
		event_callback(padapter, (u8*)peventbuf);

		pevt_priv->evt_done_cnt++;
	}


_abort_event_:


	return H2C_SUCCESS;
		
}

void dynamic_chk_wk_hdl(_adapter *padapter, u8 *pbuf, int sz)
{
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;

	if(pmlmeext->linked_to > 0)
	{
		pmlmeext->linked_to--;	
		if(pmlmeext->linked_to==0)
		    linked_status_chk(padapter);		
	}

	HalDmWatchDog(padapter);

	//check_hw_pbc(padapter, pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);	

}

u8 r871x_drvextra_cmd_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct drvextra_cmd_parm *pdrvextra_cmd;

	if(!pbuf)
		return H2C_PARAMETERS_ERROR;

	pdrvextra_cmd = (struct drvextra_cmd_parm*)pbuf;
	
	switch(pdrvextra_cmd->ec_id)
	{
		case DYNAMIC_CHK_WK_CID:
			dynamic_chk_wk_hdl(padapter, pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);
			break;
		case DM_CTRL_WK_CID:
			//HalDmWatchDog(padapter);
			break;
		case PBC_POLLING_WK_CID:
			//check_hw_pbc(padapter, pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);			
			break;
		default:
			break;

	}


	if(pdrvextra_cmd->pbuf && pdrvextra_cmd->sz>0)
	{
		_mfree(pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);
	}


	return H2C_SUCCESS;

}

static BOOLEAN
CheckWriteMSG(
	IN	PADAPTER		Adapter,
	IN	u8		BoxNum
)
{
	u8	valHMETFR;
	BOOLEAN	Result = _FALSE;
	
	valHMETFR = read8(Adapter, REG_HMETFR);

	//DbgPrint("CheckWriteH2C(): Reg[0x%2x] = %x\n",REG_HMETFR, valHMETFR);

	if(((valHMETFR>>BoxNum)&BIT0) == 1)
		Result = _TRUE;
	
	return Result;

}

BOOLEAN CheckFwReadLastMSG(
	IN	PADAPTER		Adapter,
	IN	u8		BoxNum
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	valHMETFR, valMCUTST_1;
	BOOLEAN	 Result = _FALSE;
	
	valHMETFR = read8(Adapter, REG_HMETFR);
	valMCUTST_1 = read8(Adapter, (REG_MCUTST_1+BoxNum));

	//DbgPrint("REG[%x] = %x, REG[%x] = %x\n", 
	//	REG_HMETFR, valHMETFR, REG_MCUTST_1+BoxNum, valMCUTST_1 );

	// Do not seperate to 91C and 88C, we use the same setting. Suggested by SD4 Filen. 2009.12.03.
	if(IS_NORMAL_CHIP(pHalData->VersionID)||IS_HARDWARE_TYPE_8192D(pHalData))
	{
		if(((valHMETFR>>BoxNum)&BIT0) == 0)
			Result = _TRUE;
	}
	else
	{
		if((((valHMETFR>>BoxNum)&BIT0) == 0) && (valMCUTST_1 == 0))
		{
			Result = _TRUE;
		}
	}
	
	return Result;
}

u8 r871x_h2c_msg_hdl(_adapter *padapter, unsigned char *pbuf)
{	
	u8 ElementID, CmdLen;
	u8 *pCmdBuffer;
	u8	BoxNum;
	u16	BOXReg, BOXExtReg;
	u8	U1btmp; //Read 0x1bf
	u8 BoxContent[4], BoxExtContent[2];
	struct cmd_msg_parm  *pcmdmsg;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8 	BufIndex=0;
	u8	bWriteSucess = _FALSE;
	u8	IsFwRead = _FALSE;
	u8	WaitH2cLimmit = 100;
	u8	WaitWriteH2cLimmit = 100;
	
	if(!pbuf)
		return H2C_PARAMETERS_ERROR;

	pcmdmsg = (struct cmd_msg_parm*)pbuf;
	ElementID = pcmdmsg->eid;
	CmdLen = pcmdmsg->sz;
	pCmdBuffer = pcmdmsg->buf;

	while(!bWriteSucess)
	{
		WaitWriteH2cLimmit--;
		if(WaitWriteH2cLimmit == 0)
		{	
			DBG_8192C("FillH2CCmd92C():Write H2C fail because no trigger for FW INT!!!!!!!!\n");
			break;
		}
	
		// 2. Find the last BOX number which has been writen.
		BoxNum = pHalData->LastHMEBoxNum;
		switch(BoxNum)
		{
			case 0:
				BOXReg = REG_HMEBOX_0;
				BOXExtReg = REG_HMEBOX_EXT_0;
				break;
			case 1:
				BOXReg = REG_HMEBOX_1;
				BOXExtReg = REG_HMEBOX_EXT_1;
				break;
			case 2:
				BOXReg = REG_HMEBOX_2;
				BOXExtReg = REG_HMEBOX_EXT_2;
				break;
			case 3:
				BOXReg = REG_HMEBOX_3;
				BOXExtReg = REG_HMEBOX_EXT_3;
				break;
			default:
				break;
		}

		// 3. Check if the box content is empty.
		IsFwRead = CheckFwReadLastMSG(padapter, BoxNum);
		while(!IsFwRead)
		{
			//wait until Fw read
			WaitH2cLimmit--;
			if(WaitH2cLimmit == 0)
			{
				DBG_8192C("FillH2CCmd92C(): Wating too long for FW read clear HMEBox(%d)!!!\n", BoxNum);
				break;
			}
			udelay_os(10); //us
			IsFwRead = CheckFwReadLastMSG(padapter, BoxNum);
			U1btmp = read8(padapter, 0x1BF);
			//RT_TRACE(COMP_CMD, DBG_LOUD, ("FillH2CCmd92C(): Wating for FW read clear HMEBox(%d)!!! 0x1BF = %2x\n", BoxNum, U1btmp));
		}

		// If Fw has not read the last H2C cmd, break and give up this H2C.
		if(!IsFwRead)
		{
			DBG_8192C("FillH2CCmd92C():  Write H2C register BOX[%d] fail!!!!! Fw do not read. \n", BoxNum);
			break;
		}

		// 4. Fill the H2C cmd into box		
		_memset(BoxContent, 0, sizeof(BoxContent));
		_memset(BoxExtContent, 0, sizeof(BoxExtContent));
		
		BoxContent[0] = ElementID; // Fill element ID

		//DBG_8192C("FillH2CCmd92C():Write ElementID BOXReg(%4x) = %2x \n", BOXReg, ElementID);

		switch(CmdLen)
		{
			case 1:
				{
					BoxContent[0] &= ~(BIT7);
					_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 1);
					write32(padapter, BOXReg, *((u32*)BoxContent));
					break;
				}
			case 2:
				{	
					BoxContent[0] &= ~(BIT7);
					_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 2);
					write32(padapter, BOXReg, *((u32*)BoxContent));
					break;
				}
			case 3:
				{
					BoxContent[0] &= ~(BIT7);
					_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 3);
					write32(padapter, BOXReg, *((u32*)BoxContent));
					break;
				}
			case 4:
				{
					BoxContent[0] |= (BIT7);
					_memcpy((u8*)(BoxExtContent), pCmdBuffer+BufIndex, 2);
					_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex+2, 2);
					write16(padapter, BOXExtReg, *((u16*)BoxExtContent));
					write32(padapter, BOXReg, *((u32*)BoxContent));
					break;
				}
			case 5:
				{
					BoxContent[0] |= (BIT7);
					_memcpy((u8*)(BoxExtContent), pCmdBuffer+BufIndex, 2);
					_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex+2, 3);
					write16(padapter, BOXExtReg, *((u16*)BoxExtContent));
					write32(padapter, BOXReg, *((u32*)BoxContent));
					break;
				}
			default:
					break;
					
		}
			
		//DBG_8192C("FillH2CCmd(): BoxExtContent=0x%x\n", *(u16*)BoxExtContent);		
		//DBG_8192C("FillH2CCmd(): BoxContent=0x%x\n", *(u32*)BoxContent);
			
		if(IS_NORMAL_CHIP(pHalData->VersionID)||IS_HARDWARE_TYPE_8192D(pHalData))
		{
			// 5. Normal chip does not need to check if the H2C cmd has be written successfully.
			// 92D test chip does not need to check,
			bWriteSucess = _TRUE;
		}
		else
		{	
			// 5. Check if the H2C cmd has be written successfully.
			bWriteSucess = CheckWriteMSG(padapter, BoxNum);
			if(!bWriteSucess) //If not then write again.
				continue;
			
			//6. Fill H2C protection register.

			write8(padapter, REG_MCUTST_1+BoxNum, 0xFF);
			//RT_TRACE(COMP_CMD, DBG_LOUD, ("FillH2CCmd92C():Write Reg(%4x) = 0xFF \n", REG_MCUTST_1+BoxNum));
		}

		// Record the next BoxNum
		pHalData->LastHMEBoxNum = BoxNum+1;
		if(pHalData->LastHMEBoxNum == 4) // loop to 0
			pHalData->LastHMEBoxNum = 0;
		
		DBG_8192C("FillH2CCmd92C():pHalData->LastHMEBoxNum  = %d\n", pHalData->LastHMEBoxNum);
		
	}
		

	return H2C_SUCCESS;

}

u8 set_rssi_cmd(_adapter*padapter, u8 *param)
{
	struct cmd_obj*		ph2c;
	struct cmd_msg_parm  *pcmdmsg_parm;	
	struct cmd_priv	*pcmdpriv=&padapter->cmdpriv;
	u8	res=_SUCCESS;
	
_func_enter_;	

	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));	
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
	}
	
	pcmdmsg_parm = (struct cmd_msg_parm*)_malloc(sizeof(struct cmd_msg_parm)); 
	if(pcmdmsg_parm==NULL){
		_mfree((unsigned char *)ph2c, sizeof(struct cmd_obj));
		res= _FAIL;
		goto exit;
	}

	pcmdmsg_parm->eid = RSSI_SETTING_EID;
	pcmdmsg_parm->sz = 3;
	_memcpy(pcmdmsg_parm->buf, param, pcmdmsg_parm->sz);
		

	init_h2fwcmd_w_parm_no_rsp(ph2c, pcmdmsg_parm, GEN_CMD_CODE(_Set_H2C_MSG));

		
	enqueue_cmd_ex(pcmdpriv, ph2c);
	
exit:
	
_func_exit_;

	return res;


}

u8 set_raid_cmd(_adapter*padapter, u32 mask, u8 arg)
{
	struct cmd_obj*		ph2c;
	struct cmd_msg_parm  *pcmdmsg_parm;	
	struct cmd_priv	*pcmdpriv=&padapter->cmdpriv;
	u8	res=_SUCCESS;
	
_func_enter_;	

	ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj));	
	if(ph2c==NULL){
		res= _FAIL;
		goto exit;
	}
	
	pcmdmsg_parm = (struct cmd_msg_parm*)_malloc(sizeof(struct cmd_msg_parm)); 
	if(pcmdmsg_parm==NULL){
		_mfree((unsigned char *)ph2c, sizeof(struct cmd_obj));
		res= _FAIL;
		goto exit;
	}

	pcmdmsg_parm->eid = MACID_CONFIG_EID;
	pcmdmsg_parm->sz = 5;
	_memcpy(pcmdmsg_parm->buf, &mask, 4);
	pcmdmsg_parm->buf[4]  = arg;
	

	init_h2fwcmd_w_parm_no_rsp(ph2c, pcmdmsg_parm, GEN_CMD_CODE(_Set_H2C_MSG));

		
	enqueue_cmd_ex(pcmdpriv, ph2c);
	
exit:
	
_func_exit_;

	return res;

}

void dummy_event_callback(_adapter *adapter , u8 *pbuf)
{

}
void fwdbg_event_callback(_adapter *adapter , u8 *pbuf)
{

}

