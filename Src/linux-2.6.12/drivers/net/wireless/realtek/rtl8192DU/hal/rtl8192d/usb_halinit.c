/******************************************************************************
* usb_halinit.c                                                                                                                                 *
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
#define _HCI_HAL_INIT_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <hal_init.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifndef CONFIG_USB_HCI

#error "CONFIG_USB_HCI shall be on!\n"

#endif

#include <usb_ops.h>
#include <usb_hal.h>
#include <usb_osintf.h>

//endpoint number 1,2,3,4,5
// bult in : 1
// bult out: 2 (High)
// bult out: 3 (Normal) for 3 out_ep, (Low) for 2 out_ep
// interrupt in: 4
// bult out: 5 (Low) for 3 out_ep


VOID
_OneOutEpMapping(
	IN	HAL_DATA_TYPE	*pHalData
	)
{
	//only endpoint number 0x02

	pHalData->Queue2EPNum[0] =  pHalData->RtBulkOutPipes[0];//VO
	pHalData->Queue2EPNum[1] =  pHalData->RtBulkOutPipes[0];//VI
	pHalData->Queue2EPNum[2] =  pHalData->RtBulkOutPipes[0];//BE
	pHalData->Queue2EPNum[3] =  pHalData->RtBulkOutPipes[0];//BK
	
	pHalData->Queue2EPNum[4] =  pHalData->RtBulkOutPipes[0];//TS
	pHalData->Queue2EPNum[5] =  pHalData->RtBulkOutPipes[0];//MGT
	pHalData->Queue2EPNum[6] =  pHalData->RtBulkOutPipes[0];//BMC
	pHalData->Queue2EPNum[7] =  pHalData->RtBulkOutPipes[0];//BCN
}


VOID
_TwoOutEpMapping(
	IN	BOOLEAN			IsTestChip,
	IN	HAL_DATA_TYPE	*pHalData,
	IN	BOOLEAN	 		bWIFICfg
	)
{

/*
#define VO_QUEUE_INX	0
#define VI_QUEUE_INX		1
#define BE_QUEUE_INX		2
#define BK_QUEUE_INX		3
#define TS_QUEUE_INX		4
#define MGT_QUEUE_INX	5
#define BMC_QUEUE_INX	6
#define BCN_QUEUE_INX	7
*/

	if(IsTestChip && bWIFICfg){ // test chip && wmm
	
		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  1, 	0, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};			
		//0:H(end_number=0x02), 1:L (end_number=0x03)

		pHalData->Queue2EPNum[0] =  pHalData->RtBulkOutPipes[0];//VO
		pHalData->Queue2EPNum[1] =  pHalData->RtBulkOutPipes[1];//VI
		pHalData->Queue2EPNum[2] =  pHalData->RtBulkOutPipes[0];//BE
		pHalData->Queue2EPNum[3] =  pHalData->RtBulkOutPipes[1];//BK
		
		pHalData->Queue2EPNum[4] =  pHalData->RtBulkOutPipes[0];//TS
		pHalData->Queue2EPNum[5] =  pHalData->RtBulkOutPipes[0];//MGT
		pHalData->Queue2EPNum[6] =  pHalData->RtBulkOutPipes[0];//BMC
		pHalData->Queue2EPNum[7] =  pHalData->RtBulkOutPipes[0];//BCN
	
	}
	else if(!IsTestChip && bWIFICfg){ // Normal chip && wmm
		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  0, 	1, 	0, 	1, 	0, 	0, 	0, 	0, 		0	};
		//0:H(end_number=0x02), 1:L (end_number=0x03)
		
		pHalData->Queue2EPNum[0] =  pHalData->RtBulkOutPipes[1];//VO
		pHalData->Queue2EPNum[1] =  pHalData->RtBulkOutPipes[0];//VI
		pHalData->Queue2EPNum[2] =  pHalData->RtBulkOutPipes[1];//BE
		pHalData->Queue2EPNum[3] =  pHalData->RtBulkOutPipes[0];//BK
		
		pHalData->Queue2EPNum[4] =  pHalData->RtBulkOutPipes[0];//TS
		pHalData->Queue2EPNum[5] =  pHalData->RtBulkOutPipes[0];//MGT
		pHalData->Queue2EPNum[6] =  pHalData->RtBulkOutPipes[0];//BMC
		pHalData->Queue2EPNum[7] =  pHalData->RtBulkOutPipes[0];//BCN
		
	}
	else{//typical setting

		
		//BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  1, 	1, 	0, 	0, 	0, 	0, 	0, 	0, 		0	};			
		//0:H(end_number=0x02), 1:L (end_number=0x03)
		
		pHalData->Queue2EPNum[0] =  pHalData->RtBulkOutPipes[0];//VO
		pHalData->Queue2EPNum[1] =  pHalData->RtBulkOutPipes[0];//VI
		pHalData->Queue2EPNum[2] =  pHalData->RtBulkOutPipes[1];//BE
		pHalData->Queue2EPNum[3] =  pHalData->RtBulkOutPipes[1];//BK
		
		pHalData->Queue2EPNum[4] =  pHalData->RtBulkOutPipes[0];//TS
		pHalData->Queue2EPNum[5] =  pHalData->RtBulkOutPipes[0];//MGT
		pHalData->Queue2EPNum[6] =  pHalData->RtBulkOutPipes[0];//BMC
		pHalData->Queue2EPNum[7] =  pHalData->RtBulkOutPipes[0];//BCN	
		
	}
	
}


VOID _ThreeOutEpMapping(
	IN	HAL_DATA_TYPE	*pHalData,
	IN	BOOLEAN	 		bWIFICfg
	)
{
	if(bWIFICfg){//for WMM
		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  1, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:H(end_number=0x02), 1:N(end_number=0x03), 2:L (end_number=0x05)
		
		pHalData->Queue2EPNum[0] =  pHalData->RtBulkOutPipes[0];//VO
		pHalData->Queue2EPNum[1] =  pHalData->RtBulkOutPipes[1];//VI
		pHalData->Queue2EPNum[2] =  pHalData->RtBulkOutPipes[2];//BE
		pHalData->Queue2EPNum[3] =  pHalData->RtBulkOutPipes[1];//BK
		
		pHalData->Queue2EPNum[4] =  pHalData->RtBulkOutPipes[0];//TS
		pHalData->Queue2EPNum[5] =  pHalData->RtBulkOutPipes[0];//MGT
		pHalData->Queue2EPNum[6] =  pHalData->RtBulkOutPipes[0];//BMC
		pHalData->Queue2EPNum[7] =  pHalData->RtBulkOutPipes[0];//BCN
		
	}
	else{//typical setting

		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  2, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};			
		//0:H(end_number=0x02), 1:N(end_number=0x03), 2:L (end_number=0x05)
		
		pHalData->Queue2EPNum[0] =  pHalData->RtBulkOutPipes[0];//VO
		pHalData->Queue2EPNum[1] =  pHalData->RtBulkOutPipes[1];//VI
		pHalData->Queue2EPNum[2] =  pHalData->RtBulkOutPipes[2];//BE
		pHalData->Queue2EPNum[3] =  pHalData->RtBulkOutPipes[2];//BK
		
		pHalData->Queue2EPNum[4] =  pHalData->RtBulkOutPipes[0];//TS
		pHalData->Queue2EPNum[5] =  pHalData->RtBulkOutPipes[0];//MGT
		pHalData->Queue2EPNum[6] =  pHalData->RtBulkOutPipes[0];//BMC
		pHalData->Queue2EPNum[7] =  pHalData->RtBulkOutPipes[0];//BCN	
	}

}

BOOLEAN
_MappingOutEP(
	IN	PADAPTER	pAdapter,
	IN	u8		NumOutPipe,
	IN	BOOLEAN		IsTestChip
	)
{		
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct registry_priv *pregistrypriv = &pAdapter->registrypriv;

	BOOLEAN	 bWIFICfg = (pregistrypriv->wifi_spec) ?_TRUE:_FALSE;
	
	BOOLEAN result = _TRUE;

	switch(NumOutPipe)
	{
		case 2:
			_TwoOutEpMapping(IsTestChip, pHalData, bWIFICfg);
			break;
		case 3:
			if(!IS_HARDWARE_TYPE_8192DU(pHalData))
			{
				// Test chip doesn't support three out EPs.
				if(IsTestChip){
					return _FALSE;
				}
			}
			_ThreeOutEpMapping(pHalData, bWIFICfg);
			break;
		case 1:
			_OneOutEpMapping(pHalData);
			break;
		default:
			result = _FALSE;
			break;
	}

	return result;
	
}

VOID
_ConfigTestChipOutEP(
	IN	PADAPTER	pAdapter,
	IN	u8		NumOutPipe
	)
{
	u8			value8,txqsele;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);

	pHalData->OutEpQueueSel = 0;
	pHalData->OutEpNumber	= 0;

	value8 = read8(pAdapter, REG_TEST_SIE_OPTIONAL);
	value8 = (value8 & USB_TEST_EP_MASK) >> USB_TEST_EP_SHIFT;
	
	switch(value8)
	{
		case 0:		// 2 bulk OUT, 1 bulk IN
		case 3:		
			pHalData->OutEpQueueSel = TX_SELE_HQ | TX_SELE_LQ;
			pHalData->OutEpNumber	= 2;
			//RT_TRACE(COMP_INIT,  DBG_LOUD, ("EP Config: 2 bulk OUT, 1 bulk IN\n"));
			break;
		case 1:		// 1 bulk IN/OUT => map all endpoint to Low queue
		case 2:		// 1 bulk IN, 1 bulk OUT => map all endpoint to High queue
			txqsele = read8(pAdapter, REG_TEST_USB_TXQS);
			if(txqsele & 0x0F){//map all endpoint to High queue
				pHalData->OutEpQueueSel  = TX_SELE_HQ;
			}
			else if(txqsele&0xF0){//map all endpoint to Low queue
				pHalData->OutEpQueueSel  =  TX_SELE_LQ;
			}
			pHalData->OutEpNumber	= 1;
			//RT_TRACE(COMP_INIT,  DBG_LOUD, ("%s\n", ((1 == value8) ? "1 bulk IN/OUT" : "1 bulk IN, 1 bulk OUT")));
			break;
		default:
			break;
	}

	// TODO: Error recovery for this case
	//RT_ASSERT((NumOutPipe == pHalData->OutEpNumber), ("Out EP number isn't match! %d(Descriptor) != %d (SIE reg)\n", (u4Byte)NumOutPipe, (u4Byte)pHalData->OutEpNumber));

}



VOID
_ConfigNormalChipOutEP(
	IN	PADAPTER	pAdapter,
	IN	u8		NumOutPipe
	)
{
	u8			value8;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);

	pHalData->OutEpQueueSel = 0;
	pHalData->OutEpNumber	= 0;
		
	// Normal and High queue
	if(IS_HARDWARE_TYPE_8192DU(pHalData)){
		if(pHalData->interfaceIndex==0)
			value8=read8(pAdapter, REG_USB_High_NORMAL_Queue_Select_MAC0);
		else
			value8=read8(pAdapter, REG_USB_High_NORMAL_Queue_Select_MAC1);
	}
	else
		value8 = read8(pAdapter, (REG_NORMAL_SIE_EP + 1));
	
	if(value8 & USB_NORMAL_SIE_EP_MASK){
		pHalData->OutEpQueueSel |= TX_SELE_HQ;
		pHalData->OutEpNumber++;
	}
	
	if((value8 >> USB_NORMAL_SIE_EP_SHIFT) & USB_NORMAL_SIE_EP_MASK){
		pHalData->OutEpQueueSel |= TX_SELE_NQ;
		pHalData->OutEpNumber++;
	}
	
	// Low queue
	if(IS_HARDWARE_TYPE_8192DU(pHalData)){
		if(pHalData->interfaceIndex==0)
			value8=read8(pAdapter, (REG_USB_High_NORMAL_Queue_Select_MAC0+1));
		else
			value8=read8(pAdapter, (REG_USB_High_NORMAL_Queue_Select_MAC1+1));
	}
	else
		value8 = read8(pAdapter, (REG_NORMAL_SIE_EP + 2));
	if(value8 & USB_NORMAL_SIE_EP_MASK){
		pHalData->OutEpQueueSel |= TX_SELE_LQ;
		pHalData->OutEpNumber++;
	}

//add for 0xfe44 0xfe45 0xfe47 0xfe48 not validly 
	if(IS_HARDWARE_TYPE_8192DU(pHalData)){
		switch(NumOutPipe){
			case 	3:
					pHalData->OutEpQueueSel=TX_SELE_HQ| TX_SELE_LQ|TX_SELE_NQ;
					pHalData->OutEpNumber=3;
					break;
			case 	2:
					pHalData->OutEpQueueSel=TX_SELE_HQ| TX_SELE_NQ;
					pHalData->OutEpNumber=2;
					break;
			case 	1:
					pHalData->OutEpQueueSel=TX_SELE_HQ;
					pHalData->OutEpNumber=1;
					break;
			default:				
					break;
			
			}
		
	}

	// TODO: Error recovery for this case
	//RT_ASSERT((NumOutPipe == pHalData->OutEpNumber), ("Out EP number isn't match! %d(Descriptor) != %d (SIE reg)\n", (u4Byte)NumOutPipe, (u4Byte)pHalData->OutEpNumber));

}

BOOLEAN HalUsbSetQueuePipeMapping8192CUsb(
	IN	PADAPTER	pAdapter,
	IN	u8		NumInPipe,
	IN	u8		NumOutPipe
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
	BOOLEAN			result		= _FALSE;
	BOOLEAN			isNormalChip;

	// ReadAdapterInfo8192C also call _ReadChipVersion too.
	// Since we need dynamic config EP mapping, so we call this function to get chip version.
	// We can remove _ReadChipVersion from ReadAdapterInfo8192C later.
	_ReadChipVersion(pAdapter);

	isNormalChip = IS_NORMAL_CHIP(pHalData->VersionID);

	if(isNormalChip||IS_HARDWARE_TYPE_8192DU(pHalData)){
		_ConfigNormalChipOutEP(pAdapter, NumOutPipe);
	}
	else{
		_ConfigTestChipOutEP(pAdapter, NumOutPipe);
	}

	// Normal chip with one IN and one OUT doesn't have interrupt IN EP.
	if(isNormalChip && (1 == pHalData->OutEpNumber)){
		if(1 != NumInPipe){
			return result;
		}
	}

	// All config other than above support one Bulk IN and one Interrupt IN.
	//if(2 != NumInPipe){
	//	return result;
	//}

	result = _MappingOutEP(pAdapter, NumOutPipe, !isNormalChip);
	
	return result;

}


u8 _InitPowerOn(_adapter *padapter)
{
	HAL_DATA_TYPE    *pHalData = GET_HAL_DATA(padapter);
	u8	ret = _SUCCESS;
	u32	value32;
	u16	value16=0;
	u8	value8 = 0;

	// polling autoload done.
	u32	pollingCount = 0;

	if(padapter->bSurpriseRemoved){
		return _FAIL;
	}

	if(IS_HARDWARE_TYPE_8192D(pHalData)&&(pHalData->MacPhyMode92D!=SINGLEMAC_SINGLEPHY))
	{
		// in DMDP mode,polling power off in process bit 0x17[7]
		value8=read8(padapter, 0x17);
		while((value8&BIT7) && (pollingCount < 200))
		{
			udelay_os(200);
			value8=read8(padapter, 0x17);		
			pollingCount++;
		}
	}
	pollingCount = 0;

	do
	{
		if(read8(padapter, REG_APS_FSMCO) & PFM_ALDN){
			//RT_TRACE(COMP_INIT,DBG_LOUD,("Autoload Done!\n"));
			break;
		}

		if(pollingCount++ > POLLING_READY_TIMEOUT_COUNT){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling REG_APS_FSMCO[PFM_ALDN] done!\n"));
			return _FAIL;
		}
				
	}while(_TRUE);


//	For hardware power on sequence.
#if 1
	//0.	RSV_CTRL 0x1C[7:0] = 0x00			// unlock ISO/CLK/Power control register
	write8(padapter, REG_RSV_CTRL, 0x0);	
	// Power on when re-enter from IPS/Radio off/card disable
	write8(padapter, REG_SPS0_CTRL, 0x2b);//enable SPS into PWM mode	
	udelay_os(100);//PlatformSleepUs(100);//this is not necessary when initially power on

	value8 = read8(padapter, REG_LDOV12D_CTRL);	
	if(0== (value8 & LDV12_EN) ){
		value8 |= LDV12_EN;
		write8(padapter, REG_LDOV12D_CTRL, value8);
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" power-on :REG_LDOV12D_CTRL Reg0x21:0x%02x.\n",value8));
		udelay_os(100);//PlatformSleepUs(100);//this is not necessary when initially power on
		value8 = read8(padapter, REG_SYS_ISO_CTRL);
		value8 &= ~ISO_MD2PP;
		write8(padapter, REG_SYS_ISO_CTRL, value8);			
	}	
	
	// auto enable WLAN
	pollingCount = 0;
	value16 = read16(padapter, REG_APS_FSMCO);
	value16 |= APFM_ONMAC;
	write16(padapter, REG_APS_FSMCO, value16);

	do
	{
		if(0 == (read16(padapter, REG_APS_FSMCO) & APFM_ONMAC)){
			//RT_TRACE(COMP_INIT,DBG_LOUD,("MAC auto ON okay!\n"));
			break;
		}

		if(pollingCount++ > POLLING_READY_TIMEOUT_COUNT){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling REG_APS_FSMCO[APFM_ONMAC] done!\n"));
			return _FAIL;
		}
				
	}while(_TRUE);


	// release RF digital isolation
	value16 = read16(padapter, REG_SYS_ISO_CTRL);
	value16 &= ~ISO_DIOR;
	write16(padapter, REG_SYS_ISO_CTRL, value16);

	// Enable MAC DMA/WMAC/SCHEDULE/SEC block
	value16 = read16(padapter, REG_CR);
	value16 |= (HCI_TXDMA_EN | HCI_RXDMA_EN | TXDMA_EN | RXDMA_EN
				| PROTOCOL_EN | SCHEDULE_EN | MACTXEN | MACRXEN | ENSEC);
	write16(padapter, REG_CR, value16);
	
#else
	// Enable PLL Power
	value8 = PlatformIORead1Byte(Adapter, REG_LDOA15_CTRL);
	value8 |= LDA15_EN;
	PlatformIOWrite1Byte(Adapter, REG_LDOA15_CTRL, value8);

	// Release XTAL Gated for AFE PLL
	value32 = PlatformIORead4Byte(Adapter, REG_AFE_XTAL_CTRL);
	value32 &= ~XTAL_GATE_AFE;
	PlatformIOWrite4Byte(Adapter, REG_AFE_XTAL_CTRL, value32);

	// Enable AFE PLL
	value8 = PlatformIORead1Byte(Adapter, REG_AFE_PLL_CTRL);
	value8 |= APLL_EN;
	PlatformIOWrite1Byte(Adapter, REG_AFE_PLL_CTRL, value8);

	// wait for AFE clock stable
	pollingCount = 0;
	do
	{
		if(PlatformIORead1Byte(Adapter, REG_SYS_CFG) & ACLK_VLD){
			RT_TRACE(COMP_INIT,DBG_LOUD,("AFE clock is stable!\n"));
			break;
		}

		if(pollingCount++ > POLLING_READY_TIMEOUT_COUNT){
			RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling AFE clock done!\n"));
			return RT_STATUS_FAILURE;
		}

	}while(TRUE);
	
	// Release Isolation for MAC_TOP RF, BB_PHY, PCIE_TOP, AFE_PLL 
	value8 = PlatformIORead1Byte(Adapter, REG_SYS_ISO_CTRL);
	value8 &= ~ISO_MD2PP;
	PlatformIOWrite1Byte(Adapter, REG_SYS_ISO_CTRL, value8);

	// Enable WMAC and Security Clock
	value16 = PlatformIORead2Byte(Adapter, REG_SYS_CLKR);
	value16 |= (MAC_CLK_EN | SEC_CLK_EN);
	PlatformIOWrite2Byte(Adapter, REG_SYS_CLKR, value16);

	// Release WMAC reset & register reset
	value16 = PlatformIORead2Byte(Adapter, REG_SYS_FUNC_EN);
	value16 |= (FEN_DCORE | FEN_MREGEN);
	PlatformIOWrite2Byte(Adapter, REG_SYS_FUNC_EN, value16);

	RT_TRACE(COMP_INIT,DBG_LOUD,("8192C power init done!\n"));

	// Release MAC IO register reset
	value16 = PlatformIORead2Byte(Adapter, REG_CR);
	value16 |= (HCI_TXDMA_EN | HCI_RXDMA_EN | TXDMA_EN | RXDMA_EN
				| PROTOCOL_EN | SCHEDULE_EN | MACTXEN | MACRXEN | ENSEC);
	PlatformIOWrite2Byte(Adapter, REG_CR, value16);
#endif

	return ret;

}


//-------------------------------------------------------------------------
//
// LLT R/W/Init function
//
//-------------------------------------------------------------------------
u8 _LLTWrite(
	IN  PADAPTER	Adapter,
	IN	u32		address,
	IN	u32		data
	)
{
	u8	status = _SUCCESS;
	int	count = 0;
	u32	value = _LLT_INIT_ADDR(address) | _LLT_INIT_DATA(data) | _LLT_OP(_LLT_WRITE_ACCESS);

	write32(Adapter, REG_LLT_INIT, value);
	
	//polling
	do{
		
		value = read32(Adapter, REG_LLT_INIT);
		if(_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)){
			break;
		}
		
		if(count > POLLING_LLT_THRESHOLD){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling write LLT done at address %d!\n", address));
			status = _FAIL;
			break;
		}
	}while(count++);

	return status;
	
}

u8 _LLTRead(
	IN  PADAPTER	Adapter,
	IN	u32		address
	)
{
	int		count = 0;
	u32		value = _LLT_INIT_ADDR(address) | _LLT_OP(_LLT_READ_ACCESS);

	write32(Adapter, REG_LLT_INIT, value);

	//polling and get value
	do{
		
		value = read32(Adapter, REG_LLT_INIT);
		if(_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)){
			return (u8)value;
		}
		
		if(count > POLLING_LLT_THRESHOLD){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling read LLT done at address %d!\n", address));
			break;
		}
	}while(count++);

	return 0xFF;

}

u8 InitLLTTable(
	IN  PADAPTER	Adapter,
	IN	u32		boundary
	)
{
	u8		status = _SUCCESS;
	u32		i;
	u32		txpktbuf_bndy = boundary;
	u32		Last_Entry_Of_TxPktBuf = LAST_ENTRY_OF_TX_PKT_BUFFER;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);

	if(IS_HARDWARE_TYPE_8192DU(pHalData))	
	{
		if(pHalData->MacPhyMode92D !=SINGLEMAC_SINGLEPHY){
			//for 92du two mac: The page size is different from 92c and 92s
			txpktbuf_bndy =125;
			Last_Entry_Of_TxPktBuf=127;
		}
		else{
			txpktbuf_bndy =253;
			Last_Entry_Of_TxPktBuf=255;
		} 
	}

	for(i = 0 ; i < (txpktbuf_bndy - 1) ; i++){
		status = _LLTWrite(Adapter, i , i + 1);
		if(_SUCCESS != status){
			return status;
		}
	}

	// end of list
	status = _LLTWrite(Adapter, (txpktbuf_bndy - 1), 0xFF); 
	if(_SUCCESS != status){
		return status;
	}

	// Make the other pages as ring buffer
	// This ring buffer is used as beacon buffer if we config this MAC as two MAC transfer.
	// Otherwise used as local loopback buffer. 
	for(i = txpktbuf_bndy ; i < Last_Entry_Of_TxPktBuf ; i++){
		status = _LLTWrite(Adapter, i, (i + 1)); 
		if(_SUCCESS != status){
			return status;
		}
	}
	
	// Let last entry point to the start entry of ring buffer
	status = _LLTWrite(Adapter, Last_Entry_Of_TxPktBuf, txpktbuf_bndy);
	if(_SUCCESS != status){
		return status;
	}

	return status;
	
}


u8 usb_hal_bus_init(_adapter *padapter)
{
	u8	val8 = 0;
	u8	ret = _SUCCESS;
	u32	boundary;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	struct registry_priv *pregistrypriv = &padapter->registrypriv;

	if(IS_HARDWARE_TYPE_8192DU(pHalData))
	{
		write8(padapter, REG_RSV_CTRL, 0x0);
		val8 = read8(padapter, 0x0003);
		val8 &= (~BIT7);
		write8(padapter, 0x0003, val8);
		// mac1 should set 0x3[7] to enable mac reg
		val8 |= BIT7;
		write8( padapter,  0x0003, val8);		
	}

	//mac status:
	//0x81[4]:0 mac0 off, 1:mac0 on
	//0x82[4]:0 mac1 off, 1: mac1 on.	
	if(IS_HARDWARE_TYPE_8192D(pHalData))
	{
		if(pHalData->interfaceIndex == 0)
		{
			val8 = read8(padapter, REG_MAC0);
			write8(padapter, REG_MAC0, val8|MAC0_ON);
		}
		else
		{
			val8 = read8(padapter, REG_MAC1);
			write8(padapter, REG_MAC1, val8|MAC1_ON);
		}
	}

	ret = _InitPowerOn(padapter);
	if(ret==_FAIL)
		return ret;


	if(!pregistrypriv->wifi_spec){
		boundary = TX_PAGE_BOUNDARY;
	}
	else{// for WMM
		boundary = (IS_NORMAL_CHIP(pHalData->VersionID))	?WMM_NORMAL_TX_PAGE_BOUNDARY
														:WMM_TEST_TX_PAGE_BOUNDARY;
	}

	if(IS_HARDWARE_TYPE_8192DU(pHalData)){
		if(IS_NORMAL_CHIP( pHalData->VersionID)){
			switch(pHalData->MacPhyMode92D)
			{
				case  DUALMAC_DUALPHY:
					write8(padapter,0xf8, 0xf3); //Enable DMDP
					// dual mac coexistence,temply set 0 now, defaut set 0xbf  for dual phy
					write8(padapter,0x5f0,0);
					break;
				case  DUALMAC_SINGLEPHY:
					write8(padapter,0xf8, 0xf1); //Enable DMSP
					write8(padapter,0x5f0,0xf8);
					break;
				case  SINGLEMAC_SINGLEPHY:
					write8(padapter,0xf8, 0xf4); //Enable SMSP
					write8(padapter,0x5f0,0);
					break;
			}
		}
		else{
			switch(pHalData->MacPhyMode92D)
			{
				case  DUALMAC_DUALPHY:
					write8(padapter,0x2c, 0xf3); //Enable DMDP
					// dual mac coexistence,temply set 0 now, defaut set 0xbf  for dual phy
					write8(padapter,0x5f0,0);
					break;
				case  DUALMAC_SINGLEPHY:
					write8(padapter,0x2c, 0xf1); //Enable DMSP
					write8(padapter,0x5f0,0xf8);
					break;
				case  SINGLEMAC_SINGLEPHY:
					write8(padapter,0x2c, 0xf4); //Enable SMSP
					write8(padapter,0x5f0,0);
					break;
			}
		}
	}

	ret =  InitLLTTable(padapter, boundary);
	if(ret == _FAIL){
		//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to init power on!\n"));
		return ret;
	}	

	return ret;

}

VOID 
_DisableGPIO(
	IN	PADAPTER	Adapter
	)
{
/***************************************
j. GPIO_PIN_CTRL 0x44[31:0]=0x000		// 
k. Value = GPIO_PIN_CTRL[7:0]
l.  GPIO_PIN_CTRL 0x44[31:0] = 0x00FF0000 | (value <<8); //write external PIN level
m. GPIO_MUXCFG 0x42 [15:0] = 0x0780
n. LEDCFG 0x4C[15:0] = 0x8080
***************************************/
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);
	u8	value8;
	u16	value16;
	u32	value32;

	//1. Disable GPIO[7:0]
	write16(Adapter, REG_GPIO_PIN_CTRL+2, 0x0000);
    	value32 = read32(Adapter, REG_GPIO_PIN_CTRL) & 0xFFFF00FF;  
	value8 = (u8) (value32&0x000000FF);
	value32 |= ((value8<<8) | 0x00FF0000);
	write32(Adapter, REG_GPIO_PIN_CTRL, value32);
	      
	//2. Disable GPIO[10:8]          
	write8(Adapter, REG_GPIO_MUXCFG+3, 0x00);
	    value16 = read16(Adapter, REG_GPIO_MUXCFG+2) & 0xFF0F;  
	value8 = (u8) (value16&0x000F);
	value16 |= ((value8<<4) | 0x0780);
	write16(Adapter, REG_GPIO_MUXCFG+2, value16);

	//3. Disable LED0 & 1
	if(IS_HARDWARE_TYPE_8192DU(pHalData)){
		write16(Adapter, REG_LEDCFG0, 0x8888);
	}
	else{
		write16(Adapter, REG_LEDCFG, 0x8080);
	}

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Disable GPIO and LED.\n"));
 
} //end of _DisableGPIO()

VOID
_ResetFWDownloadRegister(
	IN PADAPTER			Adapter
	)
{
	u32	value32;

	value32 = read32(Adapter, REG_MCUFWDL);
	value32 &= ~(MCUFWDL_EN | MCUFWDL_RDY);
	write32(Adapter, REG_MCUFWDL, value32);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reset FW download register.\n"));
}


int
_DisableRF_AFE(
	IN PADAPTER			Adapter
	)
{
	int		rtStatus = _SUCCESS;
	u32			pollingCount = 0;
	u8			value8;
	
	//disable RF/ AFE AD/DA
	value8 = APSDOFF;
	write8(Adapter, REG_APSD_CTRL, value8);


#if (RTL8192CU_ASIC_VERIFICATION)

	do
	{
		if(read8(Adapter, REG_APSD_CTRL) & APSDOFF_STATUS){
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("Disable RF, AFE, AD, DA Done!\n"));
			break;
		}

		if(pollingCount++ > POLLING_READY_TIMEOUT_COUNT){
			//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Failed to polling APSDOFF_STATUS done!\n"));
			return _FAIL;
		}
				
	}while(_TRUE);
	
#endif

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Disable RF, AFE,AD, DA.\n"));
	return rtStatus;

}

VOID
_ResetBB(
	IN PADAPTER			Adapter
	)
{
	u16	value16;

	//reset BB
	value16 = read16(Adapter, REG_SYS_FUNC_EN);
	value16 &= ~(FEN_BBRSTB | FEN_BB_GLB_RSTn);
	write16(Adapter, REG_SYS_FUNC_EN, value16);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reset BB.\n"));
}

VOID
_ResetMCU(
	IN PADAPTER			Adapter
	)
{
	u16	value16;
	
	// reset MCU
	value16 = read16(Adapter, REG_SYS_FUNC_EN);
	value16 &= ~FEN_CPUEN;
	write16(Adapter, REG_SYS_FUNC_EN, value16);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reset MCU.\n"));
}

VOID
_DisableMAC_AFE_PLL(
	IN PADAPTER			Adapter
	)
{
	u32	value32;
	
	//disable MAC/ AFE PLL
	value32 = read32(Adapter, REG_APS_FSMCO);
	value32 |= APDM_MAC;
	write32(Adapter, REG_APS_FSMCO, value32);
	
	value32 |= APFM_OFF;
	write32(Adapter, REG_APS_FSMCO, value32);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Disable MAC, AFE PLL.\n"));
}

VOID
_AutoPowerDownToHostOff(
	IN	PADAPTER		Adapter
	)
{
	u32			value32;
	write8(Adapter, REG_SPS0_CTRL, 0x22);

	value32 = read32(Adapter, REG_APS_FSMCO);	
	
	value32 |= APDM_HOST;//card disable
	write32(Adapter, REG_APS_FSMCO, value32);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Auto Power Down to Host-off state.\n"));

	// set USB suspend
	value32 = read32(Adapter, REG_APS_FSMCO);
	value32 &= ~AFSM_PCIE;
	write32(Adapter, REG_APS_FSMCO, value32);

}

VOID
_SetUsbSuspend(
	IN PADAPTER			Adapter
	)
{
	u32			value32;

	value32 = read32(Adapter, REG_APS_FSMCO);
	
	// set USB suspend
	value32 |= AFSM_HSUS;
	write32(Adapter, REG_APS_FSMCO, value32);

	//RT_ASSERT(0 == (read32(Adapter, REG_APS_FSMCO) & BIT(12)),(""));
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Set USB suspend.\n"));
	
}

VOID
_DisableRFAFEAndResetBB(
	IN PADAPTER			Adapter
	)
{
/**************************************
a.	TXPAUSE 0x522[7:0] = 0xFF             //Pause MAC TX queue
b.	RF path 0 offset 0x00 = 0x00            // disable RF
c. 	APSD_CTRL 0x600[7:0] = 0x40
d.	SYS_FUNC_EN 0x02[7:0] = 0x16		//reset BB state machine
e.	SYS_FUNC_EN 0x02[7:0] = 0x14		//reset BB state machine
***************************************/
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);
	u8 eRFPath = 0,value8 = 0;
	write8(Adapter, REG_TXPAUSE, 0xFF);

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)	
	if(!Adapter->bSlaveOfDMSP)
#endif
	
	PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x0, bMaskByte0, 0x0);

	value8 |= APSDOFF;
	write8(Adapter, REG_APSD_CTRL, value8);//0x40

	if(!IS_HARDWARE_TYPE_8192DU(pHalData) )
	{
		value8 = 0;
		value8 |=( FEN_USBD | FEN_USBA | FEN_BB_GLB_RSTn);
		write8(Adapter, REG_SYS_FUNC_EN,value8 );//0x16

		value8 &=( ~FEN_BB_GLB_RSTn );
		write8(Adapter, REG_SYS_FUNC_EN, value8); //0x14
	}

	if(pHalData->interfaceIndex==0)
	{	// query another mac state;
		value8 = read8(Adapter, REG_MAC1);
		value8&=MAC1_ON;
	}
	else
	{
		value8 = read8(Adapter, REG_MAC0);
		value8&=MAC0_ON;
	}

	if(IS_HARDWARE_TYPE_8192DU(pHalData)&&(value8==0)){			
		//testchip  should not do BB reset if another mac is alive;
		value8 = 0 ; 
		value8 |=( FEN_USBD | FEN_USBA | FEN_BB_GLB_RSTn);
		write8(Adapter, REG_SYS_FUNC_EN,value8 );//0x16		

		value8 &=( ~FEN_BB_GLB_RSTn );
		write8(Adapter, REG_SYS_FUNC_EN, value8); //0x14

		//0x20:value 05-->04
		write8(Adapter, REG_LDOA15_CTRL,0x04);
		//RF Control 
		write8(Adapter, REG_RF_CTRL,0);
	}

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> RF off and reset BB.\n"));
}

VOID
_ResetDigitalProcedure1(
	IN 	PADAPTER			Adapter,
	IN	BOOLEAN				bWithoutHWSM	
	)
{

	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);

	if((pHalData->FirmwareVersion <=  0x20)&&(!IS_HARDWARE_TYPE_8192D(pHalData))){
		#if 0
		/*****************************
		f.	SYS_FUNC_EN 0x03[7:0]=0x54		// reset MAC register, DCORE
		g.	MCUFWDL 0x80[7:0]=0				// reset MCU ready status
		******************************/
		u4Byte	value32 = 0;
		PlatformIOWrite1Byte(Adapter, REG_SYS_FUNC_EN+1, 0x54);
		PlatformIOWrite1Byte(Adapter, REG_MCUFWDL, 0);	
		#else
		/*****************************
		f.	MCUFWDL 0x80[7:0]=0				// reset MCU ready status
		g.	SYS_FUNC_EN 0x02[10]= 0			// reset MCU register, (8051 reset)
		h.	SYS_FUNC_EN 0x02[15-12]= 5		// reset MAC register, DCORE
		i.     SYS_FUNC_EN 0x02[10]= 1			// enable MCU register, (8051 enable)
		******************************/
			u16 valu16 = 0;
			write8(Adapter, REG_MCUFWDL, 0);

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
			if(IS_NORMAL_CHIP(pHalData->VersionID))
#endif
			{
				valu16 = read16(Adapter, REG_SYS_FUNC_EN);
				write16(Adapter, REG_SYS_FUNC_EN, (valu16 & (~FEN_CPUEN)));//reset MCU ,8051
			}
			valu16 = read16(Adapter, REG_SYS_FUNC_EN)&0x0FFF;
			write16(Adapter, REG_SYS_FUNC_EN, (valu16 |(FEN_HWPDN|FEN_ELDR)));//reset MAC
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
			if(IS_NORMAL_CHIP(pHalData->VersionID))
#endif
			{
				valu16 = read16(Adapter, REG_SYS_FUNC_EN);
				write16(Adapter, REG_SYS_FUNC_EN, (valu16 | FEN_CPUEN));//enable MCU ,8051
			}

		
		#endif
	}
	else{
		u8 retry_cnts = 0;	
		if(IS_NORMAL_CHIP(pHalData->VersionID)||!IS_HARDWARE_TYPE_8192D(pHalData)){
			if(read8(Adapter, REG_MCUFWDL) & BIT1){ //IF fw in RAM code, do reset 

				write8(Adapter, REG_MCUFWDL, 0);
				if(Adapter->bFWReady){
					write8(Adapter, REG_HMETFR+3, 0x20);//8051 reset by self
					while( (retry_cnts++ <100) && (FEN_CPUEN &read16(Adapter, REG_SYS_FUNC_EN)))
					{					
						mdelay_os(50);//PlatformStallExecution(50);//us
					}
					//RT_ASSERT((retry_cnts < 100), ("8051 reset failed!\n"));			
					//RT_TRACE(COMP_INIT, DBG_LOUD, ("=====> 8051 reset success (%d) .\n",retry_cnts));
				}
			}
			else{
				//RT_TRACE(COMP_INIT, DBG_LOUD, ("=====> 8051 in ROM.\n"));
			}
		}
		
		write8(Adapter, REG_SYS_FUNC_EN+1, 0x54);	//Reset MAC and Enable 8051
	}			

	if(bWithoutHWSM){
	/*****************************
		Without HW auto state machine
	g.	SYS_CLKR 0x08[15:0] = 0x30A3			//disable MAC clock
	h.	AFE_PLL_CTRL 0x28[7:0] = 0x80			//disable AFE PLL
	i.	AFE_XTAL_CTRL 0x24[15:0] = 0x880F		//gated AFE DIG_CLOCK
	j.	SYS_ISO_CTRL 0x00[7:0] = 0xF9			// isolated digital to PON
	******************************/	
		//write16(Adapter, REG_SYS_CLKR, 0x30A3);
		write16(Adapter, REG_SYS_CLKR, 0x70A3);  //modify to 0x70A3 by Scott.
		write8(Adapter, REG_AFE_PLL_CTRL, 0x80);		
		write16(Adapter, REG_AFE_XTAL_CTRL, 0x880F);
		write8(Adapter, REG_SYS_ISO_CTRL, 0xF9);		
	}
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Reset Digital.\n"));

}

VOID
_ResetDigitalProcedure2(
	IN 	PADAPTER			Adapter
)
{
/*****************************
k.	SYS_FUNC_EN 0x03[7:0] = 0x44			// disable ELDR runction
l.	SYS_CLKR 0x08[15:0] = 0x3083			// disable ELDR clock
m.	SYS_ISO_CTRL 0x01[7:0] = 0x83			// isolated ELDR to PON
******************************/
	//write8(Adapter, REG_SYS_FUNC_EN+1, 0x44);//marked by Scott.
	write16(Adapter, REG_SYS_CLKR, 0x70a3); //modify to 0x70a3 by Scott.
	write8(Adapter, REG_SYS_ISO_CTRL+1, 0x82); //modify to 0x82 by Scott.
}

VOID
_DisableAnalog(
	IN PADAPTER			Adapter,
	IN BOOLEAN			bWithoutHWSM	
	)
{
	u32 value16 = 0;
	u8 value8=0;
	
	if(bWithoutHWSM){
	/*****************************
	n.	LDOA15_CTRL 0x20[7:0] = 0x04		// disable A15 power
	o.	LDOV12D_CTRL 0x21[7:0] = 0x54		// disable digital core power
	r.	When driver call disable, the ASIC will turn off remaining clock automatically 
	******************************/
	
		write8(Adapter, REG_LDOA15_CTRL, 0x04);
		//PlatformIOWrite1Byte(Adapter, REG_LDOV12D_CTRL, 0x54);		
		
		value8 = read8(Adapter, REG_LDOV12D_CTRL);		
		value8 &= (~LDV12_EN);
		write8(Adapter, REG_LDOV12D_CTRL, value8);	
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" REG_LDOV12D_CTRL Reg0x21:0x%02x.\n",value8));
	}
	
/*****************************
h.	SPS0_CTRL 0x11[7:0] = 0x23			//enter PFM mode
i.	APS_FSMCO 0x04[15:0] = 0x4802		// set USB suspend 
******************************/	
	write8(Adapter, REG_SPS0_CTRL, 0x23);
	
	value16 |= (APDM_HOST | AFSM_HSUS |PFM_ALDN);
	write16(Adapter, REG_APS_FSMCO,value16 );//0x4802 

	write8(Adapter, REG_RSV_CTRL, 0x0e );

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Disable Analog Reg0x04:0x%04x.\n",value16));
}

BOOLEAN
CanGotoPowerOff92D(
	IN	PADAPTER			Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8 u1bTmp;
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	PADAPTER	BuddyAdapter = Adapter->BuddyAdapter;
#endif
	
      if(!IS_HARDWARE_TYPE_8192DU(pHalData))
	  	return _TRUE;
	  
	if(pHalData->MacPhyMode92D==SINGLEMAC_SINGLEPHY)
		return _TRUE;	  
	
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(BuddyAdapter != NULL)
	{
		if(BuddyAdapter->MgntInfo.init_adpt_in_progress)
		{
			RT_TRACE(COMP_DUALMACSWITCH,DBG_LOUD,("do not power off during another adapter is initialization \n"));
			return _FALSE;
		}
	}

#endif

	if(pHalData->interfaceIndex==0)
	{	// query another mac status;
		u1bTmp = read8(Adapter, REG_MAC1);
		u1bTmp&=MAC1_ON;
	}
	else
	{
		u1bTmp = read8(Adapter, REG_MAC0);
		u1bTmp&=MAC0_ON;
	}

	if(u1bTmp)
	{
		// DMSP mode, when two mac are disable,then can go to power off sequence
		// DMDP mode, when one mac disable ,disable its rf/bb	
		if(pHalData->MacPhyMode92D==DUALMAC_DUALPHY)
		{
			_DisableRFAFEAndResetBB(Adapter);
		}
		else
		{
			write8(Adapter, REG_TXPAUSE, 0xFF);
		}		
		return _FALSE;
	}

	//0x17[7]:1b' power off in process
	u1bTmp=read8(Adapter, 0x17);
	u1bTmp|=BIT7;
	write8(Adapter, 0x17, u1bTmp);
	
	udelay_os(500);
	// query another mac status;
	if(pHalData->interfaceIndex==0)
	{	// query another mac status;
		u1bTmp = read8(Adapter, REG_MAC1);
		u1bTmp&=MAC1_ON;
	}
	else
	{
		u1bTmp = read8(Adapter, REG_MAC0);
		u1bTmp&=MAC0_ON;
	}
	//if another mac is alive,do not do power off 
	if(u1bTmp)
	{
		u1bTmp=read8(Adapter, 0x17);
		u1bTmp&=(~BIT7);
		write8(Adapter, 0x17, u1bTmp);
		return _FALSE;
	}

	return _TRUE;
}

int	
CardDisableHWSM( // HW Auto state machine
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			resetMCU
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	int		rtStatus = _SUCCESS;
	u8		value;

	if(Adapter->bSurpriseRemoved){
		return rtStatus;
	}

	if(!CanGotoPowerOff92D(Adapter))
		return rtStatus;
/*
	if(IS_HARDWARE_TYPE_8192D(Adapter))
	{//0x FE10[4] clear before suspend
	   value=PlatformEFIORead1Byte(Adapter,0xfe10);
	   value&=(~BIT4);
	   PlatformEFIOWrite1Byte(Adapter,0xfe10,value);
	}*/
#if 1
	//==== RF Off Sequence ====
	_DisableRFAFEAndResetBB(Adapter);

	//  ==== Reset digital sequence   ======
	_ResetDigitalProcedure1(Adapter, _FALSE);
	
	//  ==== Pull GPIO PIN to balance level and LED control ======
	_DisableGPIO(Adapter);

	//  ==== Disable analog sequence ===
	_DisableAnalog(Adapter, _FALSE);

	if(IS_HARDWARE_TYPE_8192D(pHalData)){
		value=read8(Adapter, 0x17);
		value&=(~BIT7);
		write8(Adapter, 0x17, value);
	}

	RT_TRACE(_module_hci_hal_init_c_, _drv_info_, ("======> Card disable finished.\n"));
#else
	_DisableGPIO(Adapter);
	
	//reset FW download register
	_ResetFWDownloadRegister(Adapter);


	//disable RF/ AFE AD/DA
	rtStatus = _DisableRF_AFE(Adapter);
	if(RT_STATUS_SUCCESS != rtStatus){
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("_DisableRF_AFE failed!\n"));
		goto Exit;
	}
	_ResetBB(Adapter);

	if(resetMCU){
		_ResetMCU(Adapter);
	}

	_AutoPowerDownToHostOff(Adapter);
	//_DisableMAC_AFE_PLL(Adapter);
	
	_SetUsbSuspend(Adapter);
Exit:
#endif
	return rtStatus;
	
}

int	
CardDisableWithoutHWSM( // without HW Auto state machine
	IN	PADAPTER		Adapter	
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	int		rtStatus = _SUCCESS;
	u8		value;

#if FW_PROCESS_VENDOR_CMD
	Adapter->bFWReady = _FALSE;
#endif
	
	if(Adapter->bSurpriseRemoved){
		return rtStatus;
	}

//set Mac is disabled for dual mac mode
//it is used to avoid power off when 1 is RF on while another is RF OFF
//by sherry 20100519

	if(IS_HARDWARE_TYPE_8192DU(pHalData)){
		u8    u1bTmp;

		if(pHalData->interfaceIndex == 0){
			u1bTmp = read8(Adapter, REG_MAC0);
			write8(Adapter, REG_MAC0, u1bTmp&(~MAC0_ON));
		}
		else{
			u1bTmp = read8(Adapter, REG_MAC1);
			write8(Adapter, REG_MAC1, u1bTmp&(~MAC1_ON));
		}

		// stop tx/rx 
		write8(Adapter, REG_TXPAUSE, 0xFF);
		udelay_os(500); 
		write8(Adapter,	REG_CR, 0x0);
	}

	if(!CanGotoPowerOff92D(Adapter))
	return rtStatus;
	
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Card Disable Without HWSM .\n"));
	//==== RF Off Sequence ====
	_DisableRFAFEAndResetBB(Adapter);

	//  ==== Reset digital sequence   ======
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	_ResetDigitalProcedure1(Adapter, _FALSE);
#else
	_ResetDigitalProcedure1(Adapter, _TRUE);
#endif

	//  ==== Pull GPIO PIN to balance level and LED control ======
	_DisableGPIO(Adapter);

	//  ==== Reset digital sequence   ======
	_ResetDigitalProcedure2(Adapter);

	//  ==== Disable analog sequence ===
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	_DisableAnalog(Adapter, _FALSE);
#else
	_DisableAnalog(Adapter, _TRUE);
#endif

	if(IS_HARDWARE_TYPE_8192D(pHalData)){
		value=read8(Adapter, 0x17);
		value&=(~BIT7);
		write8(Adapter, 0x17, value);
	}

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("<====== Card Disable Without HWSM .\n"));
	return rtStatus;
}

 u8 usb_hal_bus_deinit(_adapter *padapter)
 {
	u8	u1bTmp;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

_func_enter_;

	if(IS_HARDWARE_TYPE_8192DU(pHalData)){
		if(pHalData->interfaceIndex == 0){
			u1bTmp = read8(padapter, REG_MAC0);
			write8(padapter, REG_MAC0, u1bTmp&(~MAC0_ON));
		}
		else{
			u1bTmp = read8(padapter, REG_MAC1);
			write8(padapter, REG_MAC1, u1bTmp&(~MAC1_ON));
		}
	}

	CardDisableHWSM(padapter, _FALSE);

	if(IS_HARDWARE_TYPE_8192D(pHalData))
	{//0xFE10[4] clear before suspend	 suggested by zhouzhou
		u1bTmp=read8(padapter,0xfe10);
		u1bTmp&=(~BIT4);
		write8(padapter,0xfe10,u1bTmp);

		if(pHalData->bInSetPower && (!IS_NORMAL_CHIP(pHalData->VersionID)))
		{
			// S3 or s4   FW 8051 go to rom
			write8(padapter,0x1cf,0x40);
		}
	}

_func_exit_;
	
	return _SUCCESS;
 }

unsigned int usb_inirp_init(_adapter * padapter)
{	
	u8 i;	
	struct recv_buf *precvbuf;
	uint	status;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	struct intf_hdl * pintfhdl=&padapter->pio_queue->intf;
	struct recv_priv *precvpriv = &(padapter->recvpriv);

_func_enter_;

	status = _SUCCESS;

	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("===> usb_inirp_init \n"));	
		
	precvpriv->ff_hwaddr = RECV_BULK_IN_ADDR;

	//issue Rx irp to receive data	
	precvbuf = (struct recv_buf *)precvpriv->precv_buf;	
	for(i=0; i<NR_RECVBUFF; i++)
	{
		if(usb_read_port(pintfhdl, precvpriv->ff_hwaddr, 0, (unsigned char *)precvbuf) == _FALSE )
		{
			RT_TRACE(_module_hci_hal_init_c_,_drv_err_,("usb_rx_init: usb_read_port error \n"));
			status = _FAIL;
			goto exit;
		}
		
		precvbuf++;		
		precvpriv->free_recv_buf_queue_cnt--;
	}

		
exit:
	
	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("<=== usb_inirp_init \n"));

_func_exit_;

	return status;

}

unsigned int usb_inirp_deinit(_adapter * padapter)
{	
	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("\n ===> usb_rx_deinit \n"));
	
	usb_read_port_cancel(padapter);


	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("\n <=== usb_rx_deinit \n"));

	return _SUCCESS;
}

