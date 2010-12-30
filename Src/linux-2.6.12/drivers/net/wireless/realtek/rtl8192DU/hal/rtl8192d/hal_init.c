/******************************************************************************
* hal_init.c                                                                                                                                 *
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

#define _HAL_INIT_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtl871x_byteorder.h>

#include <hal_init.h>
#include <rtl8712_efuse.h>

#ifdef CONFIG_SDIO_HCI
			#include <sdio_hal.h>
#ifdef PLATFORM_LINUX
	#include <linux/mmc/sdio_func.h>
#endif
#elif defined(CONFIG_USB_HCI)
			#include <usb_hal.h>
#endif	

//---------------------------------------------------------------
//
//	MAC init functions
//
//---------------------------------------------------------------
VOID
_SetMacID(
	IN  PADAPTER Adapter, u8* MacID
	)
{
	u32 i;
	for(i=0 ; i< MAC_ADDR_LEN ; i++){
		write32(Adapter, REG_MACID+i, MacID[i]);
	}
}

VOID
_SetBSSID(
	IN  PADAPTER Adapter, u8* BSSID
	)
{
	u32 i;
	for(i=0 ; i< MAC_ADDR_LEN ; i++){
		write32(Adapter, REG_BSSID+i, BSSID[i]);
	}
}


// Shall USB interface init this?
VOID
_InitInterrupt(
	IN  PADAPTER Adapter
	)
{
	u32	value32;

	// HISR - turn all on
	value32 = 0xFFFFFFFF;
	write32(Adapter, REG_HISR, value32);

	// HIMR - turn all on
	write32(Adapter, REG_HIMR, value32);
}


VOID
_InitQueueReservedPage(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	BOOLEAN			isNormalChip = IS_NORMAL_CHIP(pHalData->VersionID);
	
	u32			outEPNum	= (u32)pHalData->OutEpNumber;
	u32			numHQ		= 0;
	u32			numLQ		= 0;
	u32			numNQ		= 0;
	u32			numPubQ;
	u32			value32;
	u8			value8;
	u32			txQPageNum, txQPageUnit,txQRemainPage;


	if(!pregistrypriv->wifi_spec){		
		numPubQ = (isNormalChip) ? NORMAL_PAGE_NUM_PUBQ : TEST_PAGE_NUM_PUBQ;
		//RT_ASSERT((numPubQ < TX_TOTAL_PAGE_NUMBER), ("Public queue page number is great than total tx page number.\n"));
		txQPageNum = TX_TOTAL_PAGE_NUMBER - numPubQ;

		//RT_ASSERT((0 == txQPageNum%txQPageNum), ("Total tx page number is not dividable!\n"));

		if(pHalData->MacPhyMode92D !=SINGLEMAC_SINGLEPHY)	
		{
			numPubQ = TX_TOTAL_PAGE_NUMBER_92DNORMAL;//TEST_PAGE_NUM_PUBQ_92DNORMAL;//TEST_PAGE_NUM_PUBQ_92DNORMAL;
			//RT_ASSERT((numPubQ < TX_TOTAL_PAGE_NUMBER), ("Public queue page number is great than total tx page number.\n"));
			txQPageNum = TX_TOTAL_PAGE_NUMBER_92DNORMAL- numPubQ;	
		}

		txQPageUnit = txQPageNum/outEPNum;
		txQRemainPage = txQPageNum % outEPNum;

		if(pHalData->OutEpQueueSel & TX_SELE_HQ){
			numHQ = txQPageUnit;
		}
		if(pHalData->OutEpQueueSel & TX_SELE_LQ){
			numLQ = txQPageUnit;
		}
		// HIGH priority queue always present in the configuration of 2 or 3 out-ep 
		// so ,remainder pages have assigned to High queue
		if((outEPNum>1) && (txQRemainPage)){			
			numHQ += txQRemainPage;
		}

		// NOTE: This step shall be proceed before writting REG_RQPN.
		if(isNormalChip ||IS_HARDWARE_TYPE_8192DU(pHalData)){
			if(pHalData->OutEpQueueSel & TX_SELE_NQ){
				numNQ = txQPageUnit;
			}
			value8 = (u8)_NPQ(numNQ);
			write8(Adapter, REG_RQPN_NPQ, value8);
		}
		//RT_ASSERT(((numHQ + numLQ + numNQ + numPubQ) < TX_PAGE_BOUNDARY), ("Total tx page number is greater than tx boundary!\n"));
	}
	else{ //for WMM 
		//RT_ASSERT((outEPNum>=2), ("for WMM ,number of out-ep must more than or equal to 2!\n"));

		if(pHalData->HardwareType != HARDWARE_TYPE_RTL8192DU)
		{
			numPubQ = (isNormalChip) 	?WMM_NORMAL_PAGE_NUM_PUBQ
									:WMM_TEST_PAGE_NUM_PUBQ;		
			
			if(pHalData->OutEpQueueSel & TX_SELE_HQ){
				numHQ = (isNormalChip)?WMM_NORMAL_PAGE_NUM_HPQ
									:WMM_TEST_PAGE_NUM_HPQ;
			}

			if(pHalData->OutEpQueueSel & TX_SELE_LQ){
				numLQ = (isNormalChip)?WMM_NORMAL_PAGE_NUM_LPQ
									:WMM_TEST_PAGE_NUM_LPQ;
			}
			// NOTE: This step shall be proceed before writting REG_RQPN.
			if(isNormalChip){			
				if(pHalData->OutEpQueueSel & TX_SELE_NQ){
					numNQ = WMM_NORMAL_PAGE_NUM_NPQ;
				}
				value8 = (u8)_NPQ(numNQ);
				write8(Adapter, REG_RQPN_NPQ, value8);
			}
		}
		else
		{
			// 92du wifi config only for SMSP
			numPubQ =WMM_NORMAL_PAGE_NUM_PUBQ_92D;		

			if(pHalData->OutEpQueueSel & TX_SELE_HQ){
				numHQ = WMM_NORMAL_PAGE_NUM_HPQ_92D;									
								}

			if(pHalData->OutEpQueueSel & TX_SELE_LQ){
				numLQ = WMM_NORMAL_PAGE_NUM_LPQ_92D;
									
				}
	
			if(pHalData->OutEpQueueSel & TX_SELE_NQ){
				numNQ = WMM_NORMAL_PAGE_NUM_NPQ_92D;
				value8 = (u8)_NPQ(numNQ);
				write8(Adapter, REG_RQPN_NPQ, value8);
			}
		}
	}

	// TX DMA
	value32 = _HPQ(numHQ) | _LPQ(numLQ) | _PUBQ(numPubQ) | LD_RQPN;	
	write32(Adapter, REG_RQPN, value32);	
}

void _InitID(IN  PADAPTER Adapter)
{
	int i;	 
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	
	for(i=0; i<6; i++)
	{
		write8(Adapter, (REG_MACID+i), pEEPROM->mac_addr[i]);		 	
	}

/*
	NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);
	//Ziv test
#if 1
	{
		u1Byte sMacAddr[6] = {0};
		u4Byte i;
		
		for(i = 0 ; i < MAC_ADDR_LEN ; i++){
			sMacAddr[i] = PlatformIORead1Byte(Adapter, (REG_MACID + i));
		}
		RT_PRINT_ADDR(COMP_INIT|COMP_EFUSE, DBG_LOUD, "Read back MAC Addr: ", sMacAddr);
	}
#endif

#if 0
	u4Byte nMAR 	= 0xFFFFFFFF;
	u8 m_MacID[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	u8 m_BSSID[] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	int i;
	
	_SetMacID(Adapter, Adapter->PermanentAddress);
	_SetBSSID(Adapter, m_BSSID);

	//set MAR
	PlatformIOWrite4Byte(Adapter, REG_MAR, nMAR);
	PlatformIOWrite4Byte(Adapter, REG_MAR+4, nMAR);
#endif
*/
}


VOID
_InitTxBufferBoundary(
	IN  PADAPTER Adapter
	)
{	
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	u16	txdmactrl;	
	u8	txpktbuf_bndy; 

	if(!pregistrypriv->wifi_spec){
		txpktbuf_bndy = TX_PAGE_BOUNDARY;
	}
	else{//for WMM
		txpktbuf_bndy = ( IS_NORMAL_CHIP( pHalData->VersionID))?WMM_NORMAL_TX_PAGE_BOUNDARY
															:WMM_TEST_TX_PAGE_BOUNDARY;
	}

	if(IS_HARDWARE_TYPE_8192DU(pHalData))	
	{
		if(pHalData->MacPhyMode92D ==SINGLEMAC_SINGLEPHY)
			txpktbuf_bndy = 253;
		else
			txpktbuf_bndy = 125;
	}
	write8(Adapter, REG_TXPKTBUF_BCNQ_BDNY, txpktbuf_bndy);
	write8(Adapter, REG_TXPKTBUF_MGQ_BDNY, txpktbuf_bndy);
	write8(Adapter, REG_TXPKTBUF_WMAC_LBK_BF_HD, txpktbuf_bndy);
	write8(Adapter, REG_TRXFF_BNDY, txpktbuf_bndy);	
#if 1
	write8(Adapter, REG_TDECTRL+1, txpktbuf_bndy);
#else
	txdmactrl = PlatformIORead2Byte(Adapter, REG_TDECTRL);
	txdmactrl &= ~BCN_HEAD_MASK;
	txdmactrl |= BCN_HEAD(txpktbuf_bndy);
	PlatformIOWrite2Byte(Adapter, REG_TDECTRL, txdmactrl);
#endif
}

VOID
_InitPageBoundary(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	// RX Page Boundary
	//srand(static_cast<unsigned int>(time(NULL)) );
	u16 rxff_bndy = 0x27FF;//(rand() % 1) ? 0x27FF : 0x23FF;

	if(IS_HARDWARE_TYPE_8192DU(pHalData))
	{
		if(pHalData->MacPhyMode92D==SINGLEMAC_SINGLEPHY)
		{
			if(IS_NORMAL_CHIP( pHalData->VersionID))
					rxff_bndy = 0x27FF;
			else
					rxff_bndy = 0x13FF;
		}
		else
			rxff_bndy = 0x13FF;	
	}

	write16(Adapter, (REG_TRXFF_BNDY + 2), rxff_bndy);

	// TODO: ?? shall we set tx boundary?
}


VOID
_InitNormalChipRegPriority(
	IN	PADAPTER	Adapter,
	IN	u16		beQ,
	IN	u16		bkQ,
	IN	u16		viQ,
	IN	u16		voQ,
	IN	u16		mgtQ,
	IN	u16		hiQ
	)
{
	u16 value16		= (read16(Adapter, REG_TRXDMA_CTRL) & 0x7);

	value16 |=	_TXDMA_BEQ_MAP(beQ) 	| _TXDMA_BKQ_MAP(bkQ) |
				_TXDMA_VIQ_MAP(viQ) 	| _TXDMA_VOQ_MAP(voQ) |
				_TXDMA_MGQ_MAP(mgtQ)| _TXDMA_HIQ_MAP(hiQ);
	
	write16(Adapter, REG_TRXDMA_CTRL, value16);
}

VOID
_InitNormalChipOneOutEpPriority(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	u16	value = 0;
	switch(pHalData->OutEpQueueSel)
	{
		case TX_SELE_HQ:
			value = QUEUE_HIGH;
			break;
		case TX_SELE_LQ:
			value = QUEUE_LOW;
			break;
		case TX_SELE_NQ:
			value = QUEUE_NORMAL;
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}
	
	_InitNormalChipRegPriority(Adapter,
								value,
								value,
								value,
								value,
								value,
								value
								);

}

VOID
_InitNormalChipTwoOutEpPriority(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	u16			beQ,bkQ,viQ,voQ,mgtQ,hiQ;
	

	u16	valueHi = 0;
	u16	valueLow = 0;
	
	switch(pHalData->OutEpQueueSel)
	{
		case (TX_SELE_HQ | TX_SELE_LQ):
			valueHi = QUEUE_HIGH;
			valueLow = QUEUE_LOW;
			break;
		case (TX_SELE_NQ | TX_SELE_LQ):
			valueHi = QUEUE_NORMAL;
			valueLow = QUEUE_LOW;
			break;
		case (TX_SELE_HQ | TX_SELE_NQ):
			valueHi = QUEUE_HIGH;
			valueLow = QUEUE_NORMAL;
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}

	if(!pregistrypriv->wifi_spec ){
		beQ 		= valueLow;
		bkQ 		= valueLow;
		viQ		= valueHi;
		voQ 		= valueHi;
		mgtQ 	= valueHi; 
		hiQ 		= valueHi;								
	}
	else{//for WMM ,CONFIG_OUT_EP_WIFI_MODE
		beQ		= valueLow;
		bkQ 		= valueHi;		
		viQ 		= valueHi;
		voQ 		= valueLow;
		mgtQ 	= valueHi;
		hiQ 		= valueHi;							
	}
	
	_InitNormalChipRegPriority(Adapter,beQ,bkQ,viQ,voQ,mgtQ,hiQ);

}

VOID
_InitNormalChipThreeOutEpPriority(
	IN	PADAPTER Adapter
	)
{
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	u16			beQ,bkQ,viQ,voQ,mgtQ,hiQ;

	if(!pregistrypriv->wifi_spec ){// typical setting
		beQ		= QUEUE_LOW;
		bkQ 		= QUEUE_LOW;
		viQ 		= QUEUE_NORMAL;
		voQ 		= QUEUE_HIGH;
		mgtQ 	= QUEUE_HIGH;
		hiQ 		= QUEUE_HIGH;			
	}
	else{// for WMM
		beQ		= QUEUE_LOW;
		bkQ 		= QUEUE_NORMAL;
		viQ 		= QUEUE_NORMAL;
		voQ 		= QUEUE_HIGH;
		mgtQ 	= QUEUE_HIGH;
		hiQ 		= QUEUE_HIGH;			
	}
	_InitNormalChipRegPriority(Adapter,beQ,bkQ,viQ,voQ,mgtQ,hiQ);
}

VOID
_InitNormalChipQueuePriority(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	switch(pHalData->OutEpNumber)
	{
		case 1:
			_InitNormalChipOneOutEpPriority(Adapter);
			break;
		case 2:
			_InitNormalChipTwoOutEpPriority(Adapter);
			break;
		case 3:
			_InitNormalChipThreeOutEpPriority(Adapter);
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}


}

VOID
_InitTestChipQueuePriority(
	IN	PADAPTER Adapter
	)
{
	u8	hq_sele ;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	
	switch(pHalData->OutEpNumber)
	{
		case 2:	// (TX_SELE_HQ|TX_SELE_LQ)
			if(!pregistrypriv->wifi_spec)//typical setting			
				hq_sele =  HQSEL_VOQ | HQSEL_VIQ | HQSEL_MGTQ | HQSEL_HIQ ;
			else	//for WMM
				hq_sele = HQSEL_VOQ | HQSEL_BEQ | HQSEL_MGTQ | HQSEL_HIQ ;
			break;
		case 1:
			if(TX_SELE_LQ == pHalData->OutEpQueueSel ){//map all endpoint to Low queue
				 hq_sele = 0;
			}
			else if(TX_SELE_HQ == pHalData->OutEpQueueSel){//map all endpoint to High queue
				hq_sele =  HQSEL_VOQ | HQSEL_VIQ | HQSEL_BEQ | HQSEL_BKQ | HQSEL_MGTQ | HQSEL_HIQ ;
			}		
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}
	write8(Adapter, (REG_TRXDMA_CTRL+1), hq_sele);
}


VOID
_InitQueuePriority(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	if(IS_NORMAL_CHIP( pHalData->VersionID) ||IS_HARDWARE_TYPE_8192DU(pHalData)){
		_InitNormalChipQueuePriority(Adapter);
	}
	else{
		_InitTestChipQueuePriority(Adapter);
	}
}

VOID
_InitHardwareDropIncorrectBulkOut(
	IN  PADAPTER Adapter
	)
{
	u32	value32 = read32(Adapter, REG_TXDMA_OFFSET_CHK);
	value32 |= DROP_DATA_EN;
	write32(Adapter, REG_TXDMA_OFFSET_CHK, value32);
}

VOID
_InitNetworkType(
	IN  PADAPTER Adapter
	)
{
	u32	value32;

	value32 = read32(Adapter, REG_CR);

	// TODO: use the other function to set network type
#if RTL8191C_FPGA_NETWORKTYPE_ADHOC
	value32 = (value32 & ~MASK_NETTYPE) | _NETTYPE(NT_LINK_AD_HOC);
#else
	value32 = (value32 & ~MASK_NETTYPE) | _NETTYPE(NT_LINK_AP);
#endif
	write32(Adapter, REG_CR, value32);
//	RASSERT(pIoBase->read8(REG_CR + 2) == 0x2);
}

VOID
_InitTransferPageSize(
	IN  PADAPTER Adapter
	)
{
	// Tx page size is always 128.
	
	u8	value8;
	value8 = _PSRX(PBP_128) | _PSTX(PBP_128);
	write8(Adapter, REG_PBP, value8);
}

VOID
_InitDriverInfoSize(
	IN  PADAPTER	Adapter,
	IN	u8		drvInfoSize
	)
{
	write8(Adapter,REG_RX_DRVINFO_SZ, drvInfoSize);
}

VOID
_InitWMACSetting(
	IN  PADAPTER Adapter
	)
{
	//u4Byte			value32;
	u16			value16;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	//pHalData->ReceiveConfig = AAP | APM | AM | AB | APP_ICV | ADF | AMF | APP_FCS | HTC_LOC_CTRL | APP_MIC | APP_PHYSTS;
	pHalData->ReceiveConfig = AAP | APM | AM | AB | APP_ICV | ADF | AMF | HTC_LOC_CTRL | APP_MIC | APP_PHYSTS;
#if (0 == RTL8192C_RX_PACKET_NO_INCLUDE_CRC)
	pHalData->ReceiveConfig |= ACRC32;
#endif

	// some REG_RCR will be modified later by phy_ConfigMACWithHeaderFile()
	write32(Adapter, REG_RCR, pHalData->ReceiveConfig);

	// Accept all data frames
	value16 = 0xFFFF;
	write16(Adapter, REG_RXFLTMAP2, value16);

	// Accept all management frames
	value16 = 0xFFFF;
	write16(Adapter, REG_RXFLTMAP0, value16);
	
	
}

VOID
_InitAdaptiveCtrl(
	IN  PADAPTER Adapter
	)
{
	u16	value16;
	u32	value32;

	// Response Rate Set
	value32 = read32(Adapter, REG_RRSR);
	value32 &= ~RATE_BITMAP_ALL;
	value32 |= RATE_RRSR_CCK_ONLY_1M;
	write32(Adapter, REG_RRSR, value32);

	// CF-END Threshold
	//m_spIoBase->write8(REG_CFEND_TH, 0x1);

	// SIFS (used in NAV)
	value16 = _SPEC_SIFS_CCK(0x10) | _SPEC_SIFS_OFDM(0x10);
	write16(Adapter, REG_SPEC_SIFS, value16);

	// Retry Limit
	value16 = _LRL(0x30) | _SRL(0x30);
	write16(Adapter, REG_RL, value16);
	
}

VOID
_InitRateFallback(
	IN  PADAPTER Adapter
	)
{
	// Set Data Auto Rate Fallback Retry Count register.
	write32(Adapter, REG_DARFRC, 0x00000000);
	write32(Adapter, REG_DARFRC+4, 0x10080404);
	write32(Adapter, REG_RARFRC, 0x04030201);
	write32(Adapter, REG_RARFRC+4, 0x08070605);

}


VOID
_InitEDCA(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u16				value16;

#if 1
	//disable EDCCA count down, to reduce collison and retry
	value16 = read16(Adapter, REG_RD_CTRL);
	value16 |= DIS_EDCA_CNT_DWN;
	write16(Adapter, REG_RD_CTRL, value16);

	// Update SIFS timing.  ??????????
	pHalData->SifsTime = 0x0e0e0a0a;
	//Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_SIFS,  (pu1Byte)&pHalData->SifsTime);
	// SIFS for CCK Data ACK
	write8(Adapter, REG_SIFS_CTX, 0x0a);
	// SIFS for CCK consecutive tx like CTS data!
	write8(Adapter, REG_SIFS_CTX+1, 0x0a);
	
	// SIFS for OFDM Data ACK
	write8(Adapter, REG_SIFS_TRX, 0x0e);
	// SIFS for OFDM consecutive tx like CTS data!
	write8(Adapter, REG_SIFS_TRX+1, 0x0e);

	// Set CCK/OFDM SIFS
	write16(Adapter, REG_SIFS_CTX, 0x0a0a); // CCK SIFS shall always be 10us.
	write16(Adapter, REG_SIFS_TRX, 0x1010);
#endif

	write16(Adapter, REG_PROT_MODE_CTRL, 0x0204);

	write32(Adapter, REG_BAR_MODE_CTRL, 0x014004);


	// TXOP
	write32(Adapter, REG_EDCA_BE_PARAM, 0x005EA42B);
	write32(Adapter, REG_EDCA_BK_PARAM, 0x0000A44F);
	write32(Adapter, REG_EDCA_VI_PARAM, 0x005EA324);
	write32(Adapter, REG_EDCA_VO_PARAM, 0x002FA226);

	// PIFS
	write8(Adapter, REG_PIFS, 0x1C);
		
	//AGGR BREAK TIME Register
	write8(Adapter, REG_AGGR_BREAK_TIME, 0x16);

	write16(Adapter, REG_NAV_PROT_LEN, 0x0040);
	
	write8(Adapter, REG_BCNDMATIM, 0x02);

	write8(Adapter, REG_ATIMWND, 0x02);
	
}


VOID
_InitAMPDUAggregation(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);

	write32(Adapter, REG_AGGLEN_LMT, 0x99997631);

	if(IS_HARDWARE_TYPE_8192DU(pHalData))
	{
		if(pHalData->MacPhyMode92D !=SINGLEMAC_SINGLEPHY)
			write32(Adapter, REG_AGGLEN_LMT, 0x88888888);
		else
			write32(Adapter, REG_AGGLEN_LMT, 0xffffffff);
	}

	write8(Adapter, REG_AGGR_BREAK_TIME, 0x16);
}

VOID
_InitBeaconMaxError(
	IN  PADAPTER	Adapter,
	IN	BOOLEAN		InfraMode
	)
{
#ifdef RTL8192CU_ADHOC_WORKAROUND_SETTING
	write8(Adapter, REG_BCN_MAX_ERR,  0xFF );
#else
	//write8(Adapter, REG_BCN_MAX_ERR, (InfraMode ? 0xFF : 0x10));
#endif
}

VOID
_InitRDGSetting(
	IN	PADAPTER Adapter
	)
{
	write8(Adapter,REG_RD_CTRL,0xFF);
	write16(Adapter, REG_RD_NAV_NXT, 0x200);
	write8(Adapter,REG_RD_RESP_PKT_TH,0x05);
}

VOID
_InitRxSetting(
	IN	PADAPTER Adapter
	)
{
	write32(Adapter, REG_MACID, 0x87654321);
	write32(Adapter, 0x0700, 0x87654321);
}


VOID
_InitRetryFunction(
	IN  PADAPTER Adapter
	)
{
	u8	value8;
	
	value8 = read8(Adapter, REG_FWHW_TXQ_CTRL);
	value8 |= EN_AMPDU_RTY_NEW;
	write8(Adapter, REG_FWHW_TXQ_CTRL, value8);

	// Set ACK timeout
	write8(Adapter, REG_ACKTO, 0x40);
}


VOID
_InitUsbAggregationSetting(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

#if USB_TX_AGGREGATION_92C
{
	u32			value32;

	if(pHalData->UsbTxAggMode){
		value32 = read32(Adapter, REG_TDECTRL);
		value32 = value32 & ~(BLK_DESC_NUM_MASK << BLK_DESC_NUM_SHIFT);
		value32 |= ((pHalData->UsbTxAggDescNum & BLK_DESC_NUM_MASK) << BLK_DESC_NUM_SHIFT);
		
		write32(Adapter, REG_TDECTRL, value32);
	}
}
#endif

	// Rx aggregation setting
#if USB_RX_AGGREGATION_92C
{
	u8		valueDMA;
	u8		valueUSB;

	valueDMA = read8(Adapter, REG_TRXDMA_CTRL);
	valueUSB = read8(Adapter, REG_USB_SPECIAL_OPTION);

	switch(pHalData->UsbRxAggMode)
	{
		case USB_RX_AGG_DMA:
			valueDMA |= RXDMA_AGG_EN;
			valueUSB &= ~USB_AGG_EN;
			break;
		case USB_RX_AGG_USB:
			valueDMA &= ~RXDMA_AGG_EN;
			valueUSB |= USB_AGG_EN;
			break;
		case USB_RX_AGG_DMA_USB:
			valueDMA |= RXDMA_AGG_EN;
			valueUSB |= USB_AGG_EN;
			break;
		case USB_RX_AGG_DISABLE:
		default:
			valueDMA &= ~RXDMA_AGG_EN;
			valueUSB &= ~USB_AGG_EN;
			break;
	}

	write8(Adapter, REG_TRXDMA_CTRL, valueDMA);
	write8(Adapter, REG_USB_SPECIAL_OPTION, valueUSB);
#if  0 // Temply masked  zhiyuan 2010/1/4
	switch(pHalData->UsbRxAggMode)
	{
		case USB_RX_AGG_DMA:
			write8(Adapter, REG_RXDMA_AGG_PG_TH, pHalData->UsbRxAggPageCount);
			write8(Adapter, REG_USB_DMA_AGG_TO, pHalData->UsbRxAggPageTimeout);
			break;
		case USB_RX_AGG_USB:
			write8(Adapter, REG_USB_AGG_TH, pHalData->UsbRxAggBlockCount);
			write8(Adapter, REG_USB_AGG_TO, pHalData->UsbRxAggBlockTimeout);
			break;
		case USB_RX_AGG_DMA_USB:
			write8(Adapter, REG_RXDMA_AGG_PG_TH, pHalData->UsbRxAggPageCount);
			write8(Adapter, REG_USB_DMA_AGG_TO, pHalData->UsbRxAggPageTimeout);
			write8(Adapter, REG_USB_AGG_TH, pHalData->UsbRxAggBlockCount);
			write8(Adapter, REG_USB_AGG_TO, pHalData->UsbRxAggBlockTimeout);
			break;
		case USB_RX_AGG_DISABLE:
		default:
			// TODO: 
			break;
	}
#endif
	switch(PBP_128)
	{
		case PBP_128:
			pHalData->HwRxPageSize = 128;
			break;
		case PBP_64:
			pHalData->HwRxPageSize = 64;
			break;
		case PBP_256:
			pHalData->HwRxPageSize = 256;
			break;
		case PBP_512:
			pHalData->HwRxPageSize = 512;
			break;
		case PBP_1024:
			pHalData->HwRxPageSize = 1024;
			break;
		default:
			//RT_ASSERT(FALSE, ("RX_PAGE_SIZE_REG_VALUE definition is incorrect!\n"));
			break;
	}

}
#endif

}


VOID
_InitOperationMode(
	IN	PADAPTER			Adapter
	)
{
#if 1//gtest
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8				regBwOpMode = 0 , min_MPDU_spacing;
	u32				regRATR = 0, regRRSR = 0;


	//1 This part need to modified according to the rate set we filtered!!
	//
	// Set RRSR, RATR, and REG_BWOPMODE registers
	//
	switch(pHalData->CurrentWirelessMode)
	{
		case WIRELESS_MODE_B:
			regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK;
			regRRSR = RATE_ALL_CCK;
			break;
		case WIRELESS_MODE_A:
			//RT_ASSERT(FALSE,("Error wireless a mode\n"));
#if 1
			regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_OFDM_AG;
			regRRSR = RATE_ALL_OFDM_AG;
#endif
			break;
		case WIRELESS_MODE_G:
			regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			break;
		case WIRELESS_MODE_UNKNOWN:
		case WIRELESS_MODE_AUTO:
			//if (pHalData->bInHctTest)
			if (0)
			{
				regBwOpMode = BW_OPMODE_20MHZ;
				regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
				regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			}
			else
			{
				regBwOpMode = BW_OPMODE_20MHZ;
				regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
				regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			}
			break;
		case WIRELESS_MODE_N_24G:
			// It support CCK rate by default.
			// CCK rate will be filtered out only when associated AP does not support it.
			regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			break;
		case WIRELESS_MODE_N_5G:
			//RT_ASSERT(FALSE,("Error wireless mode"));
#if 1
			regBwOpMode = BW_OPMODE_5G;
			regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			regRRSR = RATE_ALL_OFDM_AG;
#endif
			break;
	}

	// Ziv ????????
	//PlatformEFIOWrite4Byte(Adapter, REG_INIRTS_RATE_SEL, regRRSR);
	write8(Adapter, REG_BWOPMODE, regBwOpMode);

	// For Min Spacing configuration.
	switch(pHalData->rf_type)
	{
		case RF_1T2R:
		case RF_1T1R:
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("Initializeadapter: RF_Type%s\n", (pHalData->RF_Type==RF_1T1R? "(1T1R)":"(1T2R)")));
			min_MPDU_spacing = (MAX_MSS_DENSITY_1T<<3);						
			break;
		case RF_2T2R:
		case RF_2T2R_GREEN:
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("Initializeadapter:RF_Type(2T2R)\n"));
			min_MPDU_spacing = (MAX_MSS_DENSITY_2T<<3);			
			break;
	}
	
	write8(Adapter, REG_AMPDU_MIN_SPACE, min_MPDU_spacing);
#endif
}


VOID
_InitSecuritySetting(
	IN  PADAPTER Adapter
	)
{
#if 0
	//Security related.
	//-----------------------------------------------------------------------------
	// Set up security related. 070106, by rcnjko:
	// 1. Clear all H/W keys.
	// 2. Enable H/W encryption/decryption.
	//-----------------------------------------------------------------------------
	if(Adapter->ResetProgress == RESET_TYPE_NORESET && Adapter->bInSetPower == FALSE)
	{
		SecClearAllKeys(Adapter);	
		CamResetAllEntry(Adapter);
		SecInit(Adapter);    
	}
#else

	u8 ucIndex;

	// 1. Clear all H/W keys.
	for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
		CAM_mark_invalid(Adapter, ucIndex);
	
	for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
		CAM_empty_entry(Adapter, ucIndex);

	//
	invalidate_cam_all(Adapter);


#endif	
}

VOID
_InitBeaconParameters(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(IS_HARDWARE_TYPE_8192DU(pHalData))	
	{
		 //default value  for register 0x558 and 0x559 is  0x05 0x03 (92DU before bitfile0821) zhiyuan 2009/08/26
		write16(Adapter, REG_TBTT_PROHIBIT,0x3c02);// ms
		write8(Adapter, REG_DRVERLYINT, 0x05);//ms
		write8(Adapter, REG_BCNDMATIM, 0x03);
	}
	else
	{
		// TODO: Remove these magic number
		write16(Adapter, REG_TBTT_PROHIBIT,0x6404);// ms
		write8(Adapter, REG_DRVERLYINT, DRIVER_EARLY_INT_TIME);//ms
		write8(Adapter, REG_BCNDMATIM, BCN_DMA_ATIME_INT_TIME);
	}
	// Suggested by designer timchen. Change beacon AIFS to the largest number
	// beacause test chip does not contension before sending beacon. by tynli. 2009.11.03
	if(IS_NORMAL_CHIP( pHalData->VersionID)||IS_HARDWARE_TYPE_8192DU(pHalData)){
		write16(Adapter, REG_BCNTCFG, 0x660F);
	}
	else{		
		write16(Adapter, REG_BCNTCFG, 0x66FF);
	}

}

VOID
_InitRFType(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8				value8;
#if (DISABLE_BB_RF==1)
	pHalData->rf_chip	= RF_PSEUDO_11N;//for RF verification
	pHalData->rf_type	=RF_1T1R;// RF_2T2R;	
#else

	pHalData->rf_chip =RF_6052;

	if(IS_HARDWARE_TYPE_8192D(pHalData))
	{
		if(pHalData->MacPhyMode92D==DUALMAC_DUALPHY)
		{
			pHalData->rf_type = RF_1T1R;	
		}
		else{// SMSP OR DMSP
			pHalData->rf_type = RF_2T2R;
		}
	}
	else{
		pHalData->rf_type = RF_1T1R;// RF_2T2R;	
	}

#endif
}

/*VOID
_InitRFType(
	IN	PADAPTER Adapter
	)
{
	struct registry_priv	 *pregpriv = &Adapter->registrypriv;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	BOOLEAN			is92CU		= IS_92C_SERIAL(pHalData->VersionID);

#if	DISABLE_BB_RF
	pHalData->rf_chip	= RF_PSEUDO_11N;
	return;
#endif

	pHalData->rf_chip	= RF_6052;

	if(pregpriv->rf_config != RF_819X_MAX_TYPE)
	{
		pHalData->rf_type = pregpriv->rf_config;
		DBG_8192C("Set RF Chip ID to RF_6052 and RF type to %d.\n", pHalData->rf_type);
		return;
	}	

	if(_FALSE == is92CU){
		pHalData->rf_type = RF_1T1R;
		DBG_8192C("Set RF Chip ID to RF_6052 and RF type to 1T1R.\n");
		return;
	}

	// TODO: Consider that EEPROM set 92CU to 1T1R later.
	// Force to overwrite setting according to chip version. Ignore EEPROM setting.
	//pHalData->RF_Type = is92CU ? RF_2T2R : RF_1T1R;
	//RT_TRACE(COMP_INIT,DBG_TRACE,("Set RF Chip ID to RF_6052 and RF type to %d.\n", pHalData->RF_Type));

}*/

VOID _InitAdhocWorkaroundParams(IN PADAPTER Adapter)
{
#if RTL8192CU_ADHOC_WORKAROUND_SETTING
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	pHalData->RegBcnCtrlVal = read8(Adapter, REG_BCN_CTRL);
	pHalData->RegTxPause = read8(Adapter, REG_TXPAUSE); 
#endif	
}

VOID
_BeaconFunctionEnable(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			Enable,
	IN	BOOLEAN			Linked
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			value8 = 0;
#if 0
	value8 = Enable ? (EN_BCN_FUNCTION | EN_TXBCN_RPT) : EN_BCN_FUNCTION;

	if(_FALSE == Linked){		
		if(IS_NORMAL_CHIP( pHalData->VersionID)){
			value8 |= DIS_TSF_UDT0_NORMAL_CHIP;
		}
		else{
			value8 |= DIS_TSF_UDT0_TEST_CHIP;
		}
	}

	write8(Adapter, REG_BCN_CTRL, value8);
#else
	write8(Adapter, REG_BCN_CTRL, 0x1a);
	write8(Adapter, REG_RD_CTRL+1, 0x6F);	
#endif
}


// Set CCK and OFDM Block "ON"
VOID _BBTurnOnBlock(
	IN	PADAPTER		Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
#if (DISABLE_BB_RF)
	return;
#endif

	if(!IS_HARDWARE_TYPE_8192DU(pHalData)){
		PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bCCKEn, 0x1);
		PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bOFDMEn, 0x1);
	}
}

VOID _RfPowerSave(
	IN	PADAPTER		Adapter
	)
{
#if 0
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	PMGNT_INFO		pMgntInfo	= &(Adapter->MgntInfo);
	u1Byte			eRFPath;

#if (DISABLE_BB_RF)
	return;
#endif

	if(pMgntInfo->RegRfOff == TRUE){ // User disable RF via registry.
		RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8192CUsb(): Turn off RF for RegRfOff.\n"));
		MgntActSet_RF_State(Adapter, eRfOff, RF_CHANGE_BY_SW);
		// Those action will be discard in MgntActSet_RF_State because off the same state
		for(eRFPath = 0; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
	}
	else if(pMgntInfo->RfOffReason > RF_CHANGE_BY_PS){ // H/W or S/W RF OFF before sleep.
		RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8192CUsb(): Turn off RF for RfOffReason(%ld).\n", pMgntInfo->RfOffReason));
		MgntActSet_RF_State(Adapter, eRfOff, pMgntInfo->RfOffReason);
	}
	else{
		pHalData->eRFPowerState = eRfOn;
		pMgntInfo->RfOffReason = 0; 
		if(Adapter->bInSetPower || Adapter->bResetInProgress)
			PlatformUsbEnableInPipes(Adapter);
		RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8192CUsb(): RF is on.\n"));
	}
#endif
}

#if ANTENNA_SELECTION_STATIC_SETTING
enum {
	Antenna_Lfet = 1,
	Antenna_Right = 2,	
};

VOID
_InitAntenna_Selection	(IN	PADAPTER Adapter)
{
	write8(Adapter, REG_LEDCFG2, 0x82);	
	PHY_SetBBReg(Adapter, rFPGA0_XA_RFInterfaceOE, bAntennaSelect, Antenna_Right);
}
#endif

BOOLEAN
_IsFWDownloaded(
	IN	PADAPTER			Adapter
	)
{
	return ((read32(Adapter, REG_MCUFWDL) & MCUFWDL_RDY) ? _TRUE : _FALSE);
}

VOID
_FWDownloadEnable(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			enable
	)
{
#if DEV_BUS_TYPE == USB_INTERFACE
	u32	value32 = read32(Adapter, REG_MCUFWDL);

	if(enable){
		value32 |= MCUFWDL_EN;
	}
	else{
		value32 &= ~MCUFWDL_EN;
	}

	write32(Adapter, REG_MCUFWDL, value32);

#else
	u8	tmp;

	if(enable)
	{
		// 8051 enable
		tmp = read8(Adapter, REG_SYS_FUNC_EN+1);
		write8(Adapter, REG_SYS_FUNC_EN+1, tmp|0x04);

		// MCU firmware download enable.
		tmp = read8(Adapter, REG_MCUFWDL);
		write8(Adapter, REG_MCUFWDL, tmp|0x01);

		// 8051 reset
		tmp = read8(Adapter, REG_MCUFWDL+2);
		write8(Adapter, REG_MCUFWDL+2, tmp&0xf7);
	}
	else
	{
		// MCU firmware download enable.
		tmp = read8(Adapter, REG_MCUFWDL);
		write8(Adapter, REG_MCUFWDL, tmp&0xfe);

		// Reserved for fw extension.   0x81[7] is used for mac0 status ,so don't write this reg here		 
		//write8(Adapter, REG_MCUFWDL+1, 0x00);
	}
#endif
}

#if (DEV_BUS_TYPE == USB_INTERFACE)
VOID
_BlockWrite_92d(
	IN		PADAPTER		Adapter,
	IN		PVOID			buffer,
	IN		u32				size
	)
{
	u32			blockSize8 = sizeof(u64);	
	u32			blocksize4 = sizeof(u32);
	u32			blockSize = 64;
	u8*			bufferPtr = (u8*)buffer;
	u32*		pu4BytePtr = (u32*)buffer;
	u32			i, offset, blockCount, remainSize,remain8,remain4,blockCount8,blockCount4;

	blockCount = size / blockSize;
	remain8 = size % blockSize;
	for(i = 0 ; i < blockCount ; i++){
		offset = i * blockSize;
		writeN(Adapter, (FW_8192C_START_ADDRESS + offset), 64,(bufferPtr + offset));
	}


	if(remain8){
		offset = blockCount * blockSize;

		blockCount8=remain8/blockSize8;
		remain4=remain8%blockSize8;
		//RT_TRACE(COMP_INIT,DBG_LOUD,("remain4 size %x blockcount %x blockCount8 %x\n",remain4,blockCount,blockCount8));		 
		for(i = 0 ; i < blockCount8 ; i++){	
			writeN(Adapter, (FW_8192C_START_ADDRESS + offset+i*blockSize8), 8,(bufferPtr + offset+i*blockSize8));
		}

		if(remain4){
			offset=blockCount * blockSize+blockCount8*blockSize8;		
			blockCount4=remain4/blocksize4;
			remainSize=remain8%blocksize4;
			 
			for(i = 0 ; i < blockCount4 ; i++){				
				write32(Adapter, (FW_8192C_START_ADDRESS + offset+i*blocksize4), *(pu4BytePtr+ offset/4+i));		
			}	
			
			if(remainSize){
				offset=blockCount * blockSize+blockCount8*blockSize8+blockCount4*blocksize4;
				for(i = 0 ; i < remainSize ; i++){
					write8(Adapter, (FW_8192C_START_ADDRESS + offset + i), *(bufferPtr +offset+ i));
				}	
			}
			
		}
		
	}
	
}
#endif

VOID
_BlockWrite(
	IN		PADAPTER		Adapter,
	IN		PVOID			buffer,
	IN		u32			size
	)
{
	u32			blockSize	= sizeof(u32);	// Use 4-byte write to download FW
	u8*			bufferPtr	= (u8*)buffer;
	u32*			pu4BytePtr	= (u32*)buffer;
	u32			i, offset, blockCount, remainSize;

	blockCount = size / blockSize;
	remainSize = size % blockSize;

	for(i = 0 ; i < blockCount ; i++){
		offset = i * blockSize;
		write32(Adapter, (FW_8192C_START_ADDRESS + offset), *(pu4BytePtr + i));
	}

	if(remainSize){
		offset = blockCount * blockSize;
		bufferPtr += offset;
		
		for(i = 0 ; i < remainSize ; i++){
			write8(Adapter, (FW_8192C_START_ADDRESS + offset + i), *(bufferPtr + i));
		}
	}

}

VOID
_PageWrite(
	IN		PADAPTER		Adapter,
	IN		u32			page,
	IN		PVOID			buffer,
	IN		u32			size
	)
{
	u8 value8;
	u8 u8Page = (u8) (page & 0x07) ;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	

	value8 = (read8(Adapter, REG_MCUFWDL+2)& 0xF8 ) | u8Page ;
	write8(Adapter, REG_MCUFWDL+2,value8);
#if (DEV_BUS_TYPE == USB_INTERFACE)	
	if(IS_HARDWARE_TYPE_8192DU(pHalData)){
		_BlockWrite_92d(Adapter,buffer,size);
	}
	else
#endif
	{
		_BlockWrite(Adapter,buffer,size);
	}
}

VOID
_WriteFW(
	IN		PADAPTER		Adapter,
	IN		PVOID			buffer,
	IN		u32			size
	)
{
	// Since we need dynamic decide method of dwonload fw, so we call this function to get chip version.
	// We can remove _ReadChipVersion from ReadAdapterInfo8192C later.

	BOOLEAN			isNormalChip;	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	
	isNormalChip = IS_NORMAL_CHIP(pHalData->VersionID);

	if(isNormalChip ||IS_HARDWARE_TYPE_8192D(pHalData)){
		u32 	pageNums,remainSize ;
		u32 	page,offset;
		u8*	bufferPtr = (u8*)buffer;

#if DEV_BUS_TYPE==PCI_INTERFACE
		// 20100120 Joseph: Add for 88CE normal chip. 
		// Fill in zero to make firmware image to dword alignment.
		_FillDummy(bufferPtr, &size);
#endif

		pageNums = size / MAX_PAGE_SIZE ;		
		//RT_ASSERT((pageNums <= 4), ("Page numbers should not greater then 4 \n"));			
		remainSize = size % MAX_PAGE_SIZE;		
		
		for(page = 0; page < pageNums;  page++){
			offset = page *MAX_PAGE_SIZE;
			_PageWrite(Adapter,page, (bufferPtr+offset),MAX_PAGE_SIZE);			
		}
		if(remainSize){
			offset = pageNums *MAX_PAGE_SIZE;
			page = pageNums;
			_PageWrite(Adapter,page, (bufferPtr+offset),remainSize);
		}	
		DBG_8192C("_WriteFW Done- for Normal chip.\n");
	}
	else	{
		_BlockWrite(Adapter,buffer,size);
		DBG_8192C("_WriteFW Done- for Test chip.\n");
	}
}

int _FWFreeToGo_92D(
	IN		PADAPTER		Adapter
	)
{
	u32			counter = 0;
	u32			value32;
	// polling CheckSum report
	do{
		value32 = read32(Adapter, REG_MCUFWDL);
	}while((counter ++ < POLLING_READY_TIMEOUT_COUNT) && (!(value32 & FWDL_ChkSum_rpt  )));	

	if(counter >= POLLING_READY_TIMEOUT_COUNT){	
		DBG_8192C("chksum report faill ! REG_MCUFWDL:0x%08x .\n",value32);
		return _FAIL;
	}
	DBG_8192C("Checksum report OK ! REG_MCUFWDL:0x%08x .\n",value32);

	value32 = read32(Adapter, REG_MCUFWDL);
	value32 |= MCUFWDL_RDY;
	write32(Adapter, REG_MCUFWDL, value32);
	return _SUCCESS;
	
}

int _FWFreeToGo(
	IN		PADAPTER		Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u32			counter = 0;
	u32			value32;
	
	// polling CheckSum report
	do{
		value32 = read32(Adapter, REG_MCUFWDL);
	}while((counter ++ < POLLING_READY_TIMEOUT_COUNT) && (!(value32 & FWDL_ChkSum_rpt)));	

	if(counter >= POLLING_READY_TIMEOUT_COUNT){	
		//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("chksum report faill ! REG_MCUFWDL:0x%08x .\n",value32));		
		return _FAIL;
	}
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Checksum report OK ! REG_MCUFWDL:0x%08x .\n",value32));


	value32 = read32(Adapter, REG_MCUFWDL);
	value32 |= MCUFWDL_RDY;
	value32 &= ~WINTINI_RDY;
	write32(Adapter, REG_MCUFWDL, value32);
	
	// polling for FW ready
	counter = 0;
	do
	{
			if(read32(Adapter, REG_MCUFWDL) & WINTINI_RDY){
				//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Polling FW ready success!! REG_MCUFWDL:0x%08x .\n",PlatformIORead4Byte(Adapter, REG_MCUFWDL)) );
				return _SUCCESS;
			}
			udelay_os(5);
	}while(counter++ < POLLING_READY_TIMEOUT_COUNT);

	//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Polling FW ready fail!! REG_MCUFWDL:0x%08x .\n",PlatformIORead4Byte(Adapter, REG_MCUFWDL)) );
	return _FAIL;
	
}


VOID
FirmwareSelfReset(
	IN	PADAPTER		Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	u1bTmp;
	u8	Delay = 100;
		
	if((pHalData->FirmwareVersion > 0x21) ||
		(pHalData->FirmwareVersion == 0x21 &&
		pHalData->FirmwareSubVersion >= 0x01))
	{
		//0x1cf=0x20. Inform 8051 to reset. 2009.12.25. tynli_test
		write8(Adapter, REG_HMETFR+3, 0x20);
	
		u1bTmp = read8(Adapter, REG_SYS_FUNC_EN+1);
		while(u1bTmp&BIT2)
		{
			Delay--;
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("PowerOffAdapter8192CE(): polling 0x03[2] Delay = %d \n", Delay));
			if(Delay == 0)
				break;
			udelay_os(50);
			u1bTmp = read8(Adapter, REG_SYS_FUNC_EN+1);
		}
	
		if((u1bTmp&BIT2) && (Delay == 0))
		{
			//DbgPrint("FirmwareDownload92C(): Fail!!!!!! 0x03 = %x\n", u1bTmp);
			//RT_ASSERT(FALSE, ("PowerOffAdapter8192CE(): 0x03 = %x\n", u1bTmp));
		}
	}
}

//
// description :polling fw ready
//
int _FWInit(
	IN PADAPTER			  Adapter     
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u32			value[2];
	u32			counter = 0;
	u32			value32;


	DBG_8192C("FW already have download ; \n");

	// polling for FW ready
	counter = 0;
	do
	{
		if(pHalData->interfaceIndex==0){
			if(read8(Adapter, FW_MAC0_ready) & mac0_ready){
				DBG_8192C("Polling FW ready success!! REG_MCUFWDL:0x%x .\n",read8(Adapter, FW_MAC0_ready));
				return _SUCCESS;
			}
			udelay_os(5);
		}
		else{
			if(read8(Adapter, FW_MAC1_ready) &mac1_ready){
				DBG_8192C("Polling FW ready success!! REG_MCUFWDL:0x%x .\n",read8(Adapter, FW_MAC1_ready));
				return _SUCCESS;
			}
			udelay_os(5);					
		}

	}while(counter++ < POLLING_READY_TIMEOUT_COUNT);

	if(pHalData->interfaceIndex==0){
		DBG_8192C("Polling FW ready fail!! MAC0 FW init not ready:0x%x .\n",read8(Adapter, FW_MAC0_ready) );
	}
	else{
		DBG_8192C("Polling FW ready fail!! MAC1 FW init not ready:0x%x .\n",read8(Adapter, FW_MAC1_ready) );
	}
	
	DBG_8192C("Polling FW ready fail!! REG_MCUFWDL:0x%x .\n",read32(Adapter, REG_MCUFWDL));
	return _FAIL;

}

//
//	Description:
//		Download 8192C firmware code.
//
//
int FirmwareDownload92C(
	IN	PADAPTER			Adapter
)
{	
	int	rtStatus = _SUCCESS;	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	char 			R88CFwImageFileName[] ={RTL8188C_FW_IMG};
	char 			R92CFwImageFileName[] ={RTL8192C_FW_IMG};
	char 			R92DFwImageFileName[] ={RTL8192D_FW_IMG};
	char				TestChipFwFile92D[] = {RTL8192D_FW_IMG};
	char				TestChipFwFile[] = {RTL8188C_FW_IMG};
	u8*				FwImage;
	u32				FwImageLen;
	char*			pFwImageFileName;
	u8*				pucMappedFile = NULL;
	//vivi, merge 92c and 92s into one driver, 20090817
	//vivi modify this temply, consider it later!!!!!!!!
	//PRT_FIRMWARE	pFirmware = GET_FIRMWARE_819X(Adapter);	
	//PRT_FIRMWARE_92C	pFirmware = GET_FIRMWARE_8192C(Adapter);
	PRT_FIRMWARE_92C	pFirmware = NULL;
	PRT_8192C_FIRMWARE_HDR		pFwHdr = NULL;
	u8		*pFirmwareBuf;
	u32		FirmwareLen;
	u8		value;
	u32		count;

	if(Adapter->bSurpriseRemoved){
		return _FAIL;
	}

	pFirmware = (PRT_FIRMWARE_92C)_malloc(sizeof(RT_FIRMWARE_92C));
	if(!pFirmware)
	{
		rtStatus = _FAIL;
		goto Exit;
	}

	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		if(IS_92C_SERIAL(pHalData->VersionID))
			pFwImageFileName = R92CFwImageFileName;
		else if(IS_HARDWARE_TYPE_8192D(pHalData))
			pFwImageFileName = R92DFwImageFileName;
		else
			pFwImageFileName = R88CFwImageFileName;

		FwImage = Rtl819XFwImageArray;
		FwImageLen = ImgArrayLength;
		DBG_8192C(" ===> FirmwareDownload91C() fw:Rtl819XFwImageArray\n");
	}
	else
	{
		if(IS_HARDWARE_TYPE_8192D(pHalData))
			pFwImageFileName =TestChipFwFile92D;
		else
			pFwImageFileName = TestChipFwFile;

		if(IS_HARDWARE_TYPE_8192D(pHalData))
		{
			FwImage = Rtl819XFwImageArray;
			FwImageLen = ImgArrayLength;
			DBG_8192C(" ===> FirmwareDownload91C() fw:Rtl8192CTestFwImg\n");
		}
		else
		{
			FwImage = Rtl819XFwImageArray;
			FwImageLen = ImgArrayLength;
			DBG_8192C(" ===> FirmwareDownload91C() fw:Rtl8192CTestFwImg\n");
		}
	}

	//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> FirmwareDownload91C() fw:%s\n", pFwImageFileName));

#ifdef CONFIG_EMBEDDED_FWIMG
	pFirmware->eFWSource = FW_SOURCE_HEADER_FILE;
#else
	pFirmware->eFWSource = FW_SOURCE_IMG_FILE; // We should decided by Reg.
#endif

	switch(pFirmware->eFWSource)
	{
		case FW_SOURCE_IMG_FILE:
			//TODO:
			break;
		case FW_SOURCE_HEADER_FILE:
			if(ImgArrayLength > FW_8192C_SIZE){
				rtStatus = _FAIL;
				//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Firmware size exceed 0x%X. Check it.\n", FW_8192C_SIZE) );
				goto Exit;
			}

			//RtlCopyMemory(pFirmware->szFwBuffer, Rtl819XFwImageArray, ImgArrayLength);
			_memcpy(pFirmware->szFwBuffer, Rtl819XFwImageArray, ImgArrayLength);
			pFirmware->ulFwLength = FwImageLen;
			break;
	}

	pFirmwareBuf = pFirmware->szFwBuffer;
	FirmwareLen = pFirmware->ulFwLength;

	// To Check Fw header. Added by tynli. 2009.12.04.
	pFwHdr = (PRT_8192C_FIRMWARE_HDR)pFirmware->szFwBuffer;

	pHalData->FirmwareVersion =  le16_to_cpu(pFwHdr->Version); 
	pHalData->FirmwareSubVersion = le16_to_cpu(pFwHdr->Subversion); 

	DBG_8192C(" FirmwareVersion(%#x), Signature(%#x)\n", pHalData->FirmwareVersion, pFwHdr->Signature);

	if(IS_FW_HEADER_EXIST(pFwHdr))
	{
		//RT_TRACE(COMP_INIT, DBG_LOUD,("Shift 32 bytes for FW header!!\n"));
		pFirmwareBuf = pFirmwareBuf + 32;
		FirmwareLen = FirmwareLen -32;
	}

	if(IS_HARDWARE_TYPE_8192D(pHalData)){
		// check if fw have already download ok,
		if(_IsFWDownloaded(Adapter))
			goto Exit;	
		 
		//check if fw in download process, avoid two mac download fw simutaneously
		count=0;
		while((read8(Adapter, 0x1f)&BIT5)&&(count <= 5000 )){
			count++;
			udelay_os(500); 
		}
		if(count==0){
			//no mac download fw ,to down it
			// set 0x1f[5]: fw in download process

			if(pHalData->interfaceIndex==0)	{
				value=read8(Adapter,REG_MAC1);
				value&=MAC1_ON;
			}
			else{
				value=read8(Adapter,REG_MAC0);
				value&=MAC0_ON;
			}

			if(value)
			{// if two mac are going to download fw ,set mac0 download ,mac1 wait.
				if(pHalData->interfaceIndex==0){
					DBG_8192C("MAC0 is going to download fw\n");
					value=read8(Adapter, 0x1f);
					value|=BIT5;
					write8(Adapter, 0x1f,value);
				}
				else{
					DBG_8192C("MAC1 is Waiting fw download\n");
					count=0;
					while((!_IsFWDownloaded(Adapter))&&(count<5000)){
						count++;
						udelay_os(500); 
					}
					goto Exit;	
				}		
			 	
			 }
			 else{
				DBG_8192C("set 0x1f[5] represent fw in download  process\n");
				value=read8(Adapter, 0x1f);
				value|=BIT5;
				write8(Adapter, 0x1f,value);
			}
		}
		else{
			//another mac download fw
			// check if fw download ok,
			if(_IsFWDownloaded(Adapter)){
				goto Exit;
			}
			else{
				// fw not download ok ,to download it,
				//set 0x1f[5]: fw in download process
				value=read8(Adapter, 0x1f);
				value|=BIT5;
				write8(Adapter, 0x1f,value);
			}
		}
	}

	// Suggested by Filen. If 8051 is running in RAM code, driver should inform Fw to reset by itself,
	// or it will cause download Fw fail. 2010.02.01. by tynli.
	if(read8(Adapter, REG_MCUFWDL)&BIT7) //8051 RAM code
	{	
		FirmwareSelfReset(Adapter);
		write8(Adapter, REG_MCUFWDL, 0x00);		
	}

	_FWDownloadEnable(Adapter, _TRUE);
	_WriteFW(Adapter, pFirmwareBuf, FirmwareLen);
	_FWDownloadEnable(Adapter, _FALSE);

	if(IS_HARDWARE_TYPE_8192D(pHalData)){
		rtStatus=_FWFreeToGo_92D(Adapter);
	}
	else{
		rtStatus = _FWFreeToGo(Adapter);		
	}

	if(_SUCCESS != rtStatus){
		//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("DL Firmware failed!\n") );	
		goto Exit;
	}	
	//RT_TRACE(COMP_INIT, DBG_LOUD, (" Firmware is ready to run!\n"));

Exit:
	if(IS_HARDWARE_TYPE_8192D(pHalData)){
		rtStatus =_FWInit(Adapter);
		
		// download fw over,clear 0x1f[5]
		value=read8(Adapter, 0x1f);
		value&=(~BIT5);
		write8(Adapter, 0x1f,value);
	}

	if(pFirmware)
		_mfree((u8*)pFirmware, sizeof(RT_FIRMWARE_92C));

	//RT_TRACE(COMP_INIT, DBG_LOUD, (" <=== FirmwareDownload91C()\n"));
	return rtStatus;

}

uint rtl8192c_hal_init(_adapter *padapter) 
{
	u8	val8, tmpU1b;
	u16	val16;
	u32	val32, RfRegValue, i=0, j;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	u8 isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	u8 is92C = IS_92C_SERIAL(pHalData->VersionID);
	struct registry_priv *pregistrypriv = &padapter->registrypriv;
	//struct eeprom_priv *peeprompriv = &padapter->eeprompriv;	 
	uint status = _SUCCESS;

	_InitQueueReservedPage(padapter);
	_InitTxBufferBoundary(padapter);		
	_InitQueuePriority(padapter);
	_InitPageBoundary(padapter);	
	_InitTransferPageSize(padapter);	

	// Get Rx PHY status in order to report RSSI and others.
	_InitDriverInfoSize(padapter, 4);

	_InitInterrupt(padapter);	
	_InitID(padapter);//set mac_address
	_InitNetworkType(padapter);//set msr	
	_InitWMACSetting(padapter);
	_InitAdaptiveCtrl(padapter);
	_InitEDCA(padapter);
	_InitRateFallback(padapter);
	_InitRetryFunction(padapter);
	_InitUsbAggregationSetting(padapter);
	_InitOperationMode(padapter);//todo
	_InitBeaconParameters(padapter);
	_InitAMPDUAggregation(padapter);
	_InitBeaconMaxError(padapter, _TRUE);
	_BeaconFunctionEnable(padapter, _FALSE, _FALSE);

#if ENABLE_USB_DROP_INCORRECT_OUT
	_InitHardwareDropIncorrectBulkOut(padapter);
#endif

	if(pHalData->bRDGEnable){
		_InitRDGSetting(padapter);
	}

	if(IS_HARDWARE_TYPE_8192DU(pHalData))	
	{
		// Set Data Auto Rate Fallback Reg.
		for(i = 0 ; i < 4 ; i++){
			write32(padapter, REG_ARFR0+i*4, 0x1f0ffff0);
		}

		if(pregistrypriv->wifi_spec)
		{
			write8(padapter, 0xfe5c, 0x08);
			write16(padapter, 0x460, 0);
		}
		else{
			write8(padapter, 0xfe5c, 0x07);
			if(pHalData->MacPhyMode92D==SINGLEMAC_SINGLEPHY){
				write16(padapter, 0x460, 0x8888);
			}
			else{
				write16(padapter, 0x460, 0x5555);
			}
		}

		write8(padapter, REG_TXPAUSE, 0);

		tmpU1b=read8(padapter, 0x605);
		tmpU1b|=0x30;
		write8(padapter, 0x605,tmpU1b);	
		write8(padapter, 0x55e,0x30);
		write8(padapter, 0x55f,0x30);
		write8(padapter, 0x606,0x30);
		//PlatformEFIOWrite1Byte(Adapter, 0x5e4,0x38);	  // masked for new bitfile
	
		//for bitfile 0912/0923 zhiyuan 2009/09/23
		// temp for high queue and mgnt Queue corrupt in time;
		//it may cause hang when sw beacon use high_Q,other frame use mgnt_Q; or ,sw beacon use mgnt_Q ,other frame use high_Q;
		write8(padapter, 0x523, 0x10);
		val16 = read16(padapter, 0x524);
		val16|=BIT12;
		write16(padapter,0x524 , val16);

		// suggested by zhouzhou   usb suspend  idle time count for bitfile0927  2009/10/09 zhiyuan
		val8=read8(padapter, 0xfe56);
		val8 |=(BIT0|BIT1);
		write8(padapter, 0xfe56, val8);

		if(pHalData->MacPhyMode92D ==SINGLEMAC_SINGLEPHY){
			DBG_8192C("InitializeAdapter8192CUsb  :  SUPER MAC 92D USB\n");
			write8(padapter,REG_AGGLEN_LMT+3,0xFF);
		}

		// led control for 92du
		write16(padapter, REG_LEDCFG0, 0x8282);
	}

#if ((1 == MP_DRIVER) ||  (0 == FW_PROCESS_VENDOR_CMD))   

	_InitRxSetting(padapter);
	DBG_8192C("InitializeAdapter8192CUsb(): Don't Download Firmware !!\n");
	padapter->bFWReady = _FALSE;
	pHalData->fw_ractrl = _FALSE;
#else
	status = FirmwareDownload92C(padapter);
	if(status == _FAIL)
	{
#if FW_PROCESS_VENDOR_CMD
		padapter->bFWReady = _FALSE;
#endif	
		pHalData->fw_ractrl = _FALSE;

		DBG_8192C("fw download fail!\n");

		//goto exit;
	}	
	else
	{
#if FW_PROCESS_VENDOR_CMD
		padapter->bFWReady = _TRUE;
#endif	
		pHalData->fw_ractrl = _TRUE;

		DBG_8192C("fw download ok!\n");	
	}
#endif


	//if(pMgntInfo->RegRfOff == TRUE){
	//	pHalData->eRFPowerState = eRfOff;
	//}


	// Set RF type for BB/RF configuration	
	_InitRFType(padapter);//->_ReadRFType()

	// Save target channel
	// <Roger_Notes> Current Channel will be updated again later.
	//pHalData->CurrentChannel = 44;//default set to 6

#if 1
	status = PHY_MACConfig8192C(padapter);
	if(status == _FAIL)
	{
		goto exit;
	}	
#endif

	//
	//d. Initialize BB related configurations.
	//
	status = PHY_BBConfig8192C(padapter);
	if(status == _FAIL)
	{
		goto exit;
	}

	// 92CU use 3-wire to r/w RF
	//pHalData->Rf_Mode = RF_OP_By_SW_3wire;

	if(IS_HARDWARE_TYPE_8192DU(pHalData))
	{
		// set before initialize RF, 
		PHY_SetBBReg(padapter, 0x88c, 0x00f00000,  0xf);
	}

	status = PHY_RFConfig8192C(padapter);
	if(status == _FAIL)
	{
		goto exit;
	}

	if(IS_HARDWARE_TYPE_8192DU(pHalData))
	{
		// set default value after initialize RF, 
		PHY_SetBBReg(padapter, 0x88c, 0x00f00000,  0);
	}
	
	_Update_BB_RF_FOR_92DU(padapter);


	_BBTurnOnBlock(padapter);
	_InitID(padapter);
	//NicIFSetMacAddress(padapter, padapter->PermanentAddress);

	if(pHalData->CurrentBandType92D == BAND_ON_5G)
		pHalData->CurrentWirelessMode = WIRELESS_11AN;
	else
		pHalData->CurrentWirelessMode = WIRELESS_11BGN;

	_InitSecuritySetting(padapter);
	_RfPowerSave(padapter);

#if ANTENNA_SELECTION_STATIC_SETTING
	if (!(IS_92C_SERIAL(pHalData->VersionID) || IS_92D_SINGLEPHY(pHalData->VersionID))){ //for 88CU ,1T1R
		_InitAntenna_Selection(padapter);
	}
#endif

	//
	// f. Start to BulkIn transfer.
	//


	//if(pregistrypriv->wifi_spec)
	//	write16(padapter,REG_FAST_EDCA_CTRL ,0);

#if (MP_DRIVER == 1)
	//MPT_InitializeAdapter(padapter, Channel);
#else // temply marked this for RF
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(!padapter->bSlaveOfDMSP)
#endif
	{
		if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
		PHY_IQCalibrate(padapter);
		//dm_CheckTXPowerTracking(padapter);
		PHY_LCCalibrate(padapter);

		//5G and 2.4G must wait sometime to let RF LO ready
		//by sherry 2010.06.28
		if(IS_HARDWARE_TYPE_8192DU(pHalData))
		{
			if(!IS_NORMAL_CHIP(pHalData->VersionID))
			{
				if(pHalData->rf_type == RF_1T1R)
				{
					for(j=0;j<3000;j++)
						udelay_os(MAX_STALL_TIME);
				}
				else
				{
					for(j=0;j<5000;j++)
						udelay_os(MAX_STALL_TIME);
				}
			}
		}
	}
#endif

#if RTL8192CU_ADHOC_WORKAROUND_SETTING
	_InitAdhocWorkaroundParams(padapter);
#endif

	InitHalDm(padapter);

	if(IS_HARDWARE_TYPE_8192DU(pHalData))	
	{	
		//pHalData->LoopbackMode=LOOPBACK_MODE;
		//if(padapter->ResetProgress == RESET_TYPE_NORESET)
		//{
		       u32					ulRegRead;
			//3//
			//3//Set Loopback mode or Normal mode
			//3//
			//2006.12.13 by emily. Note!We should not merge these two CPU_GEN register writings 
			//	because setting of System_Reset bit reset MAC to default transmission mode.
			ulRegRead = read32(padapter, 0x100);	//CPU_GEN  0x100
			//if(pHalData->LoopbackMode == RTL8192SU_NO_LOOPBACK)
			{
				ulRegRead |= ulRegRead ;
			}
			//else if (pHalData->LoopbackMode == RTL8192SU_MAC_LOOPBACK )
			//{
				//RT_TRACE(COMP_INIT, DBG_LOUD, ("==>start loop back mode %x\n",ulRegRead));
			//	ulRegRead |= 0x0b000000; //0x0b000000 CPU_CCK_LOOPBACK;
			//}
			//else if(pHalData->LoopbackMode == RTL8192SU_DMA_LOOPBACK)
			//{
				//RT_TRACE(COMP_INIT, DBG_LOUD, ("==>start dule mac loop back mode %x\n",ulRegRead));
			//	ulRegRead |= 0x07000000; //0x07000000 CPU_CCK_LOOPBACK;
			//}
			//else
			//{
				//RT_ASSERT(FALSE, ("Serious error: wrong loopback mode setting\n"));	
			//}

			write32(padapter, 0x100, ulRegRead);
	              //RT_TRACE(COMP_INIT,DBG_LOUD,("==>loop back mode CPU GEN value:%x\n",ulRegRead));

			// 2006.11.29. After reset cpu, we sholud wait for a second, otherwise, it may fail to write registers. Emily
			udelay_os(500);
		//}
	}
#if 0
{
	u16  temp;
	// temp for debug use
	DBG_8192C("==>begin to read register\n");
	for(temp=0;temp<0x800;temp+=4)
	{
		val32 = read32(padapter,temp);
		DBG_8192C("Addr: 0x%x, value = 0x%x \n",temp,val32);
		udelay_os(10);
		
	}
	for(temp=0xfe00;temp<0xfeff;temp+=4)
	{
		val32 = read32(padapter,temp);
		DBG_8192C("Addr: 0x%x, value = 0x%x \n",temp,val32);
		udelay_os(10);
		
	}
	DBG_8192C("<== end read register\n");
}
#endif

	//misc
	{
		int i;		
		u8 mac_addr[6];
		for(i=0; i<6; i++)
		{			
			mac_addr[i] = read8(padapter, REG_MACID+i);
		}
		
		DBG_8192C("MAC Address from REG = %x-%x-%x-%x-%x-%x\n", 
			mac_addr[0],	mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	}

exit:

	return status;

}

uint rtl8192c_hal_deinit(_adapter *padapter)
{
	RT_TRACE(_module_hal_init_c_,_drv_info_,("+rtl8192c_hal_deinit\n"));
	
	RT_TRACE(_module_hal_init_c_,_drv_info_,("-rtl8192c_hal_deinit, success!\n"));

	//NdisMDeregisterAdapterShutdownHandler(padapter->hndis_adapter);

	return _SUCCESS;
	
}

uint	 rtl871x_hal_init(_adapter *padapter) {

	u8 val8;	 
	uint status = _SUCCESS;
	
	padapter->hw_init_completed=_FALSE;
	if(padapter->halpriv.hal_bus_init ==NULL)
	{
		RT_TRACE(_module_hal_init_c_,_drv_err_,("\nInitialize halpriv.hal_bus_init error!!!\n"));
		status = _FAIL;
		goto exit;
	}
	else
	{
		val8=padapter->halpriv.hal_bus_init(padapter);
		if(val8==_FAIL)
		{
			RT_TRACE(_module_hal_init_c_,_drv_err_,("rtl871x_hal_init: hal_bus_init fail\n"));
			status= _FAIL;
			goto exit;
		}
	}


	status = rtl8192c_hal_init(padapter);
	if( status==_SUCCESS)
		padapter->hw_init_completed=_TRUE;
	else
	 	padapter->hw_init_completed=_FALSE;
exit:

	RT_TRACE(_module_hal_init_c_,_drv_err_,("-rtl871x_hal_init:status=0x%x\n",status));

	return status;

}	

uint	 rtl871x_hal_deinit(_adapter *padapter) {


	u8			val8;

	uint		res=_SUCCESS;
	
_func_enter_;

	if(padapter->halpriv.hal_bus_deinit ==NULL)
	{
		RT_TRACE(_module_hal_init_c_,_drv_err_,("\nInitialize halpriv.hal_bus_init error!!!\n"));
		res = _FAIL;
		goto exit;
	}
	else
	{
		val8=padapter->halpriv.hal_bus_deinit(padapter);

		if(val8 ==_FAIL)
		{
			RT_TRACE(_module_hal_init_c_,_drv_err_,("\n rtl871x_hal_init: hal_bus_init fail\n"));		
			res= _FAIL;
			goto exit;

		}
	}

	res = rtl8192c_hal_deinit(padapter);

	padapter->hw_init_completed=_FALSE;

exit:
	
_func_exit_;
	
	return res;
	
}

//-------------------------------------------------------------------------
//
//	Channel Plan
//
//-------------------------------------------------------------------------

RT_CHANNEL_DOMAIN
_HalMapChannelPlan8192C(
	IN	PADAPTER	Adapter,
	IN	u8		HalChannelPlan
	)
{
	RT_CHANNEL_DOMAIN	rtChannelDomain;

	switch(HalChannelPlan)
	{
		case EEPROM_CHANNEL_PLAN_GLOBAL_DOMAIN:
			rtChannelDomain = RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN;
			break;
		case EEPROM_CHANNEL_PLAN_WORLD_WIDE_13:
			rtChannelDomain = RT_CHANNEL_DOMAIN_WORLD_WIDE_13;
			break;			
		default:
			rtChannelDomain = (RT_CHANNEL_DOMAIN)HalChannelPlan;
			break;
	}
	
	return 	rtChannelDomain;

}

VOID
ReadChannelPlan(
	IN	PADAPTER 		Adapter,
	IN	u8*			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	)
{
	struct mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	u8			channelPlan;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(AutoLoadFail){
		channelPlan = CHPL_FCC;
	}
	else{
		channelPlan = PROMContent[EEPROM_CHANNEL_PLAN];
	}

	if((pregistrypriv->channel_plan>= RT_CHANNEL_DOMAIN_MAX) || (channelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK))
	{
		pmlmepriv->ChannelPlan = _HalMapChannelPlan8192C(Adapter, (channelPlan & (~(EEPROM_CHANNEL_PLAN_BY_HW_MASK))));
		//pMgntInfo->bChnlPlanFromHW = (channelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK) ? _TRUE : _FALSE; // User cannot change  channel plan.
	}
	else
	{
		pmlmepriv->ChannelPlan = (RT_CHANNEL_DOMAIN)pregistrypriv->channel_plan;
	}

#if 0 //todo:
	switch(pMgntInfo->ChannelPlan)
	{
		case RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN:
		{
			PRT_DOT11D_INFO	pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);
	
			pDot11dInfo->bEnabled = _TRUE;
		}
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("Enable dot11d when RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN!\n"));
		break;
	}
#endif

	if(IS_HARDWARE_TYPE_8192D(pHalData)){
		switch(pHalData->BandSet92D){
			case BAND_ON_2_4G:
				pmlmepriv->ChannelPlan = RT_CHANNEL_DOMAIN_IC;
				break;
			case BAND_ON_5G:
				pmlmepriv->ChannelPlan = RT_CHANNEL_DOMAIN_5G;
				break;
			case BAND_ON_BOTH:
				pmlmepriv->ChannelPlan = RT_CHANNEL_DOMAIN_FCC;
				break;
			default:
				break;
		}		
	}

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("RegChannelPlan(%d) EEPROMChannelPlan(%ld)", pMgntInfo->RegChannelPlan, (u4Byte)channelPlan));
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("ChannelPlan = %d\n" , pMgntInfo->ChannelPlan));

	MSG_8192C("RT_ChannelPlan: 0x%02x\n", pmlmepriv->ChannelPlan);

}


//-------------------------------------------------------------------------
//
//	EEPROM Power index mapping
//
//-------------------------------------------------------------------------

VOID
_ReadPowerValueFromPROM(
	IN	PTxPowerInfo	pwrInfo,
	IN	u8*			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	)
{
	u32 rfPath, eeAddr, group, offset1,offset2=0;

	_memset(pwrInfo, 0, sizeof(TxPowerInfo));

	if(AutoLoadFail){		
		for(group = 0 ; group < CHANNEL_GROUP_MAX ; group++){
			for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
#if(TX_POWER_FOR_5G_BAND == 1)				
				if(group< CHANNEL_GROUP_MAX_2G)
#endif
					pwrInfo->CCKIndex[rfPath][group]	= EEPROM_Default_TxPowerLevel;
				pwrInfo->HT40_1SIndex[rfPath][group]	= EEPROM_Default_TxPowerLevel;
				pwrInfo->HT40_2SIndexDiff[rfPath][group]= EEPROM_Default_HT40_2SDiff;
				pwrInfo->HT20IndexDiff[rfPath][group]	= EEPROM_Default_HT20_Diff;
				pwrInfo->OFDMIndexDiff[rfPath][group]	= EEPROM_Default_LegacyHTTxPowerDiff;
				pwrInfo->HT40MaxOffset[rfPath][group]	= EEPROM_Default_HT40_PwrMaxOffset;
				pwrInfo->HT20MaxOffset[rfPath][group]	= EEPROM_Default_HT20_PwrMaxOffset;
			}
		}

		//pwrInfo->TSSI_A = EEPROM_Default_TSSI;
		//pwrInfo->TSSI_B = EEPROM_Default_TSSI;
		pwrInfo->TSSI_A[0] = EEPROM_Default_TSSI;
		pwrInfo->TSSI_B[0] = EEPROM_Default_TSSI;
		pwrInfo->TSSI_A[1] = EEPROM_Default_TSSI;
		pwrInfo->TSSI_B[1] = EEPROM_Default_TSSI;
		
		return;
	}

	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(group = 0 ; group < CHANNEL_GROUP_MAX_2G; group++){
			eeAddr = EEPROM_CCK_TX_PWR_INX_2G + (rfPath * 3) + group;
			pwrInfo->CCKIndex[rfPath][group] = PROMContent[eeAddr];
		}
	}
	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(group = 0 ; group < CHANNEL_GROUP_MAX ; group++){
			offset1 = group / 3;
			offset2 = group % 3;
			eeAddr = EEPROM_HT40_1S_TX_PWR_INX_2G+ (rfPath * 3) + offset2 + offset1*21;
			pwrInfo->HT40_1SIndex[rfPath][group] = PROMContent[eeAddr];
		}
	}

	//These just for 92D efuse offset.
	for(group = 0 ; group < CHANNEL_GROUP_MAX ; group++){
		for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
			offset1 = group / 3;
			offset2 = group % 3;
			
			pwrInfo->HT40_2SIndexDiff[rfPath][group] = 
			(PROMContent[EEPROM_HT40_2S_TX_PWR_INX_DIFF_2G + offset2 + offset1*21] >> (rfPath * 4)) & 0xF;

			pwrInfo->HT20IndexDiff[rfPath][group] =
			(PROMContent[EEPROM_HT20_TX_PWR_INX_DIFF_2G + offset2 + offset1*21] >> (rfPath * 4)) & 0xF;
			
			pwrInfo->OFDMIndexDiff[rfPath][group] =
			(PROMContent[EEPROM_OFDM_TX_PWR_INX_DIFF_2G + offset2 + offset1*21] >> (rfPath * 4)) & 0xF;

			pwrInfo->HT40MaxOffset[rfPath][group] =
			(PROMContent[EEPROM_HT40_MAX_PWR_OFFSET_2G + offset2 + offset1*21] >> (rfPath * 4)) & 0xF;

			pwrInfo->HT20MaxOffset[rfPath][group] =
			(PROMContent[EEPROM_HT20_MAX_PWR_OFFSET_2G + offset2 + offset1*21] >> (rfPath * 4)) & 0xF;
		}
	}

	//pwrInfo->TSSI_A = PROMContent[EEPROM_TSSI_A];
	//pwrInfo->TSSI_B = PROMContent[EEPROM_TSSI_B];
	pwrInfo->TSSI_A[0] = PROMContent[EEPROM_TSSI_A_2G];
	pwrInfo->TSSI_B[0] = PROMContent[EEPROM_TSSI_B_2G];
	pwrInfo->TSSI_A[1] = PROMContent[EEPROM_TSSI_A_5G];
	pwrInfo->TSSI_B[1] = PROMContent[EEPROM_TSSI_B_5G];

}


u32
_GetChannelGroup(
	IN	u32	channel
	)
{
	//RT_ASSERT((channel < 14), ("Channel %d no is supported!\n"));

	if(channel < 3){ 	// Channel 1~3
		return 0;
	}
	else if(channel < 9){ // Channel 4~9
		return 1;
	}

	return 2;				// Channel 10~14	
}


VOID
ReadTxPowerInfo(
	IN	PADAPTER 		Adapter,
	IN	u8*			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	)
{	
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	TxPowerInfo		pwrInfo;
	u32			rfPath, ch, group;
	u8			pwr, diff, tempval;

	_ReadPowerValueFromPROM(&pwrInfo, PROMContent, AutoLoadFail);

	if(!AutoLoadFail)
	{
		pHalData->EEPROMRegulatory = (PROMContent[EEPROM_RF_OPT1]&0x7);	//bit0~2
	}
	else
	{
		pHalData->EEPROMRegulatory = 0;
	}

	//
	// ThermalMeter from EEPROM
	//
	if(!AutoLoadFail)
		tempval = PROMContent[EEPROM_THERMAL_METER];
	else
		tempval = EEPROM_Default_ThermalMeter;
	pEEPROM->EEPROMThermalMeter = (tempval&0x1f);	//[4:0]

	if(pEEPROM->EEPROMThermalMeter < 0x06 || pEEPROM->EEPROMThermalMeter > 0x1c)
		pEEPROM->EEPROMThermalMeter = 0x12;
	
	pHalData->ThermalMeter[0] = pEEPROM->EEPROMThermalMeter;	
	MSG_8192C("ThermalMeter = 0x%x\n", pEEPROM->EEPROMThermalMeter);
	
	//Always do tx power.
	pHalData->bTXPowerDataReadFromEEPORM = _TRUE;

	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
			group = getChnlGroupfromArray((u8)ch);

			if(ch < CHANNEL_MAX_NUMBER_2G)
				pEEPROM->TxPwrLevelCck[rfPath][ch]	= pwrInfo.CCKIndex[rfPath][group];
			pEEPROM->TxPwrLevelHT40_1S[rfPath][ch]= pwrInfo.HT40_1SIndex[rfPath][group];

			pEEPROM->TxPwrHt20Diff[rfPath][ch]		= pwrInfo.HT20IndexDiff[rfPath][group];
			pEEPROM->TxPwrLegacyHtDiff[rfPath][ch]	= pwrInfo.OFDMIndexDiff[rfPath][group];
			pEEPROM->PwrGroupHT20[rfPath][ch]		= pwrInfo.HT20MaxOffset[rfPath][group];
			pEEPROM->PwrGroupHT40[rfPath][ch]		= pwrInfo.HT40MaxOffset[rfPath][group];

			pwr	= pwrInfo.HT40_1SIndex[rfPath][group];
			diff	= pwrInfo.HT40_2SIndexDiff[rfPath][group];

			pEEPROM->TxPwrLevelHT40_2S[rfPath][ch]  = (pwr > diff) ? (pwr - diff) : 0;
		}
	}

#if DBG

	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
			if(ch < CHANNEL_MAX_NUMBER_2G)
			{
				RTPRINT(FINIT, INIT_TxPower, 
					("RF(%ld)-Ch(%ld) [CCK / HT40_1S / HT40_2S] = [0x%x / 0x%x / 0x%x]\n", 
					rfPath, ch, 
					pHalData->TxPwrLevelCck[rfPath][ch], 
					pHalData->TxPwrLevelHT40_1S[rfPath][ch], 
					pHalData->TxPwrLevelHT40_2S[rfPath][ch]));
			}
			else
			{
				RTPRINT(FINIT, INIT_TxPower, 
					("RF(%ld)-Ch(%ld) [HT40_1S / HT40_2S] = [0x%x / 0x%x]\n", 
					rfPath, ch, 
					pHalData->TxPwrLevelHT40_1S[rfPath][ch], 
					pHalData->TxPwrLevelHT40_2S[rfPath][ch]));				
			}
		}
	}

	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-A Ht20 to HT40 Diff[%d] = 0x%x\n", ch, pHalData->TxPwrHt20Diff[RF90_PATH_A][ch]));
	}

	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-A Legacy to Ht40 Diff[%d] = 0x%x\n", ch, pHalData->TxPwrLegacyHtDiff[RF90_PATH_A][ch]));
	}
	
	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-B Ht20 to HT40 Diff[%d] = 0x%x\n", ch, pHalData->TxPwrHt20Diff[RF90_PATH_B][ch]));
	}
	
	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-B Legacy to HT40 Diff[%d] = 0x%x\n", ch, pHalData->TxPwrLegacyHtDiff[RF90_PATH_B][ch]));
	}
	
#endif

}


//-------------------------------------------------------------------
//
//	EEPROM/EFUSE Content Parsing
//
//-------------------------------------------------------------------
VERSION_8192C
ReadChipVersion(
	IN	PADAPTER	Adapter
	)
{
	u32			value32;
	VERSION_8192C	version = VERSION_NORMAL_CHIP_88C;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	

	value32 = read32(Adapter, REG_SYS_CFG);

	//Decide TestChip or NormalChip here.
	//92D's RF_type will be decided when the reg0x2c is filled.
	if(IS_HARDWARE_TYPE_8192D(pHalData)){
		if (!(value32 & 0x000f0000)){ //Test or Normal Chip
			version = VERSION_TEST_CHIP_92D_SINGLEPHY;
			MSG_8192C("Chip Version ID: VERSION_TEST_CHIP_92D.\n");
		}
		else{
			version = VERSION_NORMAL_CHIP_92D_SINGLEPHY;
			MSG_8192C("Chip Version ID: VERSION_NORMAL_CHIP_92D.\n");
		}

		//Default set. The version will be decide after get the value of reg0x2c.
	}
	else //92C,88C
	{
		if (value32 & TRP_VAUX_EN){		
			version = (value32 & TYPE_ID) ?VERSION_TEST_CHIP_92C :VERSION_TEST_CHIP_88C;		
		}
		else{
			version = (value32 & TYPE_ID) ?VERSION_NORMAL_CHIP_92C :VERSION_NORMAL_CHIP_88C;
		}

#if DBG
		switch(version)
		{
			case VERSION_NORMAL_CHIP_92C:
				MSG_8192C("Chip Version ID: VERSION_NORMAL_CHIP_92C.\n");
				break;
			case VERSION_NORMAL_CHIP_88C:
				MSG_8192C ("Chip Version ID: VERSION_NORMAL_CHIP_88C.\n");
				break;
			case VERSION_TEST_CHIP_92C:
				MSG_8192C("Chip Version ID: VERSION_TEST_CHIP_92C.\n");
				break;
			case VERSION_TEST_CHIP_88C:
				MSG_8192C("Chip Version ID: VERSION_TEST_CHIP_88C.\n");
				break;
			default:
				MSG_8192C("Chip Version ID: ???????????????.\n");
				break;
		}
#endif
		pHalData->rf_type = (version & BIT0)?RF_2T2R:RF_1T1R;
	}

	return version;
}
VOID
_ReadChipVersion(
	IN PADAPTER			Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	pHalData->VersionID = ReadChipVersion(Adapter);
}

void
_ReadIDs(
	IN	PADAPTER	Adapter,
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	//PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);


	if(_FALSE == AutoloadFail){
		// VID, PID 
		pEEPROM->EEPROMVID = le16_to_cpu( *(u16 *)&PROMContent[EEPROM_VID]);
		pEEPROM->EEPROMPID = le16_to_cpu( *(u16 *)&PROMContent[EEPROM_PID]);
		
		// Customer ID, 0x00 and 0xff are reserved for Realtek. 		
		pEEPROM->EEPROMCustomerID = *(u8 *)&PROMContent[EEPROM_CUSTOMER_ID];
		pEEPROM->EEPROMSubCustomerID = *(u8 *)&PROMContent[EEPROM_SUBCUSTOMER_ID];

	}
	else{
		pEEPROM->EEPROMVID	 = EEPROM_Default_VID;
		pEEPROM->EEPROMPID	 = EEPROM_Default_PID;

		// Customer ID, 0x00 and 0xff are reserved for Realtek. 		
		pEEPROM->EEPROMCustomerID	= EEPROM_Default_CustomerID;
		pEEPROM->EEPROMSubCustomerID = EEPROM_Default_SubCustomerID;

	}


	switch(pEEPROM->EEPROMCustomerID)
	{
		case EEPROM_CID_ALPHA:
				pHalData->CustomerID = RT_CID_819x_ALPHA;
				break;
				
		case EEPROM_CID_CAMEO:
				pHalData->CustomerID = RT_CID_819x_CAMEO;
				break;			
					
		case EEPROM_CID_SITECOM:
				pHalData->CustomerID = RT_CID_819x_Sitecom;
				break;	
					
		case EEPROM_CID_COREGA:
				pHalData->CustomerID = RT_CID_COREGA;						
				break;			
			
		case EEPROM_CID_Senao:
				pHalData->CustomerID = RT_CID_819x_Senao;
				break;
		
		case EEPROM_CID_EDIMAX_BELKIN:
				pHalData->CustomerID = RT_CID_819x_Edimax_Belkin;
				break;
		
		case EEPROM_CID_SERCOMM_BELKIN:
				pHalData->CustomerID = RT_CID_819x_Sercomm_Belkin;
				break;
					
		case EEPROM_CID_WNC_COREGA:
				pHalData->CustomerID = RT_CID_819x_WNC_COREGA;
				break;
		
		case EEPROM_CID_WHQL:
/*			
			Adapter->bInHctTest = TRUE;

			pMgntInfo->bSupportTurboMode = FALSE;
			pMgntInfo->bAutoTurboBy8186 = FALSE;

			pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
			pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
			pMgntInfo->PowerSaveControl.bLeisurePs = FALSE;
				
			pMgntInfo->keepAliveLevel = 0;

			Adapter->bUnloadDriverwhenS3S4 = FALSE;
*/				
				break;
					
		case EEPROM_CID_NetCore:
				pHalData->CustomerID = RT_CID_819x_Netcore;
				break;
		
		case EEPROM_CID_CAMEO1:
				pHalData->CustomerID = RT_CID_819x_CAMEO1;
				break;
					
		case EEPROM_CID_CLEVO:
				pHalData->CustomerID = RT_CID_819x_CLEVO;
			break;			
		
		default:
				pHalData->CustomerID = RT_CID_DEFAULT;
			break;
			
	}

	MSG_8192C("EEPROMVID = 0x%04x\n", pEEPROM->EEPROMVID);
	MSG_8192C("EEPROMPID = 0x%04x\n", pEEPROM->EEPROMPID);
	MSG_8192C("EEPROMCustomerID : 0x%02x\n", pEEPROM->EEPROMCustomerID);
	MSG_8192C("EEPROMSubCustomerID: 0x%02x\n", pEEPROM->EEPROMSubCustomerID);

	MSG_8192C("RT_CustomerID: 0x%02x\n", pHalData->CustomerID);

}


VOID
_ReadMACAddress(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	u16	usValue=0;
	u8	i=0;

	// Dual MAC should assign diffrent MAC address ,or, it is wil cause hang in single phy mode  zhiyuan 04/07/2010
	if(!IS_HARDWARE_TYPE_8192D(pHalData)){
		if(_FALSE == AutoloadFail){
			//Read Permanent MAC address and set value to hardware
			_memcpy(pEEPROM->mac_addr, &PROMContent[EEPROM_MAC_ADDR], ETH_ALEN);		
		}
		else{
			//Random assigh MAC address
			u8 sMacAddr[MAC_ADDR_LEN] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
			//sMacAddr[5] = (u8)GetRandomNumber(1, 254);		
			_memcpy(pEEPROM->mac_addr, sMacAddr, ETH_ALEN);	
		}
	}
	else{
		//Temply random assigh mac address for  efuse mac address not ready now
		if(AutoloadFail == _FALSE  ){
			if(pHalData->interfaceIndex == 0){
				for(i = 0; i < 6; i += 2)
				{
					usValue = *(u16 *)&PROMContent[EEPROM_MAC_ADDR_MAC0_92D+i];
					*((u16 *)(&pEEPROM->mac_addr[i])) = usValue;
				}
			}
			else{
				for(i = 0; i < 6; i += 2)
				{
					usValue = *(u16 *)&PROMContent[EEPROM_MAC_ADDR_MAC1_92D+i];
					*((u16 *)(&pEEPROM->mac_addr[i])) = usValue;
				}
			}

			if(is_broadcast_mac_addr(pEEPROM->mac_addr) || is_multicast_mac_addr(pEEPROM->mac_addr))
			{
				//Random assigh MAC address
				u8 sMacAddr[MAC_ADDR_LEN] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
				//u32	curtime = get_current_time();
				if(pHalData->interfaceIndex == 1){
					sMacAddr[5] = 0x01;
					//sMacAddr[5] = (u8)(curtime & 0xff);
					//sMacAddr[5] = (u8)GetRandomNumber(1, 254);
				}
				_memcpy(pEEPROM->mac_addr, sMacAddr, ETH_ALEN);
			}
		}
		else
		{
			//Random assigh MAC address
			u8 sMacAddr[MAC_ADDR_LEN] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
			//u32	curtime = get_current_time();
			if(pHalData->interfaceIndex == 1){
				sMacAddr[5] = 0x01;
				//sMacAddr[5] = (u8)(curtime & 0xff);
				//sMacAddr[5] = (u8)GetRandomNumber(1, 254);
			}
			_memcpy(pEEPROM->mac_addr, sMacAddr, ETH_ALEN);
		}
	}
	
	//NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);
	//RT_PRINT_ADDR(COMP_INIT|COMP_EFUSE, DBG_LOUD, "MAC Addr: %s", Adapter->PermanentAddress);

}


VOID
_ReadMacPhyMode(
	IN	PADAPTER	Adapter,	
	IN	u8			*PROMContent,
	IN	BOOLEAN		AutoloadFail
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			MacPhyMode,Dual_Phymode,Dual_Macmode;
	BOOLEAN		isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	
	if(!IS_HARDWARE_TYPE_8192DU(pHalData))
	   return;
	
	if(AutoloadFail==_TRUE){
		Dual_Phymode=0;   // 0:single phy 1:dual phy
		Dual_Macmode=0;  //0:single mac 1:dual mac  
	}
	else{
		MacPhyMode=PROMContent[EEPROM_MAC_FUNCTION];
		Dual_Phymode=(MacPhyMode&BIT1)>>1;	// 0:single phy 1:dual phy
		Dual_Macmode=MacPhyMode&BIT0;	//0:single mac 1:dual mac 
	}
	//Dual_Phymode=0;   // 0:single phy 1:dual phy
	//Dual_Macmode=0;  //0:single mac 1:dual mac
printk("Dual_Phymode = %d, Dual_Macmode = %d\n",Dual_Phymode,Dual_Macmode);
	if(Dual_Phymode)
	{
		pHalData->MacPhyMode92D = DUALMAC_DUALPHY;
		pHalData->VersionID &=(~CHIP_92D_SINGLEPHY);
		pHalData->rf_type = RF_1T1R;
		if(pHalData->interfaceIndex==0){
			pHalData->CurrentBandType92D = BAND_ON_5G;  // defaule set ,mac0 5G MAC1 2.4G
			pHalData->BandSet92D = BAND_ON_BOTH;
			//pHalData->BandSet92D = BAND_ON_5G;
		}
		else{
			pHalData->CurrentBandType92D = BAND_ON_2_4G;  		
			pHalData->BandSet92D = BAND_ON_2_4G;
		}
	}
	else	
	{			
		if(Dual_Macmode)
		{// Dual mac Single Phy
			pHalData->MacPhyMode92D = DUALMAC_SINGLEPHY;
			pHalData->VersionID |= CHIP_92D_SINGLEPHY;
			pHalData->rf_type = RF_2T2R;
			pHalData->CurrentBandType92D = BAND_ON_2_4G;
			pHalData->BandSet92D = BAND_ON_2_4G;
		}
		else
		{// Single mac single phy
			pHalData->MacPhyMode92D = SINGLEMAC_SINGLEPHY;
			pHalData->VersionID |= CHIP_92D_SINGLEPHY;
			pHalData->rf_type = RF_2T2R;	
			pHalData->CurrentBandType92D = BAND_ON_2_4G;
			pHalData->BandSet92D = BAND_ON_BOTH;
		}
	}
      
}


VOID
_ReadBoardType(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{	
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	BOOLEAN			isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	u8			boardType;

	if(AutoloadFail){
		pHalData->rf_type = RF_2T2R;
		pHalData->BluetoothCoexist = _FALSE;
		return;
	}

#if HARDWARE_TYPE_IS_RTL8192D
	boardType = PROMContent[EEPROM_NORMAL_BoardType];
	boardType &= BOARD_TYPE_NORMAL_MASK;
	boardType >>= 5;
#else
	if(isNormal)
	{
		boardType = PROMContent[EEPROM_NORMAL_BoardType];
		boardType &= BOARD_TYPE_NORMAL_MASK;
		boardType >>= 5;
	}
	else
	{
		boardType = PROMContent[EEPROM_TEST_BoardType];
		boardType &= BOARD_TYPE_TEST_MASK;
	}
#endif

#if 0
	switch(boardType & 0xF)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			pHalData->rf_type = RF_2T2R;
			break;
		case 5:
			pHalData->rf_type = RF_1T2R;
			break;
		default:
			pHalData->rf_type = RF_1T1R;
			break;
	}

	pHalData->BluetoothCoexist = (boardType >> 4) ? _TRUE : _FALSE;
#endif
}


VOID
_ReadLEDSetting(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	struct led_priv *pledpriv = &(Adapter->ledpriv);
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	//
	// Led mode
	//
	switch(pHalData->CustomerID)
	{
		case RT_CID_DEFAULT:
		case RT_CID_819x_ALPHA:
		case RT_CID_819x_CAMEO:
					pledpriv->LedStrategy = SW_LED_MODE1;
					pledpriv->bRegUseLed = _TRUE;
					break;

		case RT_CID_819x_Sitecom:
					pledpriv->LedStrategy = SW_LED_MODE2;
					pledpriv->bRegUseLed = _TRUE;
					break;	

		case RT_CID_COREGA:
		case RT_CID_819x_Senao:
					pledpriv->LedStrategy = SW_LED_MODE3;
					pledpriv->bRegUseLed = _TRUE;
					break;			

		case RT_CID_819x_Edimax_Belkin:
					pledpriv->LedStrategy = SW_LED_MODE4;
					pledpriv->bRegUseLed = _TRUE;
					break;					

		case RT_CID_819x_Sercomm_Belkin:
					pledpriv->LedStrategy = SW_LED_MODE5;
					pledpriv->bRegUseLed = _TRUE;
					break;

		case RT_CID_819x_WNC_COREGA:
					pledpriv->LedStrategy = SW_LED_MODE6;
					pledpriv->bRegUseLed = _TRUE;
					break;

		default:
					pledpriv->LedStrategy = SW_LED_MODE0;
					pledpriv->bRegUseLed = _FALSE;
		break;			
					
	}

}

VOID
_ReadThermalMeter(
	IN	PADAPTER	Adapter,	
	IN	u8* 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);	
	u8			tempval;

	//
	// ThermalMeter from EEPROM
	//
	if(!AutoloadFail)	
		tempval = PROMContent[EEPROM_THERMAL_METER];
	else
		tempval = EEPROM_Default_ThermalMeter;
	
	pEEPROM->EEPROMThermalMeter = (tempval&0x1f);	//[4:0]

	if(pEEPROM->EEPROMThermalMeter < 0x06 || pEEPROM->EEPROMThermalMeter > 0x1c)
		pEEPROM->EEPROMThermalMeter = 0x12;
	
	pHalData->ThermalMeter[0] = pEEPROM->EEPROMThermalMeter;

	pHalData->ThermalValue = 0;//set to 0, will be update when do dm_txpower_tracking
	
	//RTPRINT(FINIT, INIT_TxPower, ("ThermalMeter = 0x%x\n", pHalData->EEPROMThermalMeter));
	
}

VOID
_ReadRFSetting(
	IN	PADAPTER	Adapter,	
	IN	u8* 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	)
{
}

void
_ReadPROMVersion(
	IN	PADAPTER	Adapter,	
	IN	u8* 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	)
{
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	//HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(AutoloadFail){
		pEEPROM->EEPROMVersion = EEPROM_Default_Version;		
	}
	else{
		pEEPROM->EEPROMVersion = *(u8 *)&PROMContent[EEPROM_VERSION];
	}
}

void _InitAdapterVariablesByPROM(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	unsigned char AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	_ReadPROMVersion(Adapter, PROMContent, AutoloadFail);
	_ReadIDs(Adapter, PROMContent, AutoloadFail);
	_ReadMACAddress(Adapter, PROMContent, AutoloadFail);
	if(IS_HARDWARE_TYPE_8192DU(pHalData)){
		// not read power info from EFUSE for efuse not ready now zhiyuan 2010/05/04
		ReadTxPowerInfo(Adapter, PROMContent, _TRUE);
	}
	else{
		ReadTxPowerInfo(Adapter, PROMContent, AutoloadFail);
	}
	_ReadMacPhyMode(Adapter, PROMContent, AutoloadFail);
	ReadChannelPlan(Adapter, PROMContent, AutoloadFail);
	_ReadBoardType(Adapter, PROMContent, AutoloadFail);
	_ReadThermalMeter(Adapter, PROMContent, AutoloadFail);
	_ReadLEDSetting(Adapter, PROMContent, AutoloadFail);	
	_ReadRFSetting(Adapter, PROMContent, AutoloadFail);
}

static void efuse_ReadAllMap(
	IN		PADAPTER	pAdapter, 
	IN OUT	u8		*Efuse)
{	
	int i, offset;	

	// We must enable clock and LDO 2.5V otherwise, read all map will be fail!!!!
	//
	
	//efuse_PowerSwitch(pAdapter, _TRUE);
	//ReadEFuse(pAdapter, 0, 128, Efuse);
	//efuse_PowerSwitch(pAdapter, _FALSE);

	efuse_reg_init(pAdapter);
	
	for(i=0, offset=0 ; i<128; i+=8, offset++)
	{
		efuse_pg_packet_read(pAdapter, offset, Efuse+i);			
	}
	
	efuse_reg_uninit(pAdapter);

}// efuse_ReadAllMap
void EFUSE_ShadowMapUpdate(
	IN		PADAPTER	pAdapter)
{	
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(pAdapter);	
		
	if (pEEPROM->bautoload_fail_flag == _TRUE)
	{			
		_memset(pEEPROM->efuse_eeprom_data, 0xff, 128);
	}
	else
	{
		efuse_ReadAllMap(pAdapter, pEEPROM->efuse_eeprom_data);
	}
	
	//PlatformMoveMemory((PVOID)&pHalData->EfuseMap[EFUSE_MODIFY_MAP][0], 
	//(PVOID)&pHalData->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE);
	
}// EFUSE_ShadowMapUpdate

void _ReadPROMContent(
	IN PADAPTER 		Adapter
	)
{	
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			PROMContent[HWSET_MAX_SIZE]={0};
	u8			eeValue;
	u32			i;
	u16			value16;

	eeValue = read8(Adapter, REG_9346CR);
	// To check system boot selection.
	pEEPROM->EepromOrEfuse		= (eeValue & BOOT_FROM_EEPROM) ? _TRUE : _FALSE;
	pEEPROM->bautoload_fail_flag	= (eeValue & EEPROM_EN) ? _FALSE : _TRUE;


	DBG_8192C("Boot from %s, Autoload %s !\n", (pEEPROM->EepromOrEfuse ? "EEPROM" : "EFUSE"),
				(pEEPROM->bautoload_fail_flag ? "Fail" : "OK") );

	//pHalData->EEType = IS_BOOT_FROM_EEPROM(Adapter) ? EEPROM_93C46 : EEPROM_BOOT_EFUSE;

	if (pEEPROM->bautoload_fail_flag == _FALSE)
	{
		if ( pEEPROM->EepromOrEfuse == _TRUE)
		{
			// Read all Content from EEPROM or EFUSE.
			for(i = 0; i < HWSET_MAX_SIZE; i += 2)
			{
				//todo:
				//value16 = EF2Byte(ReadEEprom(Adapter, (u16) (i>>1)));
				//*((u16*)(&PROMContent[i])) = value16; 				
			}
		}
		else
		{
			// Read EFUSE real map to shadow.
			EFUSE_ShadowMapUpdate(Adapter);
			_memcpy((void*)PROMContent, (void*)pEEPROM->efuse_eeprom_data, HWSET_MAX_SIZE);		
		}

		//Double check 0x8192 autoload status again
		if(RTL8192_EEPROM_ID != *((u16 *)PROMContent))
		{
			pEEPROM->bautoload_fail_flag = _TRUE;
			DBG_8192C("Autoload OK but EEPROM ID content is incorrect!!\n");
		}
		
	}
	else //auto load fail
	{
		_memset(pEEPROM->efuse_eeprom_data, 0xff, HWSET_MAX_SIZE);
		_memcpy((void*)PROMContent, (void*)pEEPROM->efuse_eeprom_data, HWSET_MAX_SIZE);		
	}

	
	_InitAdapterVariablesByPROM(Adapter, PROMContent, pEEPROM->bautoload_fail_flag);
	
}


VOID
_InitOtherVariable(
	IN PADAPTER 		Adapter
	)
{
#if 0
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	


	if(Adapter->bInHctTest){
		pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
		pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
		pMgntInfo->PowerSaveControl.bLeisurePs = FALSE;
		pMgntInfo->keepAliveLevel = 0;
	}

	// 2009/06/10 MH For 92S 1*1=1R/ 1*2&2*2 use 2R. We default set 1*1 use radio A
	// So if you want to use radio B. Please modify RF path enable bit for correct signal
	// strength calculate.
	if (pHalData->RF_Type == RF_1T1R){
		pHalData->bRFPathRxEnable[0] = TRUE;
	}
	else{
		pHalData->bRFPathRxEnable[0] = pHalData->bRFPathRxEnable[1] = TRUE;
	}

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("RegChannelPlan(%d) EEPROMChannelPlan(%d)", pMgntInfo->RegChannelPlan, pHalData->EEPROMChannelPlan));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("ChannelPlan = %d\n" , pMgntInfo->ChannelPlan));
#endif
}

VOID
_ReadRFType(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

#if DISABLE_BB_RF
	pHalData->rf_chip = RF_PSEUDO_11N;
#else
	pHalData->rf_chip = RF_6052;
#endif
}

int ReadAdapterInfo8192C(PADAPTER	Adapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	MSG_8192C("====> ReadAdapterInfo8192C\n");

	_ReadChipVersion(Adapter);
	_ReadRFType(Adapter);
	_ReadPROMContent(Adapter);

	_InitOtherVariable(Adapter);

	MSG_8192C("%s()(done), rf_chip=0x%x, rf_type=0x%x\n",  __FUNCTION__, pHalData->rf_chip, pHalData->rf_type);

	//For 92DU Phy and Mac mode set ,will initialize by EFUSE/EPPROM     zhiyuan 2010/03/25
	if(IS_HARDWARE_TYPE_8192D(pHalData))
	{
		if(pHalData->CurrentBandType92D == BAND_ON_5G)
			pHalData->CurrentChannel = 44;
		else
			pHalData->CurrentChannel = 6;
	}

	MSG_8192C("<==== ReadAdapterInfo8192C\n");

	return _SUCCESS;
}

u8 GetEEPROMSize8192C(PADAPTER Adapter)
{
	u8	size = 0;
	u32	curRCR;

	curRCR = read16(Adapter, REG_9346CR);
	size = (curRCR & BOOT_FROM_EEPROM) ? 6 : 4; // 6: EEPROM used is 93C46, 4: boot from E-Fuse.
	
	MSG_8192C("EEPROM type is %s\n", size==4 ? "E-FUSE" : "93C46");
	
	return size;
}

void NicIFReadAdapterInfo(PADAPTER Adapter)
{
	// Read EEPROM size before call any EEPROM function
	//Adapter->EepromAddressSize=Adapter->HalFunc.GetEEPROMSizeHandler(Adapter);
	Adapter->EepromAddressSize = GetEEPROMSize8192C(Adapter);
	
	ReadAdapterInfo8192C(Adapter);
}

void NicIFAssociateNIC(PADAPTER Adapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	pHalData->HardwareType = HARDWARE_TYPE_RTL8192DU;
}

