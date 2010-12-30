/******************************************************************************

     (c) Copyright 2008, RealTEK Technologies Inc. All Rights Reserved.

 Module:	hal8192sphy.c	

 Note:		Merge 92SE/SU PHY config as below
			1. BB register R/W API
 			2. RF register R/W API
 			3. Initial BB/RF/MAC config by reading BB/MAC/RF txt.
 			3. Power setting API
 			4. Channel switch API
 			5. Initial gain switch API.
 			6. Other BB/MAC/RF API.
 			
 Function:	PHY: Extern function, phy: local function
 		 
 Export:	PHY_FunctionName

 Abbrev:	NONE

 History:
	Data		Who		Remark	
	08/08/2008  MHC    	1. Port from 9x series phycfg.c
						2. Reorganize code arch and ad description.
						3. Collect similar function.
						4. Seperate extern/local API.
	08/12/2008	MHC		We must merge or move USB PHY relative function later.
	10/07/2008	MHC		Add IQ calibration for PHY.(Only 1T2R mode now!!!)
	11/06/2008	MHC		Add TX Power index PG file to config in 0xExx register
						area to map with EEPROM/EFUSE tx pwr index.
	
******************************************************************************/
#ifdef RTL8192SE

#include "r8192E.h"
#include "r8192S_phy.h"
#include "r8192S_phyreg.h"
#include "r8192S_rtl6052.h"
//#include "r8190_rtl8256.h" //just don't support 8256 for tmp
#include "r8192S_hwimg.h"
#include "r8192E_dm.h"

#ifdef ENABLE_DOT11D
#include "dot11d.h"
#endif

/*---------------------------Define Local Constant---------------------------*/
/* Channel switch:The size of command tables for switch channel*/
#define MAX_PRECMD_CNT 16
#define MAX_RFDEPENDCMD_CNT 16
#define MAX_POSTCMD_CNT 16

#define MAX_DOZE_WAITING_TIMES_9x 64

#define	PHY_STOP_SWITCH_CLKREQ			0
/*---------------------------Define Local Constant---------------------------*/

/*------------------------Define global variable-----------------------------*/

#define Rtl819XMAC_Array			Rtl8192SEMAC_2T_Array
#define Rtl819XAGCTAB_Array			Rtl8192SEAGCTAB_Array
#define Rtl819XPHY_REG_Array			Rtl8192SEPHY_REG_2T2RArray
#define Rtl819XPHY_REG_to1T1R_Array 		Rtl8192SEPHY_ChangeTo_1T1RArray
#define Rtl819XPHY_REG_to1T2R_Array		Rtl8192SEPHY_ChangeTo_1T2RArray
#define Rtl819XPHY_REG_to2T2R_Array		Rtl8192SEPHY_ChangeTo_2T2RArray
#define Rtl819XPHY_REG_Array_PG			Rtl8192SEPHY_REG_Array_PG
#define Rtl819XRadioA_Array			Rtl8192SERadioA_1T_Array
#define Rtl819XRadioB_Array			Rtl8192SERadioB_Array
#define Rtl819XRadioB_GM_Array					Rtl8192SERadioB_GM_Array
#define Rtl819XRadioA_to1T_Array		Rtl8192SERadioA_to1T_Array
#define Rtl819XRadioA_to2T_Array		Rtl8192SERadioA_to2T_Array

/*------------------------Define local variable------------------------------*/
// 2004-05-11
#if 0
static u32	RF_CHANNEL_TABLE_ZEBRA[]={
		0,
		0x085c,//2412 1	 
		0x08dc,//2417 2 
		0x095c,//2422 3 
		0x09dc,//2427 4 
		0x0a5c,//2432 5 
		0x0adc,//2437 6 
		0x0b5c,//2442 7
		0x0bdc,//2447 8
		0x0c5c,//2452 9
		0x0cdc,//2457 10
		0x0d5c,//2462 11
		0x0ddc,//2467 12
		0x0e5c,//2472 13
		//0x0f5c,//2484  
		0x0f72,//2484  //20040810
};
#endif

/*------------------------Define local variable------------------------------*/


/*--------------------Define export function prototype-----------------------*/
// Please refer to header file
/*--------------------Define export function prototype-----------------------*/


/*---------------------Define local function prototype-----------------------*/
/* RF serial read/write by firmware 3wire. */

static  u32 phy_FwRFSerialRead( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset); 
static	void phy_FwRFSerialWrite( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset, u32 Data);

/* RF serial read/write */
static u32 phy_RFSerialRead(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset);
static void phy_RFSerialWrite( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset, u32 Data);
static	u32 phy_CalculateBitShift(u32 BitMask);
// Initialize relative setting
static	bool	phy_BB8190_Config_HardCode(struct net_device* dev);
static	bool	phy_BB8192S_Config_ParaFile(struct net_device* dev);
/* MAC config */
//static	bool	phy_ConfigMACWithParaFile(struct net_device* dev, char *	pFileName);
	
static bool phy_ConfigMACWithHeaderFile(struct net_device* dev);
//static bool phy_ConfigBBWithParaFile(struct net_device* dev, char *	pFileName);
	
static bool phy_ConfigBBWithHeaderFile(struct net_device* dev,u8 ConfigType);

 //static bool phy_ConfigBBWithPgParaFile(struct net_device* dev, char *	pFileName);
static bool phy_ConfigBBWithPgHeaderFile(struct net_device* dev,u8 ConfigType);

//static bool phy_SetBBtoDiffRFWithParaFile(struct net_device* dev, char *	pFileName);
static bool phy_SetBBtoDiffRFWithHeaderFile(struct net_device* dev,u8 ConfigType);

/*Initialize Register definition*/
static void phy_InitBBRFRegisterDefinition(struct net_device* dev);
static bool phy_SetSwChnlCmdArray(	SwChnlCmd*		CmdTable,
										u32			CmdTableIdx,
										u32			CmdTableSz,
										SwChnlCmdID		CmdID,
										u32			Para1,
										u32			Para2,
										u32			msDelay	);

static bool phy_SwChnlStepByStep(
	struct net_device* dev,
	u8		channel,
	u8		*stage,
	u8		*step,
	u32		*delay
	);

static void phy_FinishSwChnlNow(struct net_device* dev,u8 channel);

static u8 phy_DbmToTxPwrIdx( struct net_device* dev, WIRELESS_MODE WirelessMode, long PowerInDbm);
static bool phy_SetRFPowerState8192SE(struct net_device* dev,RT_RF_POWER_STATE eRFPowerState);
static void phy_CheckEphySwitchReady( struct net_device* dev);

static long phy_TxPwrIdxToDbm( struct net_device* dev, WIRELESS_MODE   WirelessMode, u8 TxPwrIdx);
void rtl8192_SetFwCmdIOCallback(struct net_device* dev);

						
/*---------------------Define local function prototype-----------------------*/


/*----------------------------Function Body----------------------------------*/
//
// 1. BB register R/W API
//
/**
* Function:	PHY_QueryBBReg
*
* OverView:	Read "sepcific bits" from BB register
*
* Input:
*			PADAPTER		dev,
*			u32			RegAddr,		//The target address to be readback
*			u32			BitMask		//The target bit position in the target address
*										//to be readback	
* Output:	None
* Return:		u32			Data			//The readback register value
* Note:		This function is equal to "GetRegSetting" in PHY programming guide
*/
//use phy dm core 8225 8256 6052
//u32 PHY_QueryBBReg(struct net_device* dev,u32		RegAddr,	u32		BitMask)
u32 rtl8192_QueryBBReg(struct net_device* dev, u32 RegAddr, u32 BitMask)
{

  	u32	ReturnValue = 0, OriginalValue, BitShift;

#if (DISABLE_BB_RF == 1)
	return 0;
#endif

	RT_TRACE(COMP_RF, "--->PHY_QueryBBReg(): RegAddr(%#x), BitMask(%#x)\n", RegAddr, BitMask);

	//
	// <Roger_Notes> Due to 8051 operation cycle (limitation cycle: 6us) and 1-Byte access issue, we should use 
	// 4181 to access Base Band instead of 8051 on USB interface to make sure that access could be done in 
	// infinite cycle.
	// 2008.09.06.
	//
	OriginalValue = read_nic_dword(dev, RegAddr);

	BitShift = phy_CalculateBitShift(BitMask);
	ReturnValue = (OriginalValue & BitMask) >> BitShift;

	//RTPRINT(FPHY, PHY_BBR, ("BBR MASK=0x%x Addr[0x%x]=0x%x\n", BitMask, RegAddr, OriginalValue));
	RT_TRACE(COMP_RF, "<---PHY_QueryBBReg(): RegAddr(%#x), BitMask(%#x), OriginalValue(%#x)\n", RegAddr, BitMask, OriginalValue);
	return (ReturnValue);
}

/**
* Function:	PHY_SetBBReg
*
* OverView:	Write "Specific bits" to BB register (page 8~) 
*
* Input:
*			PADAPTER		dev,
*			u32			RegAddr,		//The target address to be modified
*			u32			BitMask		//The target bit position in the target address
*										//to be modified	
*			u32			Data			//The new register value in the target bit position
*										//of the target address			
*
* Output:	None
* Return:		None
* Note:		This function is equal to "PutRegSetting" in PHY programming guide
*/
//use phy dm core 8225 8256
//void PHY_SetBBReg(struct net_device* dev,u32		RegAddr,	u32		BitMask,	u32		Data	)
void rtl8192_setBBreg(struct net_device* dev, u32 RegAddr, u32 BitMask, u32 Data)
{
	u32	OriginalValue, BitShift, NewValue;

#if (DISABLE_BB_RF == 1)
	return;
#endif


	//
	// <Roger_Notes> Due to 8051 operation cycle (limitation cycle: 6us) and 1-Byte access issue, we should use 
	// 4181 to access Base Band instead of 8051 on USB interface to make sure that access could be done in 
	// infinite cycle.
	// 2008.09.06.
	//
	{
		if(BitMask!= bMaskDWord)
		{//if not "double word" write
			OriginalValue = read_nic_dword(dev, RegAddr);
			BitShift = phy_CalculateBitShift(BitMask);
			NewValue = (((OriginalValue) & (~BitMask)) | (Data << BitShift));
			write_nic_dword(dev, RegAddr, NewValue);	
		}
		else
        {            
			write_nic_dword(dev, RegAddr, Data);			
        }			
	}
	//RTPRINT(FPHY, PHY_BBW, ("BBW MASK=0x%x Addr[0x%x]=0x%x\n", BitMask, RegAddr, Data));
//	RT_TRACE(COMP_RF, "<---PHY_SetBBReg(): RegAddr(%#x), BitMask(%#x), Data(%#x)\n", RegAddr, BitMask, Data);

	return;
}


//
// 2. RF register R/W API
//
/**
* Function:	PHY_QueryRFReg
*
* OverView:	Query "Specific bits" to RF register (page 8~) 
*
* Input:
*			PADAPTER		dev,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u32			RegAddr,		//The target address to be read
*			u32			BitMask		//The target bit position in the target address
*										//to be read	
*
* Output:	None
* Return:		u32			Readback value
* Note:		This function is equal to "GetRFRegSetting" in PHY programming guide
*/
//in dm 8256 and phy
//u32 PHY_QueryRFReg(struct net_device* dev,	RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask)
u32 rtl8192_phy_QueryRFReg(struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask)
{
	u32 Original_Value, Readback_Value, BitShift;//, flags;	
	struct r8192_priv *priv = ieee80211_priv(dev);
        unsigned long flags;
	
#if (DISABLE_BB_RF == 1) 
	return 0;
#endif
	
	RT_TRACE(COMP_RF, "--->PHY_QueryRFReg(): RegAddr(%#x), eRFPath(%#x), BitMask(%#x)\n", RegAddr, eRFPath,BitMask);
	
	if (!((priv->rf_pathmap >> eRFPath) & 0x1))
		return 0;
	
	if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))
		return	0;
	
	/* 2008/01/17 MH We get and release spin lock when reading RF register. */
	//PlatformAcquireSpinLock(dev, RT_RF_OPERATE_SPINLOCK);FIXLZM
	spin_lock_irqsave(&priv->rf_lock, flags);	
	//
	// <Roger_Notes> Due to 8051 operation cycle (limitation cycle: 6us) and 1-Byte access issue, we should use 
	// 4181 to access Base Band instead of 8051 on USB interface to make sure that access could be done in 
	// infinite cycle.
	// 2008.09.06.
	//
	if (priv->Rf_Mode == RF_OP_By_FW)
	{	
		Original_Value = phy_FwRFSerialRead(dev, eRFPath, RegAddr);
	}
	else
	{	
		Original_Value = phy_RFSerialRead(dev, eRFPath, RegAddr);	   	
	}
	
	BitShift =  phy_CalculateBitShift(BitMask);
	Readback_Value = (Original_Value & BitMask) >> BitShift;	
	spin_unlock_irqrestore(&priv->rf_lock, flags);
	//PlatformReleaseSpinLock(dev, RT_RF_OPERATE_SPINLOCK);

	//RTPRINT(FPHY, PHY_RFR, ("RFR-%d MASK=0x%x Addr[0x%x]=0x%x\n", eRFPath, BitMask, RegAddr, Original_Value));
	
	return (Readback_Value);
}


/**
* Function:	PHY_SetRFReg
*
* OverView:	Write "Specific bits" to RF register (page 8~) 
*
* Input:
*			PADAPTER		dev,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u32			RegAddr,		//The target address to be modified
*			u32			BitMask		//The target bit position in the target address
*										//to be modified	
*			u32			Data			//The new register Data in the target bit position
*										//of the target address			
*
* Output:	None
* Return:		None
* Note:		This function is equal to "PutRFRegSetting" in PHY programming guide
*/
//use phy  8225 8256
void rtl8192_phy_SetRFReg(struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask, u32 Data)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	u32 Original_Value, BitShift, New_Value;//, flags;
        unsigned long flags;
#if (DISABLE_BB_RF == 1)
	return;
#endif
	
	RT_TRACE(COMP_RF, "--->PHY_SetRFReg(): RegAddr(%#x), BitMask(%#x), Data(%#x), eRFPath(%#x)\n", 
		RegAddr, BitMask, Data, eRFPath);

	if (!((priv->rf_pathmap >> eRFPath) & 0x1))
		return ;
	if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))
	{
		return;
	}
	
	/* 2008/01/17 MH We get and release spin lock when writing RF register. */
	spin_lock_irqsave(&priv->rf_lock, flags);	
	//
	// <Roger_Notes> Due to 8051 operation cycle (limitation cycle: 6us) and 1-Byte access issue, we should use 
	// 4181 to access Base Band instead of 8051 on USB interface to make sure that access could be done in 
	// infinite cycle.
	// 2008.09.06.
	//
	if (priv->Rf_Mode == RF_OP_By_FW)	
	{		
		//DbgPrint("eRFPath-%d Addr[%02x] = %08x\n", eRFPath, RegAddr, Data);
		if (BitMask != bRFRegOffsetMask) // RF data is 12 bits only
		{
			Original_Value = phy_FwRFSerialRead(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			New_Value = (((Original_Value) & (~BitMask)) | (Data<< BitShift));
		
			phy_FwRFSerialWrite(dev, eRFPath, RegAddr, New_Value);
		}
		else
			phy_FwRFSerialWrite(dev, eRFPath, RegAddr, Data);		
	}
	else
	{		
		//DbgPrint("eRFPath-%d Addr[%02x] = %08x\n", eRFPath, RegAddr, Data);
		if (BitMask != bRFRegOffsetMask) // RF data is 12 bits only
		{
			Original_Value = phy_RFSerialRead(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			New_Value = (((Original_Value) & (~BitMask)) | (Data<< BitShift));
		
			phy_RFSerialWrite(dev, eRFPath, RegAddr, New_Value);
		}
		else
			phy_RFSerialWrite(dev, eRFPath, RegAddr, Data);
	
	}
	spin_unlock_irqrestore(&priv->rf_lock, flags);
	RT_TRACE(COMP_RF, "<---PHY_SetRFReg(): RegAddr(%#x), BitMask(%#x), Data(%#x), eRFPath(%#x)\n", 
			RegAddr, BitMask, Data, eRFPath);
	
}

/*-----------------------------------------------------------------------------
 * Function:	phy_FwRFSerialRead()
 *
 * Overview:	We support firmware to execute RF-R/W.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/21/2008	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
//use in phy only
static	u32
phy_FwRFSerialRead(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset	)
{
	u32		retValue = 0;		
#if 0	
	u32		Data = 0;
	u8		time = 0;
	//DbgPrint("FW RF CTRL\n\r");
	/* 2007/11/02 MH Firmware RF Write control. By Francis' suggestion, we can 
	   not execute the scheme in the initial step. Otherwise, RF-R/W will waste
	   much time. This is only for site survey. */
	// 1. Read operation need not insert data. bit 0-11	
	//Data &= bMask12Bits;
	// 2. Write RF register address. Bit 12-19
	Data |= ((Offset&0xFF)<<12);
	// 3. Write RF path.  bit 20-21
	Data |= ((eRFPath&0x3)<<20);
	// 4. Set RF read indicator. bit 22=0
	//Data |= 0x00000;
	// 5. Trigger Fw to operate the command. bit 31
	Data |= 0x80000000;		
	// 6. We can not execute read operation if bit 31 is 1.
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		// If FW can not finish RF-R/W for more than ?? times. We must reset FW.
		if (time++ < 100)
		{
			//DbgPrint("FW not finish RF-R Time=%d\n\r", time);
			delay_us(10);
		}
		else
			break;
	}
	// 7. Execute read operation.		
	PlatformIOWrite4Byte(dev, QPNR, Data);		
	// 8. Check if firmawre send back RF content.
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		// If FW can not finish RF-R/W for more than ?? times. We must reset FW.
		if (time++ < 100)
		{
			//DbgPrint("FW not finish RF-W Time=%d\n\r", time);
			delay_us(10);
		}
		else
			return	(0);
	}
	retValue = PlatformIORead4Byte(dev, RF_DATA);
#endif	
	return	(retValue);

}	/* phy_FwRFSerialRead */

/*-----------------------------------------------------------------------------
 * Function:	phy_FwRFSerialWrite()
 *
 * Overview:	We support firmware to execute RF-R/W.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/21/2008	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
//use in phy only
static	void
phy_FwRFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data	)
{
#if 0	
	u8	time = 0;
	DbgPrint("N FW RF CTRL RF-%d OF%02x DATA=%03x\n\r", eRFPath, Offset, Data);
	/* 2007/11/02 MH Firmware RF Write control. By Francis' suggestion, we can 
	   not execute the scheme in the initial step. Otherwise, RF-R/W will waste
	   much time. This is only for site survey. */
	
	// 1. Set driver write bit and 12 bit data. bit 0-11	
	//Data &= bMask12Bits;	// Done by uper layer.
	// 2. Write RF register address. bit 12-19
	Data |= ((Offset&0xFF)<<12);
	// 3. Write RF path.  bit 20-21
	Data |= ((eRFPath&0x3)<<20);
	// 4. Set RF write indicator. bit 22=1
	Data |= 0x400000;
	// 5. Trigger Fw to operate the command. bit 31=1
	Data |= 0x80000000;
	
	// 6. Write operation. We can not write if bit 31 is 1.
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		// If FW can not finish RF-R/W for more than ?? times. We must reset FW.
		if (time++ < 100)
		{
			//DbgPrint("FW not finish RF-W Time=%d\n\r", time);
			delay_us(10);
		}
		else
			break;
	}
	// 7. No matter check bit. We always force the write. Because FW will 
	//    not accept the command.		
	PlatformIOWrite4Byte(dev, QPNR, Data);
	/* 2007/11/02 MH Acoording to test, we must delay 20us to wait firmware
	   to finish RF write operation. */
	/* 2008/01/17 MH We support delay in firmware side now. */
	//delay_us(20);
#endif		
}	/* phy_FwRFSerialWrite */

#if (RTL92SE_FPGA_VERIFY == 1)
/**
* Function:	phy_RFSerialRead
*
* OverView:	Read regster from RF chips 
*
* Input:
*			PADAPTER		dev,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u32			Offset,		//The target address to be read			
*
* Output:	None
* Return:		u32			reback value
* Note:		Threre are three types of serial operations: 
*			1. Software serial write
*			2. Hardware LSSI-Low Speed Serial Interface 
*			3. Hardware HSSI-High speed
*			serial write. Driver need to implement (1) and (2).
*			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
*/
//use in phy only
static u32 phy_RFSerialRead(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset)
{

	u32						retValue = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32						NewOffset;

	//
	// Make sure RF register offset is correct 
	//
	Offset &= 0x3f;

	//
	// Switch page for 8256 RF IC
	//
	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||	
		priv->rf_chip == RF_6052)
	{
		//analog to digital off, for protection
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0x0);// 0x88c[11:8]

		if(Offset>=31)
		{
			priv->RFReadPageCnt[2]++;//cosa add for debug
			priv->RfReg0Value[eRFPath] |= 0x140;

			// Switch to Reg_Mode2 for Reg31~45
			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			// Modified Offset
			NewOffset = Offset - 30;
			
		}else if(Offset>=16)
		{
			priv->RFReadPageCnt[1]++;//cosa add for debug
			priv->RfReg0Value[eRFPath] |= 0x100;
			priv->RfReg0Value[eRFPath] &= (~0x40);

			// Switch to Reg_Mode1 for Reg16~30
			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			// Modified Offset
			NewOffset = Offset - 15;
		}
		else
		{
			priv->RFReadPageCnt[0]++;//cosa add for debug
			NewOffset = Offset;
	}	
	}	
	else
		NewOffset = Offset;

#if (RTL92SE_FPGA_VERIFY == 1)
	// For 92SE debug
	{
		u32	temp1, temp2;

		//cswu suggest 20080515
		// Note for 8196 & 8712 or latter project,
		// reg82c[31], reg834[31], reg83c[31] don't work.
		// only reg824[31] works
		temp1 = rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, 0xffffffff);
		temp2 = rtl8192_QueryBBReg(dev, pPhyReg->rfHSSIPara2, 0xffffffff);
		temp2 = temp2 & (~bLSSIReadAddress) | (NewOffset<<24) | bLSSIReadEdge;

		rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, 0xffffffff, temp1&(~bLSSIReadEdge));
		msleep(1);
		rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, 0xffffffff, temp2);
		msleep(1);
		rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, 0xffffffff, temp1|bLSSIReadEdge);
		msleep(1);
		
	}
#else
	//
	// Put desired read address to LSSI control register
	//
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, bLSSIReadAddress, NewOffset);

	//
	// Issue a posedge trigger
	//
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x0);	
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x1);	
#endif
	
	// TODO: we should not delay such a  long time. Ask help from SD3
	mdelay(1);

	retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
	
	// Switch back to Reg_Mode0;
	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_6052)
	{
		if (Offset >= 0x10)
		{
			priv->RfReg0Value[eRFPath] &= 0xebf;
		
			rtl8192_setBBreg(
				dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);
		}

		//analog to digital on
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0xf);// 0x88c[11:8]
	}
	
	return retValue;	
}



/**
* Function:	phy_RFSerialWrite
*
* OverView:	Write data to RF register (page 8~) 
*
* Input:
*			PADAPTER		dev,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u32			Offset,		//The target address to be read			
*			u32			Data			//The new register Data in the target bit position
*										//of the target to be read			
*
* Output:	None
* Return:		None
* Note:		Threre are three types of serial operations: 
*			1. Software serial write
*			2. Hardware LSSI-Low Speed Serial Interface 
*			3. Hardware HSSI-High speed
*			serial write. Driver need to implement (1) and (2).
*			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
 *
 * Note: 		  For RF8256 only
 *			 The total count of RTL8256(Zebra4) register is around 36 bit it only employs 
 *			 4-bit RF address. RTL8256 uses "register mode control bit" (Reg00[12], Reg00[10]) 
 *			 to access register address bigger than 0xf. See "Appendix-4 in PHY Configuration
 *			 programming guide" for more details. 
 *			 Thus, we define a sub-finction for RTL8526 register address conversion
 *		       ===========================================================
 *			 Register Mode		RegCTL[1]		RegCTL[0]		Note
 *								(Reg00[12])		(Reg00[10])
 *		       ===========================================================
 *			 Reg_Mode0				0				x			Reg 0 ~15(0x0 ~ 0xf)
 *		       ------------------------------------------------------------------
 *			 Reg_Mode1				1				0			Reg 16 ~30(0x1 ~ 0xf)
 *		       ------------------------------------------------------------------
 *			 Reg_Mode2				1				1			Reg 31 ~ 45(0x1 ~ 0xf)
 *		       ------------------------------------------------------------------
*/
static void
phy_RFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data
	)
{
	u32					DataAndAddr = 0;
	struct r8192_priv 			*priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32					NewOffset;

	Offset &= 0x3f;

	//
	// Shadow Update
	//
	PHY_RFShadowWrite(dev, eRFPath, Offset, Data);	

	//
	// Switch page for 8256 RF IC
	//
	if( 	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_6052)
	{
		//analog to digital off, for protection
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0x0);// 0x88c[11:8]
				
		if(Offset>=31)
		{
			priv->RFWritePageCnt[2]++;//cosa add for debug
			priv->RfReg0Value[eRFPath] |= 0x140;
			
			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 30;
			
		}else if(Offset>=16)
		{
			priv->RFWritePageCnt[1]++;//cosa add for debug
			priv->RfReg0Value[eRFPath] |= 0x100;
			priv->RfReg0Value[eRFPath] &= (~0x40);			
							

			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 15;
		}
		else
		{
			priv->RFWritePageCnt[0]++;//cosa add for debug
			NewOffset = Offset;
	}
	}
	else
		NewOffset = Offset;

	//
	// Put write addr in [5:0]  and write data in [31:16]
	//
	DataAndAddr = (Data<<16) | (NewOffset&0x3f);

	//
	// Write Operation
	//
	rtl8192_setBBreg(dev, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);


	if(Offset==0x0)
		priv->RfReg0Value[eRFPath] = Data;

	// Switch back to Reg_Mode0;
 	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_6052)
	{
		if (Offset >= 0x10)
		{
			if(Offset != 0)
			{
				priv->RfReg0Value[eRFPath] &= 0xebf;
				rtl8192_setBBreg(
				dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);
			}
		}	
		//analog to digital on
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0xf);// 0x88c[11:8]	
	}
	
}
#else
/**
* Function:	phy_RFSerialRead
*
* OverView:	Read regster from RF chips 
*
* Input:
*			PADAPTER		dev,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u32			Offset,		//The target address to be read			
*
* Output:	None
* Return:		u32			reback value
* Note:		Threre are three types of serial operations: 
*			1. Software serial write
*			2. Hardware LSSI-Low Speed Serial Interface 
*			3. Hardware HSSI-High speed
*			serial write. Driver need to implement (1) and (2).
*			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
*/
//use in phy only
static	u32
phy_RFSerialRead(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset
	)
{

	u32					retValue = 0;
	struct r8192_priv 			*priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32					NewOffset;
	u32 					tmplong,tmplong2;
	u8					RfPiEnable=0;
#if 0
	if(priv->rf_chip == RF_8225 && Offset > 0x24) //36 valid regs
		return	retValue;
	if(priv->rf_chip == RF_8256 && Offset > 0x2D) //45 valid regs
		return	retValue;
#endif
	//
	// Make sure RF register offset is correct 
	//
	Offset &= 0x3f;

	//
	// Switch page for 8256 RF IC
	//
	NewOffset = Offset;

	// For 92S LSSI Read RFLSSIRead
	// For RF A/B write 0x824/82c(does not work in the future) 
	// We must use 0x824 for RF A and B to execute read trigger
	tmplong = rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord);
	tmplong2 = rtl8192_QueryBBReg(dev, pPhyReg->rfHSSIPara2, bMaskDWord);
	tmplong2 = (tmplong2 & (~bLSSIReadAddress)) | (NewOffset<<23) | bLSSIReadEdge;	//T65 RF

	rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong&(~bLSSIReadEdge));
	udelay(1000);
	
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, bMaskDWord, tmplong2);
	udelay(1000);
	
	rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong|bLSSIReadEdge);

	if(eRFPath == RF90_PATH_A)
		RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter1, BIT8);
	else if(eRFPath == RF90_PATH_B)
		RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XB_HSSIParameter1, BIT8);
	
	if(RfPiEnable)
	{	// Read from BBreg8b8, 12 bits for 8190, 20bits for T65 RF
		retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBackPi, bLSSIReadBackData);
		//printk("Readback from RF-PI : 0x%x\n", retValue);
	}
	else
	{	//Read from BBreg8a0, 12 bits for 8190, 20 bits for T65 RF
		retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
		//printk("Readback from RF-SI : 0x%x\n", retValue);
	}
	
	retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
	
	return retValue;	
		
}

/**
* Function:	phy_RFSerialWrite
*
* OverView:	Write data to RF register (page 8~) 
*
* Input:
*			PADAPTER		dev,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u32			Offset,		//The target address to be read			
*			u32			Data			//The new register Data in the target bit position
*										//of the target to be read			
*
* Output:	None
* Return:		None
* Note:		Threre are three types of serial operations: 
*			1. Software serial write
*			2. Hardware LSSI-Low Speed Serial Interface 
*			3. Hardware HSSI-High speed
*			serial write. Driver need to implement (1) and (2).
*			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
 *
 * Note: 		  For RF8256 only
 *			 The total count of RTL8256(Zebra4) register is around 36 bit it only employs 
 *			 4-bit RF address. RTL8256 uses "register mode control bit" (Reg00[12], Reg00[10]) 
 *			 to access register address bigger than 0xf. See "Appendix-4 in PHY Configuration
 *			 programming guide" for more details. 
 *			 Thus, we define a sub-finction for RTL8526 register address conversion
 *		       ===========================================================
 *			 Register Mode		RegCTL[1]		RegCTL[0]		Note
 *								(Reg00[12])		(Reg00[10])
 *		       ===========================================================
 *			 Reg_Mode0				0				x			Reg 0 ~15(0x0 ~ 0xf)
 *		       ------------------------------------------------------------------
 *			 Reg_Mode1				1				0			Reg 16 ~30(0x1 ~ 0xf)
 *		       ------------------------------------------------------------------
 *			 Reg_Mode2				1				1			Reg 31 ~ 45(0x1 ~ 0xf)
 *		       ------------------------------------------------------------------
 *
 *	2008/09/02	MH	Add 92S RF definition
 *	
 *
 *
 */
 //use in phy only
static	void
phy_RFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data
	)
{
	u32					DataAndAddr = 0;
	struct r8192_priv 			*priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32					NewOffset;
	
#if 0
	//<Roger_TODO> We should check valid regs for RF_0222D case.
	if(priv->rf_chip == RF_8225 && Offset > 0x24) //36 valid regs
		return;
	if(priv->rf_chip == RF_8256 && Offset > 0x2D) //45 valid regs
		return;
#endif

	Offset &= 0x3f;

	// Shadow Update
	PHY_RFShadowWrite(dev, eRFPath, Offset, Data);	

	//
	// Switch page for 8256 RF IC
	//
		NewOffset = Offset;

	//
	// Put write addr in [5:0]  and write data in [31:16]
	//
	//DataAndAddr = (Data<<16) | (NewOffset&0x3f);
	DataAndAddr = ((NewOffset<<20) | (Data&0x000fffff)) & 0x0fffffff;	// T65 RF

	//
	// Write Operation
	//
	rtl8192_setBBreg(dev, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);
	//RTPRINT(FPHY, PHY_RFW, ("RFW-%d Addr[0x%x]=0x%x\n", eRFPath, pPhyReg->rf3wireOffset, DataAndAddr));
	
}

#endif

/**
* Function:	phy_CalculateBitShift
*
* OverView:	Get shifted position of the BitMask
*
* Input:
*			u32		BitMask,	
*
* Output:	none
* Return:		u32		Return the shift bit bit position of the mask
*/
//use in phy only
static u32 phy_CalculateBitShift(u32 BitMask)
{
	u32 i;

	for(i=0; i<=31; i++)
	{
		if ( ((BitMask>>i) &  0x1 ) == 1)
			break;
	}

	return (i);
}


//
// 3. Initial MAC/BB/RF config by reading MAC/BB/RF txt.
//
/*-----------------------------------------------------------------------------
 * Function:    PHY_MACConfig8192S
 *
 * Overview:	Condig MAC by header file or parameter file.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 *  When		Who		Remark
 *  08/12/2008	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
//adapter_start
extern bool PHY_MACConfig8192S(struct net_device* dev)
{
	bool rtStatus = true;

	//
	// Config MAC
	//
#if RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigMACWithHeaderFile(dev);
#else
	
	// Not make sure EEPROM, add later
	RT_TRACE(COMP_INIT, "Read MACREG.txt\n");
	//rtStatus = phy_ConfigMACWithParaFile(dev, RTL819X_PHY_MACREG);// lzm del it temp
#endif
	return (rtStatus == true) ? 1:0;

}

//adapter_start
extern	bool
PHY_BBConfig8192S(struct net_device* dev)
{
	bool rtStatus = true;
	u8 PathMap = 0, index = 0, rf_num = 0;
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u8 bRegHwParaFile = 1;
	
	phy_InitBBRFRegisterDefinition(dev);

	//
	// Config BB and AGC
	//
	//bRegHwParaFile = dev->MgntInfo.bRegHwParaFile;
	
	//kassert();
	   printk("bRegHwParaFile=%d\n", bRegHwParaFile);
	msleep(1000);
	
	switch(bRegHwParaFile)
	{
	case 0:
		phy_BB8190_Config_HardCode(dev);
		break;

	case 1:	    	    
		rtStatus = phy_BB8192S_Config_ParaFile(dev);
		break;

	case 2:
		// Partial Modify. 
		phy_BB8190_Config_HardCode(dev);		
		phy_BB8192S_Config_ParaFile(dev);
		break;

	default:	    
		phy_BB8190_Config_HardCode(dev);
		break;
	}
	//kassert();

	// Check BB/RF confiuration setting.
	// We only need to configure RF which is turned on.
	PathMap = (u8)(rtl8192_QueryBBReg(dev, rFPGA0_TxInfo, 0xf) |
				rtl8192_QueryBBReg(dev, rOFDM0_TRxPathEnable, 0xf));
	priv->rf_pathmap = PathMap;
	for(index = 0; index<4; index++)
	{
		if((PathMap>>index)&0x1)
			rf_num++;
	}

	if((priv->rf_type==RF_1T1R && rf_num!=1) ||
		(priv->rf_type==RF_1T2R && rf_num!=2) ||
		(priv->rf_type==RF_2T2R && rf_num!=2) ||
		(priv->rf_type==RF_2T2R_GREEN && rf_num!=2) ||
		(priv->rf_type==RF_2T4R && rf_num!=4))
	{
		RT_TRACE( COMP_INIT, "PHY_BBConfig8192S: RF_Type(%x) does not match RF_Num(%x)!!\n", priv->rf_type, rf_num);
	}
	return rtStatus;
}

//adapter_start
extern	bool
PHY_RFConfig8192S(struct net_device* dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	bool    		rtStatus = true;

	//Set priv->rf_chip = RF_8225 to do real PHY FPGA initilization

	//<Roger_EXP> We assign RF type here temporally. 2008.09.12.
	if (IS_HARDWARE_TYPE_8192SE(dev))
		priv->rf_chip = RF_6052;

	//
	// RF config
	//
	switch(priv->rf_chip)
	{
	case RF_8225:
	case RF_6052:
		rtStatus = PHY_RF6052_Config(dev);
		break;
		
	case RF_8256:			
//		rtStatus = PHY_RF8256_Config(dev); //disable now. WB 12.27.2008 
		break;
		
	case RF_8258:
		break;
		
	case RF_PSEUDO_11N:
		//rtStatus = PHY_RF8225_Config(dev);
		break;
        default:
            break;
	}

	return rtStatus;
}


// Joseph test: new initialize order!!
// Test only!! This part need to be re-organized.
// Now it is just for 8256.
//use in phy only
static bool  
phy_BB8190_Config_HardCode(struct net_device* dev)
{
	//RT_ASSERT(FALSE, ("This function is not implement yet!! \n"));
	return true;
}


//use in phy only
static bool 	
phy_BB8192S_Config_ParaFile(struct net_device* dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	bool rtStatus = true;	
	//u8				u2RegValue;
	//u16				u4RegValue;
	//char				szBBRegFile[] = RTL819X_PHY_REG;
	//char				szBBRegFile1T2R[] = RTL819X_PHY_REG_1T2R;
	//char				szBBRegPgFile[] = RTL819X_PHY_REG_PG;
	//char				szAGCTableFile[] = RTL819X_AGC_TAB;
	//char				szBBRegto1T1RFile[] = RTL819X_PHY_REG_to1T1R;
	//char				szBBRegto1T2RFile[] = RTL819X_PHY_REG_to1T2R;
	
	RT_TRACE(COMP_INIT, "==>phy_BB8192S_Config_ParaFile\n");

	//
	// 1. Read PHY_REG.TXT BB INIT!!
	// We will seperate as 1T1R/1T2R/1T2R_GREEN/2T2R
	//
#if	RTL8190_Download_Firmware_From_Header
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_2T2R ||
	    priv->rf_type == RF_1T1R ||priv->rf_type == RF_2T2R_GREEN)
	{
	    //kassert();
	    
		rtStatus = phy_ConfigBBWithHeaderFile(dev,BaseBand_Config_PHY_REG);  // fail at this stage
		
        //kassert();
		if(priv->rf_type != RF_2T2R && priv->rf_type != RF_2T2R_GREEN)
		{//2008.11.10 Added by tynli. The default PHY_REG.txt we read is for 2T2R,
		  //so we should reconfig BB reg with the right PHY parameters.
		  
/* 20090730 - kevin wang removed */
			rtStatus = phy_SetBBtoDiffRFWithHeaderFile(dev,BaseBand_Config_PHY_REG);

		}
		//kassert();
	}else
		rtStatus = false;
#else
	RT_TRACE(COMP_INIT, "RF_Type == %d\n", priv->rf_type);		
	// No matter what kind of RF we always read PHY_REG.txt. We must copy different 
	// type of parameter files to phy_reg.txt at first.	
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_2T2R ||
	    priv->rf_type == RF_1T1R ||priv->rf_type == RF_2T2R_GREEN)
	{
		rtStatus = phy_ConfigBBWithParaFile(dev, (char* )&szBBRegFile);
		if(priv->rf_type != RF_2T2R && priv->rf_type != RF_2T2R_GREEN)
		{//2008.11.10 Added by tynli. The default PHY_REG.txt we read is for 2T2R,
		  //so we should reconfig BB reg with the right PHY parameters.
			if(priv->rf_type == RF_1T1R)
				rtStatus = phy_SetBBtoDiffRFWithParaFile(dev, (char* )&szBBRegto1T1RFile);
			else if(priv->rf_type == RF_1T2R)
				rtStatus = phy_SetBBtoDiffRFWithParaFile(dev, (char* )&szBBRegto1T2RFile);
		}

	}else
		rtStatus = false;
#endif

    //kassert();
    
	if(rtStatus != true){
		RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile():Write BB Reg Fail!!");
		goto phy_BB8190_Config_ParaFile_Fail;
	}

	//
	// 2. If EEPROM or EFUSE autoload OK, We must config by PHY_REG_PG.txt
	//
	if (priv->AutoloadFailFlag == false)
	{
		priv->pwrGroupCnt = 0;

#if	RTL8190_Download_Firmware_From_Header
		rtStatus = phy_ConfigBBWithPgHeaderFile(dev,BaseBand_Config_PHY_REG);
#else
		rtStatus = phy_ConfigBBWithPgParaFile(dev, (char* )&szBBRegPgFile);
#endif
	}
	if(rtStatus != true){
		RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile():BB_PG Reg Fail!!");
		goto phy_BB8190_Config_ParaFile_Fail;
	}
	
	//
	// 3. BB AGC table Initialization
	//
#if RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigBBWithHeaderFile(dev,BaseBand_Config_AGC_TAB);
#else
	RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile AGC_TAB.txt\n");
	rtStatus = phy_ConfigBBWithParaFile(dev, (char* )&szAGCTableFile);
#endif

	if(rtStatus != true){
		printk( "phy_BB8192S_Config_ParaFile():AGC Table Fail\n");
		goto phy_BB8190_Config_ParaFile_Fail;
	}


#if 0	// 2008/08/18 MH Disable for 92SE
	if(pHalData->VersionID > VERSION_8190_BD)
	{
		//if(pHalData->RF_Type == RF_2T4R)
		//{
		// Antenna gain offset from B/C/D to A
		u4RegValue = (  pHalData->AntennaTxPwDiff[2]<<8 | 
						pHalData->AntennaTxPwDiff[1]<<4 | 
						pHalData->AntennaTxPwDiff[0]);
		//}
		//else
		//u4RegValue = 0;	
		
		PHY_SetBBReg(dev, rFPGA0_TxGainStage, 
			(bXBTxAGC|bXCTxAGC|bXDTxAGC), u4RegValue);
    	
		// CrystalCap
		// Simulate 8192???
		u4RegValue = pHalData->CrystalCap;
		PHY_SetBBReg(dev, rFPGA0_AnalogParameter1, bXtalCap92x, u4RegValue);
		// Simulate 8190??
		//u4RegValue = ((pHalData->CrystalCap & 0xc)>>2);	// bit2~3 of crystal cap
		//PHY_SetBBReg(dev, rFPGA0_AnalogParameter2, bXtalCap23, u4RegValue);

	}
#endif

	// Check if the CCK HighPower is turned ON.
	// This is used to calculate PWDB.
	priv->bCckHighPower = (bool)(rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, 0x200));


phy_BB8190_Config_ParaFile_Fail:	
	return rtStatus;
}

/*-----------------------------------------------------------------------------
 * Function:    phy_ConfigMACWithHeaderFile()
 *
 * Overview:    This function read BB parameters from Header file we gen, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		dev
 *			char* 			pFileName			
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note: 		The format of MACPHY_REG.txt is different from PHY and RF. 
 *			[Register][Mask][Value]
 *---------------------------------------------------------------------------*/
//use in phy only
static bool	
phy_ConfigMACWithHeaderFile(struct net_device* dev)
{
	u32					i = 0;
	u32					ArrayLength = 0;
	u32*					ptrArray;	
	//struct r8192_priv 	*priv = ieee80211_priv(dev);
	
//#if (HAL_CODE_BASE != RTL8192_S)	
	/*if(dev->bInHctTest)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Rtl819XMACPHY_ArrayDTM\n"));
		ArrayLength = MACPHY_ArrayLengthDTM;
		ptrArray = Rtl819XMACPHY_ArrayDTM;
	}
	else if(pHalData->bTXPowerDataReadFromEEPORM)
	{
//		RT_TRACE(COMP_INIT, DBG_LOUD, ("Rtl819XMACPHY_Array_PG\n"));
//		ArrayLength = MACPHY_Array_PGLength;
//		ptrArray = Rtl819XMACPHY_Array_PG;

	}else*/
	{ //2008.11.06 Modified by tynli.
		RT_TRACE(COMP_INIT, "Read Rtl819XMACPHY_Array\n");
		ArrayLength = MAC_2T_ArrayLength;
		ptrArray = Rtl819XMAC_Array;	
	}
		
	/*for(i = 0 ;i < ArrayLength;i=i+3){
		RT_TRACE(COMP_SEND, DBG_LOUD, ("The Rtl819XMACPHY_Array[0] is %lx Rtl819XMACPHY_Array[1] is %lx Rtl819XMACPHY_Array[2] is %lx\n",ptrArray[i], ptrArray[i+1], ptrArray[i+2]));
		if(ptrArray[i] == 0x318)
		{
			ptrArray[i+2] = 0x00000800;
			//DbgPrint("ptrArray[i], ptrArray[i+1], ptrArray[i+2] = %x, %x, %x\n",
			//	ptrArray[i], ptrArray[i+1], ptrArray[i+2]);
		}
		PHY_SetBBReg(dev, ptrArray[i], ptrArray[i+1], ptrArray[i+2]);
	}*/
	for(i = 0 ;i < ArrayLength;i=i+2){ // Add by tynli for 2 column
		write_nic_byte(dev, ptrArray[i], (u8)ptrArray[i+1]);
	}
//#endif
	return true;
}

/*-----------------------------------------------------------------------------
 * Function:    phy_ConfigBBWithHeaderFile()
 *
 * Overview:    This function read BB parameters from general file format, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		dev
 *			u8 			ConfigType     0 => PHY_CONFIG
 *										 1 =>AGC_TAB
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 *---------------------------------------------------------------------------*/
//use in phy only
static bool 	
phy_ConfigBBWithHeaderFile(struct net_device* dev,u8 ConfigType)
{
	int 		i,j;
	int dbg = 0;
	//u8 		ArrayLength;	
	u32*	Rtl819XPHY_REGArray_Table;
	u32*	Rtl819XAGCTAB_Array_Table;
	u16		PHY_REGArrayLen, AGCTAB_ArrayLen;
	//struct r8192_priv *priv = ieee80211_priv(dev);
//#if (HAL_CODE_BASE != RTL8192_S)	
	/*if(dev->bInHctTest)
	{	
	
		AGCTAB_ArrayLen = AGCTAB_ArrayLengthDTM;
		Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_ArrayDTM;

		if(pHalData->RF_Type == RF_2T4R)
		{
			PHY_REGArrayLen = PHY_REGArrayLengthDTM;
			Rtl819XPHY_REGArray_Table = Rtl819XPHY_REGArrayDTM;
		}
		else if (pHalData->RF_Type == RF_1T2R)
		{
			PHY_REGArrayLen = PHY_REG_1T2RArrayLengthDTM;
			Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_1T2RArrayDTM;
		}		
	
	}
	else
	*/
	//{
	//
	// 2008.11.06 Modified by tynli.
	//
	AGCTAB_ArrayLen = AGCTAB_ArrayLength;
	Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_Array;
	PHY_REGArrayLen = PHY_REG_2T2RArrayLength; // Default RF_type: 2T2R
	Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_Array;
	//}

    printk("PHY_REGArrayLen = %d\n", PHY_REGArrayLen);
	if(ConfigType == BaseBand_Config_PHY_REG)
	{
	    
	    // runing in this stage
		for(i=0;i<PHY_REGArrayLen;i=i+2)
		{
			if (Rtl819XPHY_REGArray_Table[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfc)
				mdelay(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfb)
				mdelay(5);  //udelay(50);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfa)				
				mdelay(5);//udelay(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xf9)
				mdelay(5);//udelay(1);
			
			printk("%d Set BB reg %08x = %08x\n",i,Rtl819XPHY_REGArray_Table[i], Rtl819XPHY_REGArray_Table[i+1]);                        	            
			
			if (Rtl819XPHY_REGArray_Table[i]==0x90C)            				                   
            {	                
                dbg = 1;
                printk("Try to access BB Reg = 0x90C, wait 10 sec ...\n");                
                
                for (j=0; j < 10; j++)
                {
                    msleep(1000);
                    printk("%d ", 10 - j);
                }
                printk("\n");
            }
                                        	                        				
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArray_Table[i], bMaskDWord, Rtl819XPHY_REGArray_Table[i+1]);		
			
			msleep(100);			
			
			if (readl((volatile unsigned int*)0xb8017ab4)!=0x5710)
            {   
                printk("Link Down !!!\n");                
                printk("reg b8017ab4 = %08x\n", readl((volatile unsigned int*)0xb8017ab4));
                printk("reg b8017a00 = %08x\n", readl((volatile unsigned int*)0xb8017a00));                                                   
            }
                   
			//if (Rtl819XPHY_REGArray_Table[i]==0x90C)            				                   
            //{		                
                //msleep(1000);                
            //}			
			//RT_TRACE(COMP_SEND, "The Rtl819XPHY_REGArray_Table[0] is %lx Rtl819XPHY_REGArray[1] is %lx \n",Rtl819XPHY_REGArray_Table[i], Rtl819XPHY_REGArray_Table[i+1]);            
		}
		//kassert();
	}
	else if(ConfigType == BaseBand_Config_AGC_TAB){
		for(i=0;i<AGCTAB_ArrayLen;i=i+2)
		{
			rtl8192_setBBreg(dev, Rtl819XAGCTAB_Array_Table[i], bMaskDWord, Rtl819XAGCTAB_Array_Table[i+1]);		
		}
	}
//#endif	// #if (HAL_CODE_BASE != RTL8192_S)		
	return true;
}


/*-----------------------------------------------------------------------------
 * Function:    phy_SetBBtoDiffRFWithHeaderFile()
 *
 * Overview:    This function 
 *			
 *
 * Input:      	PADAPTER		dev
 *			u1Byte 			ConfigType     0 => PHY_CONFIG
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 * When			Who		Remark
 * 2008/11/10	tynli
 * use in phy only		
 *---------------------------------------------------------------------------*/
static bool  
phy_SetBBtoDiffRFWithHeaderFile(struct net_device* dev, u8 ConfigType)
{
	int i;
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u32* 			Rtl819XPHY_REGArraytoXTXR_Table;
	u16				PHY_REGArraytoXTXRLen;
	
	if(priv->rf_type == RF_1T1R)
	{
		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to1T1R_Array;
		PHY_REGArraytoXTXRLen = PHY_ChangeTo_1T1RArrayLength;
	} 
	else if(priv->rf_type == RF_1T2R)
	{
		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to1T2R_Array;
		PHY_REGArraytoXTXRLen = PHY_ChangeTo_1T2RArrayLength;
	}
//   else if(priv->rf_type == RF_2T2R || priv->rf_type == RF_2T2R_GREEN)
//	{
//		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to2T2R_Array;
//		PHY_REGArraytoXTXRLen = PHY_ChangeTo_2T2RArrayLength;
//	}
	else
	{
		return false;
	}

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArraytoXTXRLen;i=i+3)
		{
			if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xf9)
				udelay(1);
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArraytoXTXR_Table[i], Rtl819XPHY_REGArraytoXTXR_Table[i+1], Rtl819XPHY_REGArraytoXTXR_Table[i+2]);		
			//RT_TRACE(COMP_SEND,  
			//"The Rtl819XPHY_REGArraytoXTXR_Table[0] is %lx Rtl819XPHY_REGArraytoXTXR_Table[1] is %lx Rtl819XPHY_REGArraytoXTXR_Table[2] is %lx \n",
			//Rtl819XPHY_REGArraytoXTXR_Table[i],Rtl819XPHY_REGArraytoXTXR_Table[i+1], Rtl819XPHY_REGArraytoXTXR_Table[i+2]);
		}
	}
	else {
		RT_TRACE(COMP_SEND, "phy_SetBBtoDiffRFWithHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n");
	}
	return true;
}

void
storePwrIndexDiffRateOffset(
	struct net_device* dev,
	u32		RegAddr,
	u32		BitMask,
	u32		Data
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	
	if(RegAddr == rTxAGC_Rate18_06)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][0] = Data;
		//printk("MCSTxPowerLevelOriginalOffset[%d][0] = 0x%x\n", priv->pwrGroupCnt, priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][0]);
	}
	if(RegAddr == rTxAGC_Rate54_24)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][1] = Data;
		//printk("MCSTxPowerLevelOriginalOffset[%d][1] = 0x%x\n", priv->pwrGroupCnt, priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][1]);
	}
	if(RegAddr == rTxAGC_CCK_Mcs32)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][6] = Data;
		//printk("MCSTxPowerLevelOriginalOffset[%d][6] = 0x%x\n", priv->pwrGroupCnt, priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][6]);
	}
	if(RegAddr == rTxAGC_Mcs03_Mcs00)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][2] = Data;
		//printk("MCSTxPowerLevelOriginalOffset[%d][2] = 0x%x\n", priv->pwrGroupCnt, priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][2]);
	}
	if(RegAddr == rTxAGC_Mcs07_Mcs04)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][3] = Data;
		//printk("MCSTxPowerLevelOriginalOffset[%d][3] = 0x%x\n", priv->pwrGroupCnt, priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][3]);
	}
	if(RegAddr == rTxAGC_Mcs11_Mcs08)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][4] = Data;
		//printk("MCSTxPowerLevelOriginalOffset[%d][4] = 0x%x\n", priv->pwrGroupCnt, priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][4]);
	}
	if(RegAddr == rTxAGC_Mcs15_Mcs12)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][5] = Data;
		//printk("MCSTxPowerLevelOriginalOffset[%d][5] = 0x%x\n", priv->pwrGroupCnt, priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][5]);
		priv->pwrGroupCnt++;
	}
}

/*-----------------------------------------------------------------------------
 * Function:	phy_ConfigBBWithPgHeaderFile
 *
 * Overview:	Config PHY_REG_PG array 
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/06/2008 	MHC		Add later!!!!!!.. Please modify for new files!!!!
 * 11/10/2008	tynli		Modify to mew files.
 //use in phy only
 *---------------------------------------------------------------------------*/
static bool 
phy_ConfigBBWithPgHeaderFile(struct net_device* dev,u8 ConfigType)
{
	int i;
	u32*	Rtl819XPHY_REGArray_Table_PG;
	u16	PHY_REGArrayPGLen;

	PHY_REGArrayPGLen = PHY_REG_Array_PGLength;
	Rtl819XPHY_REGArray_Table_PG = Rtl819XPHY_REG_Array_PG;

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArrayPGLen;i=i+3)
		{
			if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xf9)
				udelay(1);
			storePwrIndexDiffRateOffset(dev, Rtl819XPHY_REGArray_Table_PG[i], 
				Rtl819XPHY_REGArray_Table_PG[i+1], 
				Rtl819XPHY_REGArray_Table_PG[i+2]);
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArray_Table_PG[i], Rtl819XPHY_REGArray_Table_PG[i+1], Rtl819XPHY_REGArray_Table_PG[i+2]);		
		}
	}
	else{
		RT_TRACE(COMP_SEND, "phy_ConfigBBWithPgHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n");
	}
	return true;
	
}	/* phy_ConfigBBWithPgHeaderFile */

/*-----------------------------------------------------------------------------
 * Function:    PHY_ConfigRFWithHeaderFile()
 *
 * Overview:    This function read RF parameters from general file format, and do RF 3-wire
 *
 * Input:      	PADAPTER			dev
 *			char* 				pFileName			
 *			RF90_RADIO_PATH_E	eRFPath
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note:		Delay may be required for RF configuration
 *---------------------------------------------------------------------------*/
//in 8256 phy_RF8256_Config_ParaFile only
//RT_STATUS PHY_ConfigRFWithHeaderFile(struct net_device* dev,RF90_RADIO_PATH_E eRFPath)
u8 rtl8192_phy_ConfigRFWithHeaderFile(struct net_device* dev, RF90_RADIO_PATH_E	eRFPath)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	int			i;
	bool 		rtStatus = true;
	u32			*Rtl819XRadioA_Array_Table;
	u32			*Rtl819XRadioB_Array_Table;
	u16			RadioA_ArrayLen,RadioB_ArrayLen;

	RadioA_ArrayLen = RadioA_1T_ArrayLength;
	Rtl819XRadioA_Array_Table=Rtl819XRadioA_Array;

	//
	// <Roger_Notes> Using Green mode array table for RF_2T2R_GREEN
	// 2008.12.24.
	//
	if(priv->rf_type == RF_2T2R_GREEN)
	{
		Rtl819XRadioB_Array_Table = Rtl819XRadioB_GM_Array;
		RadioB_ArrayLen = RadioB_GM_ArrayLength;
	}
	else
	{		
		Rtl819XRadioB_Array_Table=Rtl819XRadioB_Array;
		RadioB_ArrayLen = RadioB_ArrayLength;	
	}


	RT_TRACE(COMP_INIT, "PHY_ConfigRFWithHeaderFile: Radio No %x\n", eRFPath);
	rtStatus = true;

	switch(eRFPath){
		case RF90_PATH_A:
			for(i = 0;i<RadioA_ArrayLen; i=i+2)
			{
				if(Rtl819XRadioA_Array_Table[i] == 0xfe)
				{ // Deay specific ms. Only RF configuration require delay.												
					mdelay(50);
				}
				else if (Rtl819XRadioA_Array_Table[i] == 0xfd)
					mdelay(5);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfc)
					mdelay(1);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfb)
					udelay(50);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfa)
					udelay(5);
				else if (Rtl819XRadioA_Array_Table[i] == 0xf9)
					udelay(1);
				else
				{
					rtl8192_phy_SetRFReg(dev, eRFPath, Rtl819XRadioA_Array_Table[i], bMask20Bits, Rtl819XRadioA_Array_Table[i+1]);
				}
			}			
			break;
		case RF90_PATH_B:
			for(i = 0;i<RadioB_ArrayLen; i=i+2){
				if(Rtl819XRadioB_Array_Table[i] == 0xfe)
				{ // Deay specific ms. Only RF configuration require delay.												
					mdelay(50);
				}
				else if (Rtl819XRadioB_Array_Table[i] == 0xfd)
					mdelay(5);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfc)
					mdelay(1);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfb)
					udelay(50);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfa)
					udelay(5);
				else if (Rtl819XRadioB_Array_Table[i] == 0xf9)
					udelay(1);
				else
				{
					rtl8192_phy_SetRFReg(dev, eRFPath, Rtl819XRadioB_Array_Table[i], bMask20Bits, Rtl819XRadioB_Array_Table[i+1]);
				}
			}			
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
		default:
			break;
	}
	return rtStatus;

}

/*-----------------------------------------------------------------------------
 * Function:    PHY_CheckBBAndRFOK()
 *
 * Overview:    This function is write register and then readback to make sure whether
 *			  BB[PHY0, PHY1], RF[Patha, path b, path c, path d] is Ok
 *
 * Input:      	PADAPTER			dev
 *			HW90_BLOCK_E		CheckBlock
 *			RF90_RADIO_PATH_E	eRFPath		// it is used only when CheckBlock is HW90_BLOCK_RF
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: PHY is OK
 *			
 * Note:		This function may be removed in the ASIC
 *---------------------------------------------------------------------------*/
//in 8256 phy_RF8256_Config_HardCode
//but we don't use it temp
bool rtl8192_phy_checkBBAndRF(
	struct net_device* dev,
	HW90_BLOCK_E		CheckBlock,
	RF90_RADIO_PATH_E	eRFPath
	)
{
	//struct r8192_priv *priv = ieee80211_priv(dev);
	bool rtStatus = true;
	u32				i, CheckTimes = 4,ulRegRead = 0;
	u32				WriteAddr[4];
	u32				WriteData[] = {0xfffff027, 0xaa55a02f, 0x00000027, 0x55aa502f};

	// Initialize register address offset to be checked
	WriteAddr[HW90_BLOCK_MAC] = 0x100;
	WriteAddr[HW90_BLOCK_PHY0] = 0x900;
	WriteAddr[HW90_BLOCK_PHY1] = 0x800;
	WriteAddr[HW90_BLOCK_RF] = 0x3;
	
	for(i=0 ; i < CheckTimes ; i++)
	{

		//
		// Write Data to register and readback
		//
		switch(CheckBlock)
		{
		case HW90_BLOCK_MAC:
			//RT_ASSERT(FALSE, ("PHY_CheckBBRFOK(): Never Write 0x100 here!"));
			RT_TRACE(COMP_INIT, "PHY_CheckBBRFOK(): Never Write 0x100 here!\n");
			break;
			
		case HW90_BLOCK_PHY0:
		case HW90_BLOCK_PHY1:
			write_nic_dword(dev, WriteAddr[CheckBlock], WriteData[i]);
			ulRegRead = read_nic_dword(dev, WriteAddr[CheckBlock]);
			break;

		case HW90_BLOCK_RF:
			// When initialization, we want the delay function(mdelay(), delay_us() 
			// ==> actually we call PlatformStallExecution()) to do NdisStallExecution()
			// [busy wait] instead of NdisMSleep(). So we acquire RT_INITIAL_SPINLOCK 
			// to run at Dispatch level to achive it.	
			//cosa PlatformAcquireSpinLock(dev, RT_INITIAL_SPINLOCK);
			WriteData[i] &= 0xfff;
			rtl8192_phy_SetRFReg(dev, eRFPath, WriteAddr[HW90_BLOCK_RF], bMask20Bits, WriteData[i]);
			// TODO: we should not delay for such a long time. Ask SD3
			mdelay(10);
			ulRegRead = rtl8192_phy_QueryRFReg(dev, eRFPath, WriteAddr[HW90_BLOCK_RF], bMaskDWord);				
			mdelay(10);
			//cosa PlatformReleaseSpinLock(dev, RT_INITIAL_SPINLOCK);
			break;
			
		default:
			rtStatus = false;
			break;
		}


		//
		// Check whether readback data is correct
		//
		if(ulRegRead != WriteData[i])
		{
			//RT_TRACE(COMP_FPGA,  ("ulRegRead: %x, WriteData: %x \n", ulRegRead, WriteData[i]));
			RT_TRACE(COMP_ERR, "read back error(read:%x, write:%x)\n", ulRegRead, WriteData[i]);
			rtStatus = false;			
			break;
		}
	}

	return rtStatus;
}

//no use temp in windows driver
#ifdef TO_DO_LIST
void
PHY_SetRFPowerState8192SUsb(
	struct net_device* dev,
	RF_POWER_STATE	RFPowerState
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool			WaitShutDown = FALSE;
	u32			DWordContent;
	//RF90_RADIO_PATH_E	eRFPath;
	u8				eRFPath;
	BB_REGISTER_DEFINITION_T	*pPhyReg;
	
	if(priv->SetRFPowerStateInProgress == TRUE)
		return;
	
	priv->SetRFPowerStateInProgress = TRUE;
	
	// TODO: Emily, 2006.11.21, we should rewrite this function

	if(RFPowerState==RF_SHUT_DOWN)
	{
		RFPowerState=RF_OFF;
		WaitShutDown=TRUE;
	}
	
	
	priv->RFPowerState = RFPowerState;
	switch( priv->rf_chip )
	{
	case RF_8225:
	case RF_6052:
		switch( RFPowerState )
		{
		case RF_ON:
			break;
	
		case RF_SLEEP:
			break;
	
		case RF_OFF:
			break;
		}
		break;

	case RF_8256:
		switch( RFPowerState )
		{
		case RF_ON:
			break;
	
		case RF_SLEEP:
			break;
	
		case RF_OFF:
			for(eRFPath=(RF90_RADIO_PATH_E)RF90_PATH_A; eRFPath < RF90_PATH_MAX; eRFPath++)
			{
				if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))		
					continue;	
				
				pPhyReg = &priv->PHYRegDef[eRFPath];
				rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV, bRFSI_RFENV);
				rtl8192_setBBreg(dev, pPhyReg->rfintfo, bRFSI_RFENV, 0);
			}
			break;
		}
		break;
		
	case RF_8258:
		break;
	}// switch( priv->rf_chip )

	priv->SetRFPowerStateInProgress = FALSE;
}
#endif

//no use temp in windows driver
void PHY_GetHWRegOriginalValue(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
#if 0//cosa, removed
	// read tx power offset
	// Simulate 8192
	priv->MCSTxPowerLevelOriginalOffset[0] =
		rtl8192_QueryBBReg(dev, rTxAGC_Rate18_06, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[1] =
		rtl8192_QueryBBReg(dev, rTxAGC_Rate54_24, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[2] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs03_Mcs00, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[3] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs07_Mcs04, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[4] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs11_Mcs08, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[5] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs15_Mcs12, bMaskDWord);

	// Read CCK offset
	priv->CCKTxPowerLevelOriginalOffset=
		rtl8192_QueryBBReg(dev, rTxAGC_CCK_Mcs32, bMaskDWord);
	RT_TRACE(COMP_INIT, "Legacy OFDM =%08x/%08x HT_OFDM=%08x/%08x/%08x/%08x\n", 
	priv->MCSTxPowerLevelOriginalOffset[0], priv->MCSTxPowerLevelOriginalOffset[1] ,
	priv->MCSTxPowerLevelOriginalOffset[2], priv->MCSTxPowerLevelOriginalOffset[3] ,
	priv->MCSTxPowerLevelOriginalOffset[4], priv->MCSTxPowerLevelOriginalOffset[5] );
#endif

	// read rx initial gain
	priv->DefaultInitialGain[0] = rtl8192_QueryBBReg(dev, rOFDM0_XAAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[1] = rtl8192_QueryBBReg(dev, rOFDM0_XBAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[2] = rtl8192_QueryBBReg(dev, rOFDM0_XCAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[3] = rtl8192_QueryBBReg(dev, rOFDM0_XDAGCCore1, bMaskByte0);
	RT_TRACE(COMP_INIT, "Default initial gain (c50=0x%x, c58=0x%x, c60=0x%x, c68=0x%x) \n", 
			priv->DefaultInitialGain[0], priv->DefaultInitialGain[1], 
			priv->DefaultInitialGain[2], priv->DefaultInitialGain[3]);

	// read framesync
	priv->framesync = rtl8192_QueryBBReg(dev, rOFDM0_RxDetector3, bMaskByte0);
	priv->framesyncC34 = rtl8192_QueryBBReg(dev, rOFDM0_RxDetector2, bMaskDWord);
	RT_TRACE(COMP_INIT, "Default framesync (0x%x) = 0x%x \n", 
				rOFDM0_RxDetector3, priv->framesync);

}



/**
* Function:	phy_InitBBRFRegisterDefinition
*
* OverView:	Initialize Register definition offset for Radio Path A/B/C/D
*
* Input:
*			PADAPTER		dev,
*
* Output:	None
* Return:		None
* Note:		The initialization value is constant and it should never be changes
*/
//use in phy only
static void phy_InitBBRFRegisterDefinition(	struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	
	// RF Interface Sowrtware Control
	priv->PHYRegDef[RF90_PATH_A].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 LSBs if read 32-bit from 0x870
	priv->PHYRegDef[RF90_PATH_B].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 MSBs if read 32-bit from 0x870 (16-bit for 0x872)
	priv->PHYRegDef[RF90_PATH_C].rfintfs = rFPGA0_XCD_RFInterfaceSW;// 16 LSBs if read 32-bit from 0x874
	priv->PHYRegDef[RF90_PATH_D].rfintfs = rFPGA0_XCD_RFInterfaceSW;// 16 MSBs if read 32-bit from 0x874 (16-bit for 0x876)

	// RF Interface Readback Value
	priv->PHYRegDef[RF90_PATH_A].rfintfi = rFPGA0_XAB_RFInterfaceRB; // 16 LSBs if read 32-bit from 0x8E0	
	priv->PHYRegDef[RF90_PATH_B].rfintfi = rFPGA0_XAB_RFInterfaceRB;// 16 MSBs if read 32-bit from 0x8E0 (16-bit for 0x8E2)
	priv->PHYRegDef[RF90_PATH_C].rfintfi = rFPGA0_XCD_RFInterfaceRB;// 16 LSBs if read 32-bit from 0x8E4
	priv->PHYRegDef[RF90_PATH_D].rfintfi = rFPGA0_XCD_RFInterfaceRB;// 16 MSBs if read 32-bit from 0x8E4 (16-bit for 0x8E6)

	// RF Interface Output (and Enable)
	priv->PHYRegDef[RF90_PATH_A].rfintfo = rFPGA0_XA_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x860
	priv->PHYRegDef[RF90_PATH_B].rfintfo = rFPGA0_XB_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x864
	priv->PHYRegDef[RF90_PATH_C].rfintfo = rFPGA0_XC_RFInterfaceOE;// 16 LSBs if read 32-bit from 0x868
	priv->PHYRegDef[RF90_PATH_D].rfintfo = rFPGA0_XD_RFInterfaceOE;// 16 LSBs if read 32-bit from 0x86C

	// RF Interface (Output and)  Enable
	priv->PHYRegDef[RF90_PATH_A].rfintfe = rFPGA0_XA_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x860 (16-bit for 0x862)
	priv->PHYRegDef[RF90_PATH_B].rfintfe = rFPGA0_XB_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x864 (16-bit for 0x866)
	priv->PHYRegDef[RF90_PATH_C].rfintfe = rFPGA0_XC_RFInterfaceOE;// 16 MSBs if read 32-bit from 0x86A (16-bit for 0x86A)
	priv->PHYRegDef[RF90_PATH_D].rfintfe = rFPGA0_XD_RFInterfaceOE;// 16 MSBs if read 32-bit from 0x86C (16-bit for 0x86E)

	//Addr of LSSI. Wirte RF register by driver
	priv->PHYRegDef[RF90_PATH_A].rf3wireOffset = rFPGA0_XA_LSSIParameter; //LSSI Parameter
	priv->PHYRegDef[RF90_PATH_B].rf3wireOffset = rFPGA0_XB_LSSIParameter;
	priv->PHYRegDef[RF90_PATH_C].rf3wireOffset = rFPGA0_XC_LSSIParameter;
	priv->PHYRegDef[RF90_PATH_D].rf3wireOffset = rFPGA0_XD_LSSIParameter;

	// RF parameter
	priv->PHYRegDef[RF90_PATH_A].rfLSSI_Select = rFPGA0_XAB_RFParameter;  //BB Band Select
	priv->PHYRegDef[RF90_PATH_B].rfLSSI_Select = rFPGA0_XAB_RFParameter;
	priv->PHYRegDef[RF90_PATH_C].rfLSSI_Select = rFPGA0_XCD_RFParameter;
	priv->PHYRegDef[RF90_PATH_D].rfLSSI_Select = rFPGA0_XCD_RFParameter;

	// Tx AGC Gain Stage (same for all path. Should we remove this?)
	priv->PHYRegDef[RF90_PATH_A].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	priv->PHYRegDef[RF90_PATH_B].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	priv->PHYRegDef[RF90_PATH_C].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	priv->PHYRegDef[RF90_PATH_D].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage

	// Tranceiver A~D HSSI Parameter-1
	priv->PHYRegDef[RF90_PATH_A].rfHSSIPara1 = rFPGA0_XA_HSSIParameter1;  //wire control parameter1
	priv->PHYRegDef[RF90_PATH_B].rfHSSIPara1 = rFPGA0_XB_HSSIParameter1;  //wire control parameter1
	priv->PHYRegDef[RF90_PATH_C].rfHSSIPara1 = rFPGA0_XC_HSSIParameter1;  //wire control parameter1
	priv->PHYRegDef[RF90_PATH_D].rfHSSIPara1 = rFPGA0_XD_HSSIParameter1;  //wire control parameter1

	// Tranceiver A~D HSSI Parameter-2
	priv->PHYRegDef[RF90_PATH_A].rfHSSIPara2 = rFPGA0_XA_HSSIParameter2;  //wire control parameter2
	priv->PHYRegDef[RF90_PATH_B].rfHSSIPara2 = rFPGA0_XB_HSSIParameter2;  //wire control parameter2
	priv->PHYRegDef[RF90_PATH_C].rfHSSIPara2 = rFPGA0_XC_HSSIParameter2;  //wire control parameter2
	priv->PHYRegDef[RF90_PATH_D].rfHSSIPara2 = rFPGA0_XD_HSSIParameter2;  //wire control parameter1

	// RF switch Control
	priv->PHYRegDef[RF90_PATH_A].rfSwitchControl = rFPGA0_XAB_SwitchControl; //TR/Ant switch control
	priv->PHYRegDef[RF90_PATH_B].rfSwitchControl = rFPGA0_XAB_SwitchControl;
	priv->PHYRegDef[RF90_PATH_C].rfSwitchControl = rFPGA0_XCD_SwitchControl;
	priv->PHYRegDef[RF90_PATH_D].rfSwitchControl = rFPGA0_XCD_SwitchControl;

	// AGC control 1 
	priv->PHYRegDef[RF90_PATH_A].rfAGCControl1 = rOFDM0_XAAGCCore1;
	priv->PHYRegDef[RF90_PATH_B].rfAGCControl1 = rOFDM0_XBAGCCore1;
	priv->PHYRegDef[RF90_PATH_C].rfAGCControl1 = rOFDM0_XCAGCCore1;
	priv->PHYRegDef[RF90_PATH_D].rfAGCControl1 = rOFDM0_XDAGCCore1;

	// AGC control 2 
	priv->PHYRegDef[RF90_PATH_A].rfAGCControl2 = rOFDM0_XAAGCCore2;
	priv->PHYRegDef[RF90_PATH_B].rfAGCControl2 = rOFDM0_XBAGCCore2;
	priv->PHYRegDef[RF90_PATH_C].rfAGCControl2 = rOFDM0_XCAGCCore2;
	priv->PHYRegDef[RF90_PATH_D].rfAGCControl2 = rOFDM0_XDAGCCore2;

	// RX AFE control 1 
	priv->PHYRegDef[RF90_PATH_A].rfRxIQImbalance = rOFDM0_XARxIQImbalance;
	priv->PHYRegDef[RF90_PATH_B].rfRxIQImbalance = rOFDM0_XBRxIQImbalance;
	priv->PHYRegDef[RF90_PATH_C].rfRxIQImbalance = rOFDM0_XCRxIQImbalance;
	priv->PHYRegDef[RF90_PATH_D].rfRxIQImbalance = rOFDM0_XDRxIQImbalance;	

	// RX AFE control 1  
	priv->PHYRegDef[RF90_PATH_A].rfRxAFE = rOFDM0_XARxAFE;
	priv->PHYRegDef[RF90_PATH_B].rfRxAFE = rOFDM0_XBRxAFE;
	priv->PHYRegDef[RF90_PATH_C].rfRxAFE = rOFDM0_XCRxAFE;
	priv->PHYRegDef[RF90_PATH_D].rfRxAFE = rOFDM0_XDRxAFE;	

	// Tx AFE control 1 
	priv->PHYRegDef[RF90_PATH_A].rfTxIQImbalance = rOFDM0_XATxIQImbalance;
	priv->PHYRegDef[RF90_PATH_B].rfTxIQImbalance = rOFDM0_XBTxIQImbalance;
	priv->PHYRegDef[RF90_PATH_C].rfTxIQImbalance = rOFDM0_XCTxIQImbalance;
	priv->PHYRegDef[RF90_PATH_D].rfTxIQImbalance = rOFDM0_XDTxIQImbalance;	

	// Tx AFE control 2 
	priv->PHYRegDef[RF90_PATH_A].rfTxAFE = rOFDM0_XATxAFE;
	priv->PHYRegDef[RF90_PATH_B].rfTxAFE = rOFDM0_XBTxAFE;
	priv->PHYRegDef[RF90_PATH_C].rfTxAFE = rOFDM0_XCTxAFE;
	priv->PHYRegDef[RF90_PATH_D].rfTxAFE = rOFDM0_XDTxAFE;	

	// Tranceiver LSSI Readback
	priv->PHYRegDef[RF90_PATH_A].rfLSSIReadBack = rFPGA0_XA_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_B].rfLSSIReadBack = rFPGA0_XB_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_C].rfLSSIReadBack = rFPGA0_XC_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_D].rfLSSIReadBack = rFPGA0_XD_LSSIReadBack;	

	// Tranceiver LSSI Readback PI mode 
	priv->PHYRegDef[RF90_PATH_A].rfLSSIReadBackPi = TransceiverA_HSPI_Readback;
	priv->PHYRegDef[RF90_PATH_B].rfLSSIReadBackPi = TransceiverB_HSPI_Readback;
	//priv->PHYRegDef[RF90_PATH_C].rfLSSIReadBackPi = rFPGA0_XC_LSSIReadBack;
	//priv->PHYRegDef[RF90_PATH_D].rfLSSIReadBackPi = rFPGA0_XD_LSSIReadBack;	

}


//
//	Description: 
//		Change RF power state.
//
//	Assumption:	
//		This function must be executed in re-schdulable context, 
//		ie. PASSIVE_LEVEL.
//
//	050823, by rcnjko.
//
//SetHwReg8192SUsb--->HalFunc.SetHwRegHandler
bool PHY_SetRFPowerState(struct net_device* dev, RT_RF_POWER_STATE eRFPowerState)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool			bResult = false;

	RT_TRACE((COMP_PS && COMP_RF), "---------> PHY_SetRFPowerState(): eRFPowerState(%d)\n", eRFPowerState);

	if(eRFPowerState == priv->ieee80211->eRFPowerState)
	{
		RT_TRACE((COMP_PS && COMP_RF), "<--------- PHY_SetRFPowerState(): discard the request for eRFPowerState(%d) is the same.\n", eRFPowerState);
		return bResult;
	}

	bResult = phy_SetRFPowerState8192SE(dev, eRFPowerState);
	
	RT_TRACE((COMP_PS && COMP_RF), "<--------- PHY_SetRFPowerState(): bResult(%d)\n", bResult);

	return bResult;
}

//
// Description:
//	Set the RF power state for 8192SE.
// Note:
//	This function may turn off the radio like halting the adapter, so when the eRFPowerState is eRFOn it may
//	mean that the NIC should be re-initialized.
// By Bruce, 2008-12-25.
//
//use in phy only
static bool phy_SetRFPowerState8192SE(struct net_device* dev,RT_RF_POWER_STATE eRFPowerState)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));
	bool 	bResult = true;
	u8		i, QueueID;
	//u8		u1bTmp;
	//u8		reset_times = 0;
	struct rtl8192_tx_ring  *ring = NULL;
	priv->SetRFPowerStateInProgress = true;
	
	switch(priv->rf_chip )
	{
		default:	// RL6052 for 8192S now!!!
		switch( eRFPowerState )
		{
			//
			// SW radio on/IPS site survey call will execute all flow
			// HW radio on
			//
			case eRfOn:
                                RT_TRACE(COMP_PS,"========>%s():eRfOn\n", __func__);
				{
									if((priv->ieee80211->eRFPowerState == eRfOff) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC))
					{ // The current RF state is OFF and the RF OFF level is halting the NIC, re-initialize the NIC.
						bool rtstatus = true;
						u32 InitializeCount = 0;
						//printk("=======>initializing\n");
//						NicIFDisableInterrupt(Adapter); //need it  if i disable int in rf off?
//						rtstatus = NicIFEnableNIC( Adapter );
						do
						{	
							InitializeCount++;
							rtstatus = NicIFEnableNIC(dev);
							//rtstatus = _rtl8192_up(dev);
						}while( (rtstatus != true) &&(InitializeCount < 10) );
						if(rtstatus != true)
						{
							RT_TRACE(COMP_ERR,"%s():Initialize Adapter fail,return\n",__FUNCTION__);
							priv->SetRFPowerStateInProgress = false;
							return false;
						}
						//printk("=============>initialing completely\n");
						RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
					}
					else
					{ // This is the normal case, we just turn on the RF.
						write_nic_word(dev, CMDR, 0x37FC);
						write_nic_byte(dev, TXPAUSE, 0x00);
						write_nic_byte(dev, PHY_CCA, 0x3);
					}

#if 1
					// Turn on RF we are still linked, which might happen when 
					// we quickly turn off and on HW RF. 2006.05.12, by rcnjko.
					if(priv->ieee80211->state == IEEE80211_LINKED)
					{
						LedControl8192SE(dev, LED_CTL_LINK); 
					}
					else
					{
						// Turn off LED if RF is not ON.
						LedControl8192SE(dev, LED_CTL_NO_LINK); 
					}
#endif
				}
				break;
			//
			// Card Disable/SW radio off/HW radio off/IPS enter call
			//
			case eRfOff:  
                                RT_TRACE(COMP_PS,"========>%s():eRfOff\n", __func__);
				{					
					// Make sure BusyQueue is empty befor turn off RFE pwoer.
					for(QueueID = 0, i = 0; QueueID < MAX_TX_QUEUE; )
					{
						ring = &priv->tx_ring[QueueID];
						if(skb_queue_len(&ring->queue) == 0)
						{
							//DbgPrint("QueueID = %d", QueueID);
							QueueID++;
							continue;
						}
					#ifdef TO_DO_LIST
					#if( DEV_BUS_TYPE==PCI_INTERFACE)
						else if(IsLowPowerState(Adapter))
						{
							RT_TRACE(COMP_PS, DBG_LOUD, 
							("eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 but lower power state!\n", (i+1), QueueID));
							break;
						}
					#endif
					#endif
						else
						{
							RT_TRACE(COMP_PS, "eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 before doze!\n", (i+1), QueueID);
							udelay(10);
							i++;
						}
						
						if(i >= MAX_DOZE_WAITING_TIMES_9x)
						{
							RT_TRACE(COMP_PS, "\n\n\n %s(): eRfOff: %d times TcbBusyQueue[%d] != 0 !!!\n\n\n", __FUNCTION__,MAX_DOZE_WAITING_TIMES_9x, QueueID);
							break;
						}
					}

					if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_HALT_NIC)
					{ // Disable all components.
						// why not just call haltadapter92se ?? neo 2009 1,3
						//SET_RTL8192SE_RF_HALT(Adapter);
						//printk("========>call NicIFDisableNIC\n");
						NicIFDisableNIC(dev);
						//rtl8192_down(dev,false);
						RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
					}
					else
					{ // Normal case.
						SET_RTL8192SE_RF_SLEEP(dev);

						//
						//If Rf off reason is from IPS, Led should blink with no link, by Maddest 071015
						//
#if 1
						if(priv->ieee80211->RfOffReason == RF_CHANGE_BY_IPS )
						{
							LedControl8192SE(dev,LED_CTL_NO_LINK); 
						}
						else
						{
							// Turn off LED if RF is not ON.
							LedControl8192SE(dev, LED_CTL_POWER_OFF); 
						}
#endif
					}
				}
				break;

			case eRfSleep:
                                RT_TRACE(COMP_PS,"========>%s():eRfSleep\n", __func__);
				{
					// HW setting had been configured with deeper mode.
					if(priv->ieee80211->eRFPowerState == eRfOff)
						break;
					
					// Make sure BusyQueue is empty befor turn off RFE pwoer.
					for(QueueID = 0, i = 0; QueueID < MAX_TX_QUEUE; )
					{
						ring = &priv->tx_ring[QueueID];
						if(skb_queue_len(&ring->queue) == 0)
						{
							QueueID++;
							continue;
						}
						#ifdef TO_DO_LIST
						#if( DEV_BUS_TYPE==PCI_INTERFACE)
							else if(IsLowPowerState(Adapter))
							{
								RT_TRACE(COMP_PS, "eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 but lower power state!\n", (i+1), QueueID);
								break;
							}
						#endif
						#endif
						else
						{
							RT_TRACE(COMP_PS, "eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 before doze!\n", (i+1), QueueID);
							udelay(10);
							i++;
						}
						
						if(i >= MAX_DOZE_WAITING_TIMES_9x)
						{
							RT_TRACE(COMP_PS, "\n\n\n %s(): eRfOff: %d times TcbBusyQueue[%d] != 0 !!!\n\n\n", __FUNCTION__,MAX_DOZE_WAITING_TIMES_9x, QueueID);
							break;
						}
					}		

					SET_RTL8192SE_RF_SLEEP(dev);
				}
				break;

			default:
				bResult = false;
				RT_TRACE(COMP_ERR, "phy_SetRFPowerState8192S(): unknow state to set: 0x%X!!!\n", eRFPowerState);
				break;
		} 
		break;
	}

	if(bResult)
	{
		// Update current RF state variable.
		priv->ieee80211->eRFPowerState = eRFPowerState;
	}
	
	priv->SetRFPowerStateInProgress = false;

	return bResult;
}


/*-----------------------------------------------------------------------------
 * Function:	PHY_SwitchEphyParameter()
 *
 * Overview:	To prevent ASPM error. We need to change some EPHY parameter to 
 *			replace HW autoload value..
 *
 * Input:		IN	PADAPTER			pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	12/26/2008	MHC		Create. The flow is refered to DD PCIE.
 *
 *---------------------------------------------------------------------------*/
void
PHY_SwitchEphyParameter(struct net_device* dev)
{
	// The way to be capable to switch clock request when the PG setting does not
	// support clock request. This is the backdoor solution to switch clock request before
	// ASPM or D3.
	write_nic_dword(dev, 0x540, 0x73c11);
	write_nic_dword(dev, 0x548, 0x2407c);
	
	// Switch EPHY parameter!!!!
	write_nic_word(dev, 0x550, 0x1000);
	write_nic_byte(dev, 0x554, 0x20);	
	phy_CheckEphySwitchReady(dev);

	write_nic_word(dev, 0x550, 0xa0eb);
	write_nic_byte(dev, 0x554, 0x3e);
	phy_CheckEphySwitchReady(dev);
	
	write_nic_word(dev, 0x550, 0xff80);
	write_nic_byte(dev, 0x554, 0x39);
	phy_CheckEphySwitchReady(dev);

	//
	// Delay L1 enter time
	//
	write_nic_byte(dev, 0x560, 0x40);
	
}	// PHY_SwitchEphyParameter


/*-----------------------------------------------------------------------------
 * Function:    GetTxPowerLevel8190()
 *
 * Overview:    This function is export to "common" moudule
 *
 * Input:       PADAPTER		dev
 *			psByte			Power Level
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 *---------------------------------------------------------------------------*/
 // no use temp
 void
PHY_GetTxPowerLevel8192S(
	struct net_device* dev,
	 long*    		powerlevel
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8			TxPwrLevel = 0;
	long			TxPwrDbm;
	//
	// Because the Tx power indexes are different, we report the maximum of them to 
	// meet the CCX TPC request. By Bruce, 2008-01-31.
	//

	// CCK
	TxPwrLevel = priv->CurrentCckTxPwrIdx;
	TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_B, TxPwrLevel);

	// Legacy OFDM
	TxPwrLevel = priv->CurrentOfdm24GTxPwrIdx + priv->LegacyHTTxPowerDiff;

	// Compare with Legacy OFDM Tx power.
	if(phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_G, TxPwrLevel);
	// HT OFDM
	TxPwrLevel = priv->CurrentOfdm24GTxPwrIdx;

	// Compare with HT OFDM Tx power.
	if(phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_N_24G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_N_24G, TxPwrLevel);
	*powerlevel = TxPwrDbm;
}

/*-----------------------------------------------------------------------------
 * Function:    SetTxPowerLevel8190()
 *
 * Overview:    This function is export to "HalCommon" moudule
 *
 * Input:       PADAPTER		dev
 *			u1Byte		channel
 *
 * Output:      NONE
 *
 * Return:      NONE
 *	2008/11/04	MHC		We remove EEPROM_93C56.
 *						We need to move CCX relative code to independet file.
 *
 *---------------------------------------------------------------------------*/
void rtl8192_phy_setTxPower(struct net_device* dev, u8	channel)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(dev);
	u8	powerlevel = (u8)EEPROM_Default_TxPower, powerlevelOFDM24G = 0x10;
	s8 ant_pwr_diff = 0;
	u32	u4RegValue;
	u8	index = (channel -1);
	// 2009/01/22 MH Add for new EEPROM format from SD3
	u8	pwrdiff[2] = {0};
	u8	ht20pwr[2] = {0}, ht40pwr[2] = {0};
	u8	rfpath = 0, rfpathnum = 2;

	if(priv->bTXPowerDataReadFromEEPORM == FALSE)
		return;

	//
	// Read predefined TX power index in EEPROM
	// 
#if EEPROM_OLD_FORMAT_SUPPORT
	powerlevel = priv->TxPowerLevelCCK[channel-1];
	powerlevelOFDM24G = priv->TxPowerLevelOFDM24G[channel-1];
#else	
	//
	// Mainly we use RF-A Tx Power to write the Tx Power registers, but the RF-B Tx
	// Power must be calculated by the antenna diff.
	// So we have to rewrite Antenna gain offset register here.		
	// Please refer to BB register 0x80c
	// 1. For CCK.
	// 2. For OFDM 1T or 2T
	//

	// 1. CCK
	powerlevel = priv->RfTxPwrLevelCck[0][index];
		
	// 2. OFDM for 1T or 2T
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_1T1R)
	{
		powerlevelOFDM24G = priv->RfTxPwrLevelOfdm1T[0][index];
		// RF B HT OFDM pwr-RFA HT OFDM pwr
		// Only one RF we need not to decide B <-> A pwr diff
		
		// Legacy<->HT pwr diff, we only care about path A.
		
		// We only assume 1T as RF path A
		rfpathnum = 1;
		ht20pwr[0] = ht40pwr[0] = priv->RfTxPwrLevelOfdm1T[0][index];
	}
	else if (priv->rf_type == RF_2T2R)
	{
		powerlevelOFDM24G = priv->RfTxPwrLevelOfdm2T[0][index];
		ant_pwr_diff = 	priv->RfTxPwrLevelOfdm2T[1][index] - 
						priv->RfTxPwrLevelOfdm2T[0][index];
		// RF B (HT OFDM pwr+legacy-ht-diff) -(RFA HT OFDM pwr+legacy-ht-diff)
		// We can not handle Path B&A HT/Legacy pwr diff for 92S now.
	
		RT_TRACE(COMP_POWER, "CH-%d HT40 A/B Pwr index = %x/%x(%d/%d)\n", 
			channel, priv->RfTxPwrLevelOfdm2T[0][index], 
			priv->RfTxPwrLevelOfdm2T[1][index],
			priv->RfTxPwrLevelOfdm2T[0][index],
			priv->RfTxPwrLevelOfdm2T[1][index]);	

		ht20pwr[0] = ht40pwr[0] = priv->RfTxPwrLevelOfdm2T[0][index];
		ht20pwr[1] = ht40pwr[1] = priv->RfTxPwrLevelOfdm2T[1][index];
	}
	RT_TRACE(COMP_POWER, "Channel-%d, set tx power index\n", channel);
				
	//
	// 2009/01/21 MH Support new EEPROM format from SD3 requirement
	// 2009/02/10 Cosa, Here is only for reg B/C/D to A gain diff.
	//
	if (priv->eeprom_version >= 2)	// Defined by SD1 Jong
	{		
		if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
		{
			for (rfpath = 0; rfpath < rfpathnum; rfpath++)
			{
				// HT 20<->40 pwr diff
				pwrdiff[rfpath] = priv->TxPwrHt20Diff[rfpath][index];

				// Calculate Antenna pwr diff			
				if (pwrdiff[rfpath] < 8)	// 0~+7
				{
					ht20pwr[rfpath] += pwrdiff[rfpath];
				}
				else				// index8-15=-8~-1
				{
					ht20pwr[rfpath] -= (16-pwrdiff[rfpath]);
				}
			}

			// RF B HT OFDM pwr-RFA HT OFDM pwr
			if (priv->rf_type == RF_2T2R)
				ant_pwr_diff = ht20pwr[1] - ht20pwr[0];

			RT_TRACE(COMP_POWER, "HT20 to HT40 pwrdiff[A/B]=%d/%d, ant_pwr_diff=%d(B-A=%d-%d)\n", 
				pwrdiff[0], pwrdiff[1], ant_pwr_diff, ht20pwr[1], ht20pwr[0]);	
		}
	}
				
	//Cosa added for protection, the reg rFPGA0_TxGainStage 
	// range is from 7~-8, index = 0x0~0xf
	if(ant_pwr_diff > 7)
		ant_pwr_diff = 7;
	if(ant_pwr_diff < -8)
		ant_pwr_diff = -8;
				
	RT_TRACE(COMP_POWER, "CCK/HT Power index = %x/%x(%d/%d), ant_pwr_diff=%d\n", 
		powerlevel, powerlevelOFDM24G, powerlevel, powerlevelOFDM24G, ant_pwr_diff);
		
	ant_pwr_diff &= 0xf;	

	// Antenna TX power difference
	priv->AntennaTxPwDiff[2] = 0;// RF-D, don't care
	priv->AntennaTxPwDiff[1] = 0;// RF-C, don't care
	priv->AntennaTxPwDiff[0] = (u8)(ant_pwr_diff);		// RF-B
	RT_TRACE(COMP_POWER, "pHalData->AntennaTxPwDiff[0]/[1]/[2] = 0x%x/0x%x/0x%x\n", 
		priv->AntennaTxPwDiff[0], priv->AntennaTxPwDiff[1], priv->AntennaTxPwDiff[2]);
	// Antenna gain offset from B/C/D to A
	u4RegValue = (	priv->AntennaTxPwDiff[2]<<8 | 
					priv->AntennaTxPwDiff[1]<<4 | 
					priv->AntennaTxPwDiff[0]	);
	RT_TRACE(COMP_POWER, "BCD-Diff=0x%x\n", u4RegValue);

	// Notify Tx power difference for B/C/D to A!!!
	rtl8192_setBBreg(dev, rFPGA0_TxGainStage, (bXBTxAGC|bXCTxAGC|bXDTxAGC), u4RegValue);
#endif
		
	//
	// CCX 2 S31, AP control of client transmit power:
	// 1. We shall not exceed Cell Power Limit as possible as we can.
	// 2. Tolerance is +/- 5dB.
	// 3. 802.11h Power Contraint takes higher precedence over CCX Cell Power Limit.
	// 
	// TODO: 
	// 1. 802.11h power contraint 
	//
	// 071011, by rcnjko.
	//
#ifdef TODO //WB, 11h has not implemented now.
	if(	priv->ieee80211->iw_mode != IW_MODE_INFRA && priv->bWithCcxCellPwr &&
		channel == priv->ieee80211->current_network.channel)
	{
		u8	CckCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_B, priv->CcxCellPwr);
		u8	LegacyOfdmCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_G, priv->CcxCellPwr);
		u8	OfdmCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_N_24G, priv->CcxCellPwr);

		RT_TRACE(COMP_TXAGC,  
		("CCX Cell Limit: %d dbm => CCK Tx power index : %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n", 
		priv->CcxCellPwr, CckCellPwrIdx, LegacyOfdmCellPwrIdx, OfdmCellPwrIdx));
		RT_TRACE(COMP_TXAGC,  
		("EEPROM channel(%d) => CCK Tx power index: %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n",
		channel, powerlevel, powerlevelOFDM24G + priv->LegacyHTTxPowerDiff, powerlevelOFDM24G)); 

		// CCK
		if(powerlevel > CckCellPwrIdx)
			powerlevel = CckCellPwrIdx;
		// Legacy OFDM, HT OFDM
		if(powerlevelOFDM24G + priv->LegacyHTTxPowerDiff > OfdmCellPwrIdx)
		{
			if((OfdmCellPwrIdx - priv->LegacyHTTxPowerDiff) > 0)
			{
				powerlevelOFDM24G = OfdmCellPwrIdx - priv->LegacyHTTxPowerDiff;
			}
			else
			{
				LegacyOfdmCellPwrIdx = 0;
			}
		}

		RT_TRACE(COMP_TXAGC, 
		("Altered CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		powerlevel, powerlevelOFDM24G + priv->LegacyHTTxPowerDiff, powerlevelOFDM24G));
	}
#endif
	priv->CurrentCckTxPwrIdx = powerlevel;
	priv->CurrentOfdm24GTxPwrIdx = powerlevelOFDM24G;

	RT_TRACE(COMP_POWER, "PHY_SetTxPowerLevel8192S(): CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		powerlevel, powerlevelOFDM24G + priv->LegacyHTTxPowerDiff, powerlevelOFDM24G);
	
	switch(priv->rf_chip)
	{
		case RF_8225:
			//YJ,Not support yet
			//PHY_SetRF8225CckTxPower(dev, powerlevel);
			//PHY_SetRF8225OfdmTxPower(dev, powerlevelOFDM24G);
			break;

		case RF_8256:
			//YJ,Not support yet
			//PHY_SetRF8256CCKTxPower(dev, powerlevel);
			//PHY_SetRF8256OFDMTxPower(dev, powerlevelOFDM24G);
			break;

		case RF_6052:
			PHY_RF6052SetCckTxPower(dev, powerlevel);
			PHY_RF6052SetOFDMTxPower(dev, powerlevelOFDM24G, channel);
			break;

		case RF_8258:
			break;
		default:
			break;
	}	

}

//
//	Description:
//		Update transmit power level of all channel supported.
//
//	TODO: 
//		A mode.
//	By Bruce, 2008-02-04.
//    no use temp
bool PHY_UpdateTxPowerDbm8192S(struct net_device* dev, long powerInDbm)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u8				idx;
	u8				rf_path;

	// TODO: A mode Tx power.
	u8	CckTxPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_B, powerInDbm);
	u8	OfdmTxPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_N_24G, powerInDbm);

	if(OfdmTxPwrIdx - priv->LegacyHTTxPowerDiff > 0)
		OfdmTxPwrIdx -= priv->LegacyHTTxPowerDiff;
	else
		OfdmTxPwrIdx = 0;

	RT_TRACE(COMP_POWER, "PHY_UpdateTxPowerDbm8192S(): %ld dBm , CckTxPwrIdx = %d, OfdmTxPwrIdx = %d\n", powerInDbm, CckTxPwrIdx, OfdmTxPwrIdx);

	for(idx = 0; idx < 14; idx++)
	{
		priv->TxPowerLevelCCK[idx] = CckTxPwrIdx;
		priv->TxPowerLevelCCK_A[idx] = CckTxPwrIdx;
		priv->TxPowerLevelCCK_C[idx] = CckTxPwrIdx;
		priv->TxPowerLevelOFDM24G[idx] = OfdmTxPwrIdx;
		priv->TxPowerLevelOFDM24G_A[idx] = OfdmTxPwrIdx;
		priv->TxPowerLevelOFDM24G_C[idx] = OfdmTxPwrIdx;
		for (rf_path = 0; rf_path < 2; rf_path++)
		{
			priv->RfTxPwrLevelCck[rf_path][idx] = CckTxPwrIdx;
			priv->RfTxPwrLevelOfdm1T[rf_path][idx] = 
			priv->RfTxPwrLevelOfdm2T[rf_path][idx] = OfdmTxPwrIdx;
		}
	}

	rtl8192_phy_setTxPower(dev, priv->chan);

	return TRUE;	
}

/*
	Description:
		When beacon interval is changed, the values of the 
		hw registers should be modified.
	By tynli, 2008.10.24.

*/

extern void PHY_SetBeaconHwReg(	struct net_device* dev, u16 BeaconInterval)
{
	u32 NewBeaconNum;	

	NewBeaconNum = BeaconInterval *32 - 64;
	//PlatformEFIOWrite4Byte(dev, WFM3+4, NewBeaconNum);
	//PlatformEFIOWrite4Byte(dev, WFM3, 0xB026007C);
	write_nic_dword(dev, WFM3+4, NewBeaconNum);
	write_nic_dword(dev, WFM3, 0xB026007C);
}

//
//	Description:
//		Map dBm into Tx power index according to 
//		current HW model, for example, RF and PA, and
//		current wireless mode.
//	By Bruce, 2008-01-29.
//    use in phy only
static u8 phy_DbmToTxPwrIdx(
	struct net_device* dev, 
	WIRELESS_MODE	WirelessMode,
	long			PowerInDbm
	)
{
	//struct r8192_priv *priv = ieee80211_priv(dev);
	u8				TxPwrIdx = 0;
	long				Offset = 0;
	

	//
	// Tested by MP, we found that CCK Index 0 equals to 8dbm, OFDM legacy equals to 
	// 3dbm, and OFDM HT equals to 0dbm repectively.
	// Note:
	//	The mapping may be different by different NICs. Do not use this formula for what needs accurate result.  
	// By Bruce, 2008-01-29.
	// 
	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = 8;
		break;

	case WIRELESS_MODE_G:
		Offset = 4;
		break;

	case WIRELESS_MODE_N_24G:
	default:
		Offset = 4;
		break;
	}

	if((PowerInDbm - Offset) > 0)
	{
		TxPwrIdx = (u8)((PowerInDbm - Offset) * 2);
	}
	else
	{
		TxPwrIdx = 0;
	}

	// Tx Power Index is too large.
	if(TxPwrIdx > MAX_TXPWR_IDX_NMODE_92S)
		TxPwrIdx = MAX_TXPWR_IDX_NMODE_92S;

	return TxPwrIdx;
}

//
//	Description:
//		Map Tx power index into dBm according to 
//		current HW model, for example, RF and PA, and
//		current wireless mode.
//	By Bruce, 2008-01-29.
//    use in phy only
static long phy_TxPwrIdxToDbm(
	struct net_device* dev,
	WIRELESS_MODE	WirelessMode,
	u8			TxPwrIdx
	)
{
	//struct r8192_priv *priv = ieee80211_priv(dev);
	long				Offset = 0;
	long				PwrOutDbm = 0;
	
	//
	// Tested by MP, we found that CCK Index 0 equals to 8dbm, OFDM legacy equals to 
	// 3dbm, and OFDM HT equals to 0dbm repectively.
	// Note:
	//	The mapping may be different by different NICs. Do not use this formula for what needs accurate result.  
	// By Bruce, 2008-01-29.
	// 
	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = -7;
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		Offset = -8;
		break;
	default:
		Offset = 4;
		break;
	}

	PwrOutDbm = TxPwrIdx / 2 + Offset; // Discard the decimal part.

	return PwrOutDbm;
}

//FIXME:YJ,090318
//Restore the initial gain and tx power and 0xa0a
#ifdef TO_DO_LIST
extern	void 
PHY_ScanOperationBackup8192S(
	struct net_device* dev,
	u8 Operation
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32				BitMask;
	u8				initial_gain;

#if(RTL8192S_DISABLE_FW_DM == 0)

	if(!priv->bDriverStopped)
	{
		switch(Operation)
		{
			case SCAN_OPT_BACKUP:
				//
				// <Roger_Notes> We halt FW DIG and disable high ppower both two DMs here 
				// and resume both two DMs while scan complete. 
				// 2008.11.27.
				//
				Adapter->HalFunc.SetFwCmdHandler(Adapter, FW_CMD_DIG_HALT);
				Adapter->HalFunc.SetFwCmdHandler(Adapter, FW_CMD_HIGH_PWR_DISABLE);

				break;
			case SCAN_OPT_RESTORE:
				//
				// <Roger_Notes> We resume DIG and enable high power both two DMs here and 
				// recover earlier DIG settings. 
				// 2008.11.27.
				//

				Adapter->HalFunc.SetFwCmdHandler(Adapter, FW_CMD_DIG_RESUME);
				if(pMgntInfo->IOTAction & HT_IOT_ACT_MID_HIGHPOWER)
					Adapter->HalFunc.SetFwCmdHandler(Adapter, FW_CMD_MID_HIGH_PWR_ENABLE);
				else
					Adapter->HalFunc.SetFwCmdHandler(Adapter, FW_CMD_HIGH_PWR_ENABLE);

				Adapter->HalFunc.SetTxPowerLevelHandler(Adapter, priv->chan);
				break;
			default:
				RT_TRACE(COMP_POWER, "Unknown Scan Backup Operation. \n");
				break;
		}
	}
#endif
}
#endif
/*-----------------------------------------------------------------------------
 * Function:    SetBWModeCallback8190Pci()
 *
 * Overview:    Timer callback function for SetSetBWMode
 *
 * Input:       	PRT_TIMER		pTimer
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Note:		(1) We do not take j mode into consideration now
 *			(2) Will two workitem of "switch channel" and "switch channel bandwidth" run
 *			     concurrently?
 *---------------------------------------------------------------------------*/
//    use in phy only (in win it's timer)
void PHY_SetBWModeCallback8192S(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8	 			regBwOpMode, regRRSR_RSC;

	//return;

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//u32				NowL, NowH;
	//u8Byte				BeginTime, EndTime; 

	RT_TRACE(COMP_SWBW, "==>SetBWModeCallback8192s()  Switch to %s bandwidth\n", \
					priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz");

	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SetBWModeInProgress= FALSE;
		return;
	}
	
	if(!priv->up)
	{
		priv->SwChnlInProgress = priv->SetBWModeInProgress = false;
		return;
	}
	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = read_nic_dword_E(dev, TSFR);
	//NowH = read_nic_dword_E(dev, TSFR+4);
	//BeginTime = ((u8Byte)NowH << 32) + NowL;
		
	//3//
	//3//<1>Set MAC register
	//3//
	regBwOpMode = read_nic_byte(dev, BW_OPMODE);
	regRRSR_RSC = read_nic_byte(dev, RRSR+2);
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
		       	// 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_20_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
        		// 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
        	regRRSR_RSC = (regRRSR_RSC&0x90) |(priv->nCur40MhzPrimeSC<<5);
			write_nic_byte(dev, RRSR+2, regRRSR_RSC);
			break;

		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8192s():\
						unknown Bandwidth: %#X\n",priv->CurrentChannelBW);
			break;
	}
	
	//3//
	//3//<2>Set PHY related register
	//3//
	switch(priv->CurrentChannelBW)
	{
		/* 20 MHz channel*/
		case HT_CHANNEL_WIDTH_20:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x0);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x0);

			// Use PHY_REG.txt default value. Do not need to change.
			// Correct the tx power for CCK rate in 40M. Suggest by YN, 20071207
			// It is set in Tx descriptor for 8192x series
		//	rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);
		//	rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);
		//	rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000204);
			// From SD3 WHChang			
		//	rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x00300000, 3);
			if(priv->card_8192_version >= VERSION_8192S_BCUT)
				write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x58);
			break;

		/* 40 MHz channel*/
		case HT_CHANNEL_WIDTH_20_40:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x1);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x1);
			
			// Use PHY_REG.txt default value. Do not need to change.
			// Correct the tx power for CCK rate in 40M. Suggest by YN, 20071207
			//	rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x35360000);
			//	rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x121c252e);
			//	rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000409);
			// From SD3 WHChang			
			//rtl8192_SetBBReg(dev, rFPGA0_AnalogParameter1, 0x00300000, 3);

			// Set Control channel to upper or lower. These settings are required only for 40MHz
			rtl8192_setBBreg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));
			rtl8192_setBBreg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);			
			
			if(priv->card_8192_version >= VERSION_8192S_BCUT)
				write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x18);
			break;
			
		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8192s(): unknown Bandwidth: %#X\n"\
						,priv->CurrentChannelBW);
			break;
			
	}
	//Skip over setting of J-mode in BB register here. Default value is "None J mode". Emily 20070315

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = read_nic_dword_E(dev, TSFR);
	//NowH = read_nic_dword_E(dev, TSFR+4);
	//EndTime = ((u8Byte)NowH << 32) + NowL;
	//RT_TRACE(COMP_SCAN, DBG_LOUD, ("SetBWModeCallback8190Pci: time of SetBWMode = %I64d us!\n", (EndTime - BeginTime)));

	//3<3>Set RF related register
	switch( priv->rf_chip )
	{
		case RF_8225:		
			//PHY_SetRF8225Bandwidth(dev, priv->CurrentChannelBW);
			break;	
			
		case RF_8256:
			// Please implement this function in Hal8190PciPhy8256.c
			PHY_SetRF8256Bandwidth(dev, priv->CurrentChannelBW);
			break;
			
		case RF_8258:
			// Please implement this function in Hal8190PciPhy8258.c
			// PHY_SetRF8258Bandwidth();
			break;

		case RF_PSEUDO_11N:
			// Do Nothing
			break;

		case RF_6052:
			PHY_RF6052SetBandwidth(dev, priv->CurrentChannelBW);
			break;
		default:
			printk("Unknown rf_chip: %d\n", priv->rf_chip);
			break;
	}

	priv->SetBWModeInProgress= FALSE;

	RT_TRACE(COMP_SWBW, "<==SetBWModeCallback8192s() \n" );
}


 /*-----------------------------------------------------------------------------
 * Function:   SetBWMode8190Pci()
 *
 * Overview:  This function is export to "HalCommon" moudule
 *
 * Input:       	PADAPTER			dev
 *			HT_CHANNEL_WIDTH	Bandwidth	//20M or 40M
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Note:		We do not take j mode into consideration now
 *---------------------------------------------------------------------------*/
//extern void PHY_SetBWMode8192S(	struct net_device* dev,
//	HT_CHANNEL_WIDTH	Bandwidth,	// 20M or 40M
//	HT_EXTCHNL_OFFSET	Offset		// Upper, Lower, or Don't care
void rtl8192_SetBWMode(struct net_device *dev, HT_CHANNEL_WIDTH	Bandwidth, HT_EXTCHNL_OFFSET Offset)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	
	// Modified it for 20/40 mhz switch by guangan 070531

	//return;
	
	//if(priv->SwChnlInProgress)
//	if(pMgntInfo->bScanInProgress)
//	{
//		RT_TRACE(COMP_SCAN, DBG_LOUD, ("SetBWMode8190Pci() %s Exit because bScanInProgress!\n", 
//					Bandwidth == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz"));
//		return;
//	}

//	if(priv->SetBWModeInProgress)
//	{
//		// Modified it for 20/40 mhz switch by guangan 070531
//		RT_TRACE(COMP_SCAN, DBG_LOUD, ("SetBWMode8190Pci() %s cancel last timer because SetBWModeInProgress!\n", 
//					Bandwidth == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz"));
//		PlatformCancelTimer(dev, &priv->SetBWModeTimer);
//		//return;
//	}

	if(priv->SetBWModeInProgress)
		return;

	priv->SetBWModeInProgress= TRUE;
	
	priv->CurrentChannelBW = Bandwidth;
	
	if(Offset==HT_EXTCHNL_OFFSET_LOWER)
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_UPPER;
	else if(Offset==HT_EXTCHNL_OFFSET_UPPER)
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_LOWER;
	else
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

#if 0
	if(!priv->bDriverStopped)
	{
#ifdef USE_WORKITEM	
		PlatformScheduleWorkItem(&(priv->SetBWModeWorkItem));//SetBWModeCallback8192SUsbWorkItem
#else
		PlatformSetTimer(dev, &(priv->SetBWModeTimer), 0);//PHY_SetBWModeCallback8192S
#endif		
	}
#endif
	//if(priv->up)
	{
	PHY_SetBWModeCallback8192S(dev);
	}
}

//    use in phy only (in win it's timer)
void PHY_SwChnlCallback8192S(struct net_device *dev)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	u32		delay;
	//bool			ret;
	
#ifdef TO_DO_LIST	
	if (dev->HalFunc.CheckLBusStatusHandler(dev) == FALSE)
		return;
#endif

	RT_TRACE(COMP_CH, "==>SwChnlCallback8190Pci(), switch to channel %d\n", priv->chan);
	
	if(!priv->up)
	{	
		priv->SwChnlInProgress = priv->SetBWModeInProgress = false;
		return;
	}	
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=FALSE;
		return; 								//return immediately if it is peudo-phy	
	}
	
	do{
		if(!priv->SwChnlInProgress)
			break;

		//if(!phy_SwChnlStepByStep(dev, priv->CurrentChannel, &priv->SwChnlStage, &priv->SwChnlStep, &delay))
		if(!phy_SwChnlStepByStep(dev, priv->chan, &priv->SwChnlStage, &priv->SwChnlStep, &delay))
		{
			if(delay>0)
			{
				mdelay(delay);
				//PlatformSetTimer(dev, &priv->SwChnlTimer, delay);
				//mod_timer(&priv->SwChnlTimer,  jiffies + MSECS(delay));
				//==>PHY_SwChnlCallback8192S(dev); for 92se
				//==>SwChnlCallback8192SUsb(dev) for 92su
			}
			else
			continue;
		}
		else
		{
			priv->SwChnlInProgress=FALSE;
			break;
		}
	}while(true);
}

// Call after initialization
//extern void PHY_SwChnl8192S(struct net_device* dev,	u8 channel)
u8 rtl8192_phy_SwChnl(struct net_device* dev, u8 channel)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	//printk("==============>%s(), channel:%d, %d, %d, %d\n", __FUNCTION__, channel, priv->up, priv->SwChnlInProgress, priv->SetBWModeInProgress);
        if(!priv->up)
	{
		priv->SwChnlInProgress = priv->SetBWModeInProgress = false;
		return false;
	}	
	if(priv->SwChnlInProgress)
		return false;

	if(priv->SetBWModeInProgress)
		return false;
	if (0)
	{
		u8 path;
		for(path=0; path<2; path++){
		printk("============>to set channel:%x\n", rtl8192_phy_QueryRFReg(dev, path, 0x18, 0x3ff));
		udelay(10);
		}
	}
	//--------------------------------------------
	switch(priv->ieee80211->mode)
	{
	case WIRELESS_MODE_A:
	case WIRELESS_MODE_N_5G:
		if (channel<=14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_A but channel<=14");
			return false;
		}
		break;
		
	case WIRELESS_MODE_B:
		if (channel>14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_B but channel>14");
			return false;
		}
		break;
		
	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		if (channel>14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_G but channel>14");
			return false;
		}
		break;

	default:
		//	RT_TRACE(COMP_ERR, "Invalid WirelessMode(%#x)!!\n", priv->ieee80211->mode);
		break;
	}
	//--------------------------------------------
	
	priv->SwChnlInProgress = TRUE;
	if( channel == 0)
		channel = 1;
	
	priv->chan=channel;

	priv->SwChnlStage=0;
	priv->SwChnlStep=0;

#if 0
	if(!dev->bDriverStopped)
	{
#ifdef USE_WORKITEM	
		PlatformScheduleWorkItem(&(priv->SwChnlWorkItem));//SwChnlCallback8192SUsbWorkItem
#else
		PlatformSetTimer(dev, &(priv->SwChnlTimer), 0);//PHY_SwChnlCallback8192S, SwChnlCallback8192SUsb
#endif
	}
#endif

	//if(priv->up)
	{
	PHY_SwChnlCallback8192S(dev);
	}
        return true;	
}


//
// Description:
//	Switch channel synchronously. Called by SwChnlByDelayHandler.
//
// Implemented by Bruce, 2008-02-14.
// The following procedure is operted according to SwChanlCallback8190Pci().
// However, this procedure is performed synchronously  which should be running under
// passive level.
// 
//not understant it
void PHY_SwChnlPhy8192S(	// Only called during initialize
	struct net_device* dev,
	u8		channel
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	
	RT_TRACE(COMP_SCAN, "==>PHY_SwChnlPhy8192S(), switch to channel %d.\n", priv->chan);

#ifdef TO_DO_LIST
	// Cannot IO.
	if(RT_CANNOT_IO(dev))
		return;
#endif

	// Channel Switching is in progress.
	if(priv->SwChnlInProgress)
		return;
	
	//return immediately if it is peudo-phy
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=FALSE;
		return;
	}
	
	priv->SwChnlInProgress = TRUE;
	if( channel == 0)
		channel = 1;
	
	priv->chan=channel;
	
	priv->SwChnlStage = 0;
	priv->SwChnlStep = 0;
	
	phy_FinishSwChnlNow(dev,channel);
	
	priv->SwChnlInProgress = FALSE;
}

//    use in phy only 
static bool
phy_SwChnlStepByStep(
	struct net_device* dev,
	u8		channel,
	u8		*stage,
	u8		*step,
	u32		*delay
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//PCHANNEL_ACCESS_SETTING	pChnlAccessSetting;
	SwChnlCmd				PreCommonCmd[MAX_PRECMD_CNT];
	u32					PreCommonCmdCnt;
	SwChnlCmd				PostCommonCmd[MAX_POSTCMD_CNT];
	u32					PostCommonCmdCnt;
	SwChnlCmd				RfDependCmd[MAX_RFDEPENDCMD_CNT];
	u32					RfDependCmdCnt;
	SwChnlCmd				*CurrentCmd = NULL;	
	u8					eRFPath;	
	
	//RT_ASSERT((dev != NULL), ("dev should not be NULL\n"));
	//RT_ASSERT(IsLegalChannel(dev, channel), ("illegal channel: %d\n", channel));
	RT_TRACE(COMP_CH, "===========>%s(), channel:%d, stage:%d, step:%d\n", __FUNCTION__, channel, *stage, *step);		
	//RT_ASSERT((pHalData != NULL), ("pHalData should not be NULL\n"));
#ifdef ENABLE_DOT11D
	if (!IsLegalChannel(priv->ieee80211, channel))
	{
		RT_TRACE(COMP_ERR, "=============>set to illegal channel:%d\n", channel);
		return true; //return true to tell upper caller function this channel setting is finished! Or it will in while loop.
	}		
#endif	

	//pChnlAccessSetting = &dev->MgntInfo.Info8185.ChannelAccessSetting;
	//RT_ASSERT((pChnlAccessSetting != NULL), ("pChnlAccessSetting should not be NULL\n"));
	
	//for(eRFPath = RF90_PATH_A; eRFPath <priv->NumTotalRFPath; eRFPath++)
	//for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	//{
		// <1> Fill up pre common command.
	PreCommonCmdCnt = 0;
	phy_SetSwChnlCmdArray(PreCommonCmd, PreCommonCmdCnt++, MAX_PRECMD_CNT, 
				CmdID_SetTxPowerLevel, 0, 0, 0);
	phy_SetSwChnlCmdArray(PreCommonCmd, PreCommonCmdCnt++, MAX_PRECMD_CNT, 
				CmdID_End, 0, 0, 0);
	
		// <2> Fill up post common command.
	PostCommonCmdCnt = 0;

	phy_SetSwChnlCmdArray(PostCommonCmd, PostCommonCmdCnt++, MAX_POSTCMD_CNT, 
				CmdID_End, 0, 0, 0);
	
		// <3> Fill up RF dependent command.
	RfDependCmdCnt = 0;
	switch( priv->rf_chip )
	{
		case RF_8225:		
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		//RT_ASSERT((channel >= 1 && channel <= 14), ("illegal channel for Zebra: %d\n", channel));
		// 2008/09/04 MH Change channel. 
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rRfChannel, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;	
		
	case RF_8256:
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		// TEST!! This is not the table for 8256!!
		//RT_ASSERT((channel >= 1 && channel <= 14), ("illegal channel for Zebra: %d\n", channel));
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rRfChannel, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;
		
	case RF_6052:
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, RF_CHNLBW, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_End, 0, 0, 0);		
		break;

	case RF_8258:
		break;
		
	default:
		//RT_ASSERT(FALSE, ("Unknown rf_chip: %d\n", priv->rf_chip));
		return FALSE;
		break;
	}

	
	do{
		switch(*stage)
		{
		case 0:
			CurrentCmd=&PreCommonCmd[*step];
			break;
		case 1:
			CurrentCmd=&RfDependCmd[*step];
			break;
		case 2:
			CurrentCmd=&PostCommonCmd[*step];
			break;
		}
		
		if(CurrentCmd->CmdID==CmdID_End)
		{
			if((*stage)==2)
			{
				return TRUE;
			}
			else
			{
				(*stage)++;
				(*step)=0;
				continue;
			}
		}
		
		switch(CurrentCmd->CmdID)
		{
		case CmdID_SetTxPowerLevel:
			//if(priv->card_8192_version > VERSION_8190_BD)
				rtl8192_phy_setTxPower(dev,channel);
			break;
		case CmdID_WritePortUlong:
			write_nic_dword(dev, CurrentCmd->Para1, CurrentCmd->Para2);
			break;
		case CmdID_WritePortUshort:
			write_nic_word(dev, CurrentCmd->Para1, (u16)CurrentCmd->Para2);
			break;
		case CmdID_WritePortUchar:
			write_nic_byte(dev, CurrentCmd->Para1, (u8)CurrentCmd->Para2);
			break;
		case CmdID_RF_WriteReg:	// Only modify channel for the register now !!!!!
			for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
			{
				// For new T65 RF 0222d register 0x18 bit 0-9 = channel number.
				if (IS_HARDWARE_TYPE_8192SE(dev))
				{
					rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, 0x1f, (CurrentCmd->Para2));
					//printk("====>%x, %x, read_back:%x\n", CurrentCmd->Para2,CurrentCmd->Para1, rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, 0x1f));
				}
				else
					rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, bRFRegOffsetMask, ((CurrentCmd->Para2)<<7));
			}
			break;
                default:
                        break;
		}
		
		break;
	}while(TRUE);
	//cosa }/*for(Number of RF paths)*/

	(*delay)=CurrentCmd->msDelay;
	(*step)++;
	RT_TRACE(COMP_CH, "<===========%s(), channel:%d, stage:%d, step:%d\n", __FUNCTION__, channel, *stage, *step);		
	return FALSE;
}

//    use in phy only 
static bool
phy_SetSwChnlCmdArray(
	SwChnlCmd*		CmdTable,
	u32			CmdTableIdx,
	u32			CmdTableSz,
	SwChnlCmdID		CmdID,
	u32			Para1,
	u32			Para2,
	u32			msDelay
	)
{
	SwChnlCmd* pCmd;

	if(CmdTable == NULL)
	{
		//RT_ASSERT(FALSE, ("phy_SetSwChnlCmdArray(): CmdTable cannot be NULL.\n"));
		return FALSE;
	}
	if(CmdTableIdx >= CmdTableSz)
	{
		//RT_ASSERT(FALSE, 
			//	("phy_SetSwChnlCmdArray(): Access invalid index, please check size of the table, CmdTableIdx:%d, CmdTableSz:%d\n",
				//CmdTableIdx, CmdTableSz));
		return FALSE;
	}

	pCmd = CmdTable + CmdTableIdx;
	pCmd->CmdID = CmdID;
	pCmd->Para1 = Para1;
	pCmd->Para2 = Para2;
	pCmd->msDelay = msDelay;

	return TRUE;
}

//called PHY_SwChnlPhy8192S, SwChnlCallback8192SUsbWorkItem
//    use in phy only 
static void
phy_FinishSwChnlNow(	// We should not call this function directly
	struct net_device* dev,
	u8		channel
		)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u32			delay;
  
	while(!phy_SwChnlStepByStep(dev,channel,&priv->SwChnlStage,&priv->SwChnlStep,&delay))
	{
		if(delay>0)
			mdelay(delay);
		if(!priv->up)
			break;
	}
}

#ifdef TO_DO_LIST
//
//	Description:
//		Configure H/W functionality to enable/disable Monitor mode.
//		Note, because we possibly need to configure BB and RF in this function, 
//		so caller should in PASSIVE_LEVEL. 080118, by rcnjko.
//
extern	VOID
PHY_SetMonitorMode8192S(
	IN	PADAPTER			pAdapter,
	IN	BOOLEAN				bEnableMonitorMode
	)
{
	HAL_DATA_TYPE		*pHalData	= GET_HAL_DATA(pAdapter);

	//2 Note: we may need to stop antenna diversity.
	if(bEnableMonitorMode)
	{
		RT_TRACE(COMP_RM, DBG_LOUD, ("PHY_SetMonitorMode8192S(): enable monitor mode\n"));

		pHalData->bInMonitorMode = TRUE;
		pAdapter->HalFunc.AllowAllDestAddrHandler(pAdapter, TRUE, TRUE);
	}
	else
	{
		RT_TRACE(COMP_RM, DBG_LOUD, ("PHY_SetMonitorMode8192S(): disable monitor mode\n"));

		pAdapter->HalFunc.AllowAllDestAddrHandler(pAdapter, FALSE, TRUE);
		pHalData->bInMonitorMode = FALSE;
	}
}
#endif


/*-----------------------------------------------------------------------------
 * Function:	PHYCheckIsLegalRfPath8190Pci()
 *
 * Overview:	Check different RF type to execute legal judgement. If RF Path is illegal
 *			We will return false.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	11/15/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
 //called by rtl8192_phy_QueryRFReg, rtl8192_phy_SetRFReg, PHY_SetRFPowerState8192SUsb
//extern	bool	
//PHY_CheckIsLegalRfPath8192S	
u8 rtl8192_phy_CheckIsLegalRFPath(struct net_device* dev, u32 eRFPath)
{
//	struct r8192_priv *priv = ieee80211_priv(dev);
	bool				rtValue = TRUE;

	// NOt check RF Path now.!
#if 0	
	if (priv->rf_type == RF_1T2R && eRFPath != RF90_PATH_A)
	{		
		rtValue = FALSE;
	}
	if (priv->rf_type == RF_1T2R && eRFPath != RF90_PATH_A)
	{

	}
#endif
	return	rtValue;

}	/* PHY_CheckIsLegalRfPath8192S */



/*-----------------------------------------------------------------------------
 * Function:	PHY_IQCalibrate8192S()
 *
 * Overview:	After all MAC/PHY/RF is configued. We must execute IQ calibration 
 *			to improve RF EVM!!?
 *
 * Input:		IN	PADAPTER	pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	10/07/2008	MHC		Create. Document from SD3 RFSI Jenyu.  
 *
 *---------------------------------------------------------------------------*/
 //called by InitializeAdapter8192SE
void	
PHY_IQCalibrate(	struct net_device* dev)
{
	//struct r8192_priv 	*priv = ieee80211_priv(dev);
	u32				i, reg;
	u32				old_value;
	long				X, Y, TX0[4];
	u32				TXA[4];
	
	// 1. Check QFN68 or 64 92S (Read from EEPROM)

	//
	// 2. QFN 68	
	//
	// For 1T2R IQK only now !!!
	for (i = 0; i < 10; i++)
	{
		// IQK 
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05430);
		//PlatformStallExecution(5);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000800e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x80800000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe40, bMaskDWord, 0x02140148);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe44, bMaskDWord, 0x681604a2);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe4c, bMaskDWord, 0x000028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe60, bMaskDWord, 0x0214014d);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe64, bMaskDWord, 0x281608ba);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe6c, bMaskDWord, 0x000028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000001);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000001);
		udelay(2000);
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05433);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000000e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x0);		

	
		reg = rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord);

		// Readback IQK value and rewrite
		if (!(reg&(BIT27|BIT28|BIT30|BIT31)))
		{
			old_value = (rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord) & 0x3FF);

			// Calibrate init gain for A path for TX0
			X = (rtl8192_QueryBBReg(dev, 0xe94, bMaskDWord) & 0x03FF0000)>>16;
			TXA[RF90_PATH_A] = (X * old_value)/0x100;
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xFFFFFC00) | (u32)TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			udelay(5);

			// Calibrate init gain for C path for TX0
			Y = ( rtl8192_QueryBBReg(dev, 0xe9C, bMaskDWord) & 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xffc0ffff) |((u32) (TX0[RF90_PATH_C]&0x3F)<<16);			
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc94, bMaskDWord);		
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc94, bMaskDWord, reg);
			udelay(5);

			// Calibrate RX A and B for RX0
			reg = rtl8192_QueryBBReg(dev, 0xc14, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xea4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			Y = (rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			udelay(5);
			old_value = (rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord) & 0x3FF);
			
			// Calibrate init gain for A path for TX1 !!!!!!
			X = (rtl8192_QueryBBReg(dev, 0xeb4, bMaskDWord) & 0x03FF0000)>>16;
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			TXA[RF90_PATH_A] = (X * old_value) / 0x100;
			reg = (reg & 0xFFFFFC00) | TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			udelay(5);

			// Calibrate init gain for C path for TX1
			Y = (rtl8192_QueryBBReg(dev, 0xebc, bMaskDWord)& 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			reg = (reg & 0xffc0ffff) |( (TX0[RF90_PATH_C]&0x3F)<<16);
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc9c, bMaskDWord);
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc9c, bMaskDWord, reg);
			udelay(5);

			// Calibrate RX A and B for RX1
			reg = rtl8192_QueryBBReg(dev, 0xc1c, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xec4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			
			Y = (rtl8192_QueryBBReg(dev, 0xecc, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			udelay(5);
			
			RT_TRACE(COMP_INIT, "PHY_IQCalibrate OK\n");
			break;
		}

	}
	
	
	//
	// 3. QFN64. Not enabled now !!! We must use different gain table for 1T2R.
	//


}

/*-----------------------------------------------------------------------------
 * Function:	PHY_IQCalibrateBcut()
 *
 * Overview:	After all MAC/PHY/RF is configued. We must execute IQ calibration 
 *			to improve RF EVM!!?
 *
 * Input:		IN	PADAPTER	pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	11/18/2008	MHC		Create. Document from SD3 RFSI Jenyu. 
 *						92S B-cut QFN 68 pin IQ calibration procedure.doc
 *
 *---------------------------------------------------------------------------*/
extern void PHY_IQCalibrateBcut(struct net_device* dev)
{
	//struct r8192_priv 	*priv = ieee80211_priv(dev);
	//PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	u32				i, reg;
	u32				old_value;
	long				X, Y, TX0[4];
	u32				TXA[4];	
	u32				calibrate_set[13] = {0};
	u32				load_value[13];
	u8			RfPiEnable=0;
	
	// 0. Check QFN68 or 64 92S (Read from EEPROM/EFUSE)
//	return;
	//
	// 1. Save e70~ee0 register setting, and load calibration setting
	// 
	/*
	0xee0[31:0]=0x3fed92fb;   
	0xedc[31:0] =0x3fed92fb;   
	0xe70[31:0] =0x3fed92fb;   
	0xe74[31:0] =0x3fed92fb;   
	0xe78[31:0] =0x3fed92fb;   
	0xe7c[31:0]= 0x3fed92fb;   
	0xe80[31:0]= 0x3fed92fb;   
	0xe84[31:0]= 0x3fed92fb; 
	0xe88[31:0]= 0x3fed92fb;  
	0xe8c[31:0]= 0x3fed92fb;   
	0xed0[31:0]= 0x3fed92fb;   
	0xed4[31:0]= 0x3fed92fb;  
	0xed8[31:0]= 0x3fed92fb;
	*/
	calibrate_set [0] = 0xee0;
	calibrate_set [1] = 0xedc;
	calibrate_set [2] = 0xe70;
	calibrate_set [3] = 0xe74;
	calibrate_set [4] = 0xe78;
	calibrate_set [5] = 0xe7c;
	calibrate_set [6] = 0xe80;
	calibrate_set [7] = 0xe84;
	calibrate_set [8] = 0xe88;
	calibrate_set [9] = 0xe8c;
	calibrate_set [10] = 0xed0;
	calibrate_set [11] = 0xed4;
	calibrate_set [12] = 0xed8;
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Save e70~ee0 register setting\n"));	
	for (i = 0; i < 13; i++)
	{
		load_value[i] = rtl8192_QueryBBReg(dev, calibrate_set[i], bMaskDWord);
		rtl8192_setBBreg(dev, calibrate_set[i], bMaskDWord, 0x3fed92fb);		
		
	}
	
	RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter1, BIT8);
	//
	// 2. QFN 68	
	//
	// For 1T2R IQK only now !!!
	for (i = 0; i < 10; i++)
	{
		RT_TRACE(COMP_INIT, "IQK -%d\n", i);	
		//BB switch to PI mode. If default is PI mode, ignoring 2 commands below.
		if (!RfPiEnable)	//if original is SI mode, then switch to PI mode.
		{
			rtl8192_setBBreg(dev, 0x820, bMaskDWord, 0x01000100);
			rtl8192_setBBreg(dev, 0x828, bMaskDWord, 0x01000100);
		}
	
		// IQK 
		// 2. IQ calibration & LO leakage calibration
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05430);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000800e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x80800000);
		udelay(5);
		//path-A IQ K and LO K gain setting
		rtl8192_setBBreg(dev, 0xe40, bMaskDWord, 0x02140102);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe44, bMaskDWord, 0x681604c2);
		udelay(5);
		//set LO calibration
		rtl8192_setBBreg(dev, 0xe4c, bMaskDWord, 0x000028d1);
		udelay(5);
		//path-B IQ K and LO K gain setting
		rtl8192_setBBreg(dev, 0xe60, bMaskDWord, 0x02140102);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe64, bMaskDWord, 0x28160d05);
		udelay(5);
		//K idac_I & IQ
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000000);
		udelay(5);

		// delay 2ms
		udelay(2000);

		//idac_Q setting
		rtl8192_setBBreg(dev, 0xe6c, bMaskDWord, 0x020028d1);
		udelay(5);
		//K idac_Q & IQ
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000000);

		// delay 2ms
		udelay(2000);

		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05433);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000000e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x0);		

	//	if (priv->bRFSiOrPi == 0)	// SI
		{
			rtl8192_setBBreg(dev, 0x820, bMaskDWord, 0x01000000);
			rtl8192_setBBreg(dev, 0x828, bMaskDWord, 0x01000000);
		}

	
		reg = rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord);

		// 3.	check fail bit, and fill BB IQ matrix
		// Readback IQK value and rewrite
		if (!(reg&(BIT27|BIT28|BIT30|BIT31)))
		{			
			old_value = (rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord) & 0x3FF);

			// Calibrate init gain for A path for TX0
			X = (rtl8192_QueryBBReg(dev, 0xe94, bMaskDWord) & 0x03FF0000)>>16;
			TXA[RF90_PATH_A] = (X * old_value)/0x100;
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xFFFFFC00) | (u32)TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			udelay(5);

			// Calibrate init gain for C path for TX0
			Y = ( rtl8192_QueryBBReg(dev, 0xe9C, bMaskDWord) & 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xffc0ffff) |((u32) (TX0[RF90_PATH_C]&0x3F)<<16);			
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc94, bMaskDWord);		
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc94, bMaskDWord, reg);
			udelay(5);

			// Calibrate RX A and B for RX0
			reg = rtl8192_QueryBBReg(dev, 0xc14, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xea4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			Y = (rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			udelay(5);
			old_value = (rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord) & 0x3FF);
			
			// Calibrate init gain for A path for TX1 !!!!!!
			X = (rtl8192_QueryBBReg(dev, 0xeb4, bMaskDWord) & 0x03FF0000)>>16;
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			TXA[RF90_PATH_A] = (X * old_value) / 0x100;
			reg = (reg & 0xFFFFFC00) | TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			udelay(5);

			// Calibrate init gain for C path for TX1
			Y = (rtl8192_QueryBBReg(dev, 0xebc, bMaskDWord)& 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			reg = (reg & 0xffc0ffff) |( (TX0[RF90_PATH_C]&0x3F)<<16);
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc9c, bMaskDWord);
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc9c, bMaskDWord, reg);
			udelay(5);

			// Calibrate RX A and B for RX1
			reg = rtl8192_QueryBBReg(dev, 0xc1c, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xec4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			
			Y = (rtl8192_QueryBBReg(dev, 0xecc, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			udelay(5);
			
			RT_TRACE(COMP_INIT, "PHY_IQCalibrate OK\n");
			break;
		}

	}
	
	//
	// 4. Reload e70~ee0 register setting.
	//
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reload e70~ee0 register setting.\n"));	
	for (i = 0; i < 13; i++)
		rtl8192_setBBreg(dev, calibrate_set[i], bMaskDWord, load_value[i]);
	
	
	//
	// 3. QFN64. Not enabled now !!! We must use different gain table for 1T2R.
	//
	


}	// PHY_IQCalibrateBcut

//-----------------------------------------------------------------------------
//	Description:
//		Schedule workitem to send specific CMD IO to FW.
//	Added by Roger, 2008.12.03.
//		
//-----------------------------------------------------------------------------
bool rtl8192se_set_fw_cmd(struct net_device* dev, FW_CMD_IO_TYPE		FwCmdIO)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
//	u2Byte	FwCmdWaitCounter = 0;

	RT_TRACE(COMP_CMD, "-->HalSetFwCmd8192S(): Set FW Cmd(%#x), SetFwCmdInProgress(%d)\n", FwCmdIO, priv->SetFwCmdInProgress);

	// Will be done by high power respectively.
	if(FwCmdIO==FW_CMD_DIG_HALT || FwCmdIO==FW_CMD_DIG_RESUME)
	{
		RT_TRACE(COMP_CMD, "<--HalSetFwCmd8192S(): Set FW Cmd(%#x)\n", FwCmdIO);
		return false;
	}

#if 0
	while(priv->SetFwCmdInProgress && FwCmdWaitCounter<10000)
	{
		FwCmdWaitCounter ++;
	//	RT_TRACE(COMP_CMD, DBG_LOUD, ("HalSetFwCmd8192S(): Wait 10 ms (%d times)...\n", FwCmdWaitCounter));
		udelay(100);
	}

	if(FwCmdWaitCounter == 10000)
	{
		//RT_ASSERT(FALSE, ("SetFwCmdIOWorkItemCallback(): Wait too logn to set FW CMD\n"));
		RT_TRACE(COMP_CMD, "HalSetFwCmd8192S(): Wait too logn to set FW CMD\n");
		return FALSE;
	}
#endif
	if (priv->SetFwCmdInProgress)
	{
		RT_TRACE(COMP_ERR, "<--HalSetFwCmd8192S(): Set FW Cmd(%#x)\n", FwCmdIO);
		return false;
	}
	priv->SetFwCmdInProgress = TRUE;
	priv->CurrentFwCmdIO = FwCmdIO; // Update current FW Cmd for callback use.
#if 0
#ifdef USE_WORKITEM			
	PlatformScheduleWorkItem(&(pHalData->FwCmdIOWorkItem));
#else
	PlatformSetTimer(Adapter, &(pHalData->SetFwCmdIOTimer), 0);
#endif
#endif
	rtl8192_SetFwCmdIOCallback(dev);
	return true;
}
void ChkFwCmdIoDone(struct net_device* dev)
{
	u16 PollingCnt = 10000;		
	u32 tmpValue;
	
	do
	{// Make sure that CMD IO has be accepted by FW.		
		//delay_us(20);
	//	usleep(10); // sleep 20us
		tmpValue = read_nic_dword(dev, WFM5);
		if(tmpValue == 0)
		{
			RT_TRACE(COMP_CMD, "[FW CMD] Set FW Cmd success!!\n");
			break;
		}			
		else
		{
			RT_TRACE(COMP_CMD, "[FW CMD] Polling FW Cmd PollingCnt(%d)!!\n", PollingCnt);
		}
	}while( --PollingCnt );

	if(PollingCnt == 0)
	{
		RT_TRACE(COMP_ERR, "[FW CMD] Set FW Cmd fail!!\n");
	}
}
// 	Callback routine of the timer callback for FW Cmd IO.
//
//	Description:
//		This routine will send specific CMD IO to FW and check whether it is done.
//
void rtl8192_SetFwCmdIOCallback(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32		input,CurrentAID = 0;
	//static u32	u4bTmp;
	
	if(!priv->up)
	{
		RT_TRACE(COMP_CMD, "SetFwCmdIOTimerCallback(): driver is going to unload\n");
		return;
	}
	
	RT_TRACE(COMP_CMD, "--->SetFwCmdIOTimerCallback(): Cmd(%#x), SetFwCmdInProgress(%d)\n", priv->CurrentFwCmdIO, priv->SetFwCmdInProgress);
#if 1
	switch(priv->CurrentFwCmdIO)
	{			
		//
		// <Roger_Notes> The following FW CMD IO was combined into single operation
		// (i.e., to prevent number of system workitem out of resource!!).
		// 2008.12.04.
		//
		case FW_CMD_HIGH_PWR_ENABLE:
		case FW_CMD_DIG_RESUME:
			RT_TRACE(COMP_CMD, "[FW CMD] Set HIGHPWR enable and DIG resume!!\n");
			write_nic_dword(dev, WFM5, FW_HIGH_PWR_ENABLE); //break;
			ChkFwCmdIoDone(dev);
			write_nic_dword(dev, WFM5, FW_DIG_RESUME); 
			break;

		case FW_CMD_HIGH_PWR_DISABLE:
		case FW_CMD_DIG_HALT:
			RT_TRACE(COMP_CMD, "[FW CMD] Set HIGHPWR disable and DIG halt!!\n");
			write_nic_dword(dev, WFM5, FW_HIGH_PWR_DISABLE); //break;
			ChkFwCmdIoDone(dev);
			write_nic_dword(dev, WFM5, FW_DIG_HALT); 
			break;

		//
		// <Roger_Notes> The following FW CMD IO should be checked
		// (i.e., workitem schedule timing issue!!).
		// 2008.12.04.
		//
		case FW_CMD_DIG_DISABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] Set DIG disable!!\n");
			write_nic_dword(dev, WFM5, FW_DIG_DISABLE); 
			break;
			
		case FW_CMD_DIG_ENABLE:
			RT_TRACE(COMP_CMD,  "[FW CMD] Set DIG enable!!\n");
			write_nic_dword(dev, WFM5, FW_DIG_ENABLE); 
			break;

		case FW_CMD_RA_RESET:
			write_nic_dword(dev, WFM5, FW_RA_RESET);	
			break;
			
		case FW_CMD_RA_ACTIVE:
			write_nic_dword(dev, WFM5, FW_RA_ACTIVE); 
			break;

		case FW_CMD_RA_REFRESH_N:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA refresh!!,N\n");
			write_nic_dword(dev, WFM5, FW_RA_REFRESH); 
			printk("===>refresh_N\n");
			break;
		case FW_CMD_RA_REFRESH_BG:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA refresh, BG!!\n");
			write_nic_dword(dev, WFM5, FW_RA_REFRESH); 
			ChkFwCmdIoDone(dev);
			write_nic_dword(dev, WFM5, FW_RA_ENABLE_BG);
			break;


		case FW_CMD_IQK_ENABLE:
			write_nic_dword(dev, WFM5, FW_IQK_ENABLE); 
			break;

		case FW_CMD_MID_HIGH_PWR_ENABLE:
			input = FW_HIGH_PWR_ENABLE | (priv->MidHighPwrTHR_L1<<8) | (priv->MidHighPwrTHR_L2<<16);
			RT_TRACE(COMP_CMD,"[FW CMD] Set MIDHIGHPWR(%x) enable and DIG resume!!\n", input);
			write_nic_dword(dev, WFM5, input); //break;
			ChkFwCmdIoDone(dev);
			write_nic_dword(dev, WFM5, FW_DIG_RESUME); 
			break;

		case FW_CMD_LPS_ENTER:
			RT_TRACE(COMP_CMD, "[FW CMD] Enter LPS mode\n");
			CurrentAID = priv->ieee80211->assoc_id;
			write_nic_dword(dev, WFM5, (FW_LPS_ENTER| ((CurrentAID|0xc000)<<8))    );
			// FW set TXOP disable here, so disable EDCA turbo mode until driver leave LPS
			priv->ieee80211->pHTInfo->IOTAction |=  HT_IOT_ACT_DISABLE_EDCA_TURBO;
			break;

		case FW_CMD_LPS_LEAVE:
			RT_TRACE(COMP_CMD, "[FW CMD] Leave LPS mode\n");
			write_nic_dword(dev, WFM5, FW_LPS_LEAVE );
			priv->ieee80211->pHTInfo->IOTAction &=  (~HT_IOT_ACT_DISABLE_EDCA_TURBO);
			break;

		default:
			RT_TRACE(COMP_ERR, "Unknown FW Cmd IO(%#x)\n", priv->CurrentFwCmdIO);
			break;
	}
		
	ChkFwCmdIoDone(dev);		
	
	switch(priv->CurrentFwCmdIO)
	{	
		case FW_CMD_HIGH_PWR_DISABLE:
			//if(pMgntInfo->bTurboScan)
			{
				//Lower initial gain
				rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, bMaskByte0, 0x17);
				rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, bMaskByte0, 0x17);
				// CCA threshold
				rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x40);
				// Disable OFDM Part
				//YJ,del,090323, sychronized with windows
				/*
				rtl8192_setBBreg(dev, rOFDM0_TRMuxPar, bMaskByte2, 0x1);
				u4bTmp = rtl8192_QueryBBReg(dev, rOFDM0_RxDetector1,bMaskDWord);
				rtl8192_setBBreg(dev, rOFDM0_RxDetector1, 0xf, 0xf);
				rtl8192_setBBreg(dev, rOFDM1_TRxPathEnable, 0xf, 0x0);
				*/
			}
			break;
		case FW_CMD_MID_HIGH_PWR_ENABLE:
		case FW_CMD_HIGH_PWR_ENABLE:
			//if(pMgntInfo->bTurboScan)
			{
				// CCA threshold
				rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0xcd);	 // 2009.03.03 change from 83 to cd
				// Enable OFDM Part
				//YJ,del,090323, sychronized with windows
				/*
				rtl8192_setBBreg(dev, rOFDM0_TRMuxPar, bMaskByte2, 0x0);
				rtl8192_setBBreg(dev, rOFDM0_RxDetector1, bMaskDWord, u4bTmp);
				if(priv->rf_type == RF_1T2R || priv->rf_type == RF_2T2R)					
					rtl8192_setBBreg(dev, rOFDM1_TRxPathEnable, 0xf, 0x3);
				else
					rtl8192_setBBreg(dev, rOFDM1_TRxPathEnable, 0xf, 0x1);
				*/
			}
			break;
	}
	priv->SetFwCmdInProgress = FALSE;// Clear FW CMD operation flag.
	RT_TRACE(COMP_CMD, "<---SetFwCmdIOWorkItemCallback()\n");
#endif
}

//
// 2008/12/26 MH When we switch EPHY parameter. We must delay to wait ready bit.
// We wait at most 1000*100 us.
//
static	void
phy_CheckEphySwitchReady(struct net_device* dev)
{
	u32	delay = 1000;
	u8	regu1;

	regu1 = read_nic_byte(dev, 0x554);
	while ((regu1 & BIT5) && (delay > 0))
	{
		regu1 = read_nic_byte(dev, 0x554);
		delay--;
		udelay(100);
	}	
	RT_TRACE(COMP_INIT, "regu1=%02x  delay = %d\n", regu1, delay);	
	
}	// phy_CheckEphySwitchReady

void
MgntDisconnectIBSS(
	struct net_device* dev
) 
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//RT_OP_MODE	OpMode;
	u8			i;
	bool	bFilterOutNonAssociatedBSSID = false;

	//IEEE80211_DEBUG(IEEE80211_DL_TRACE, "XXXXXXXXXX MgntDisconnect IBSS\n");

	priv->ieee80211->state = IEEE80211_NOLINK;

//	PlatformZeroMemory( pMgntInfo->Bssid, 6 );
	for(i=0;i<6;i++)  priv->ieee80211->current_network.bssid[i]= 0x55;
	priv->OpMode = RT_OP_MODE_NO_LINK;
	write_nic_word(dev, BSSIDR, ((u16*)priv->ieee80211->current_network.bssid)[0]);
	write_nic_dword(dev, BSSIDR+2, ((u32*)(priv->ieee80211->current_network.bssid+2))[0]);
	{
			RT_OP_MODE	OpMode = priv->OpMode;
			//LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
			u8	btMsr = read_nic_byte(dev, MSR);

			btMsr &= 0xfc;

			switch(OpMode)
			{
			case RT_OP_MODE_INFRASTRUCTURE:
				btMsr |= MSR_LINK_MANAGED;
				//LedAction = LED_CTL_LINK;
				break;

			case RT_OP_MODE_IBSS:
				btMsr |= MSR_LINK_ADHOC;
				// led link set seperate
				break;

			case RT_OP_MODE_AP:
				btMsr |= MSR_LINK_MASTER;
				//LedAction = LED_CTL_LINK;
				break;

			default:
				btMsr |= MSR_LINK_NONE;
				break;
			}

			write_nic_byte(dev, MSR, btMsr);

			// LED control
			//dev->HalFunc.LedControlHandler(dev, LedAction);
	}
	ieee80211_stop_send_beacons(priv->ieee80211);
	
	// If disconnect, clear RCR CBSSID bit
	bFilterOutNonAssociatedBSSID = false;
	{
			u32 RegRCR, Type;
			Type = bFilterOutNonAssociatedBSSID;
			RegRCR = read_nic_dword(dev,RCR);
			priv->ReceiveConfig = RegRCR;
			if (Type == true)
				RegRCR |= (RCR_CBSSID);
			else if (Type == false)
				RegRCR &= (~RCR_CBSSID);

			{
				write_nic_dword(dev, RCR,RegRCR);
				priv->ReceiveConfig = RegRCR;
			}
			
		}
	//MgntIndicateMediaStatus( dev, RT_MEDIA_DISCONNECT, GENERAL_INDICATE );
	notify_wx_assoc_event(priv->ieee80211);

}

void
MlmeDisassociateRequest(
	struct net_device* dev,
	u8* 		asSta,
	u8		asRsn
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 i;

	RemovePeerTS(priv->ieee80211, asSta);
	
	//SendDisassociation( priv->ieee80211, 0, asRsn );
	
	if(memcpy(priv->ieee80211->current_network.bssid,asSta,6) == 0)
	{
		//ShuChen TODO: change media status.
		//ShuChen TODO: What to do when disassociate.
		priv->ieee80211->state = IEEE80211_NOLINK;
		//pMgntInfo->AsocTimestamp = 0;
		for(i=0;i<6;i++)  priv->ieee80211->current_network.bssid[i] = 0x22;
//		pMgntInfo->mBrates.Length = 0;
//		dev->HalFunc.SetHwRegHandler( dev, HW_VAR_BASIC_RATE, (pu1Byte)(&pMgntInfo->mBrates) );
		priv->OpMode = RT_OP_MODE_NO_LINK;
		{
			RT_OP_MODE	OpMode = priv->OpMode;
			//LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
			u8 btMsr = read_nic_byte(dev, MSR);

			btMsr &= 0xfc;

			switch(OpMode)
			{
			case RT_OP_MODE_INFRASTRUCTURE:
				btMsr |= MSR_LINK_MANAGED;
				//LedAction = LED_CTL_LINK;
				break;

			case RT_OP_MODE_IBSS:
				btMsr |= MSR_LINK_ADHOC;
				// led link set seperate
				break;

			case RT_OP_MODE_AP:
				btMsr |= MSR_LINK_MASTER;
				//LedAction = LED_CTL_LINK;
				break;

			default:
				btMsr |= MSR_LINK_NONE;
				break;
			}

			write_nic_byte(dev, MSR, btMsr);

			// LED control
			//dev->HalFunc.LedControlHandler(dev, LedAction);
		}
		ieee80211_disassociate(priv->ieee80211);

		write_nic_word(dev, BSSIDR, ((u16*)priv->ieee80211->current_network.bssid)[0]);
		write_nic_dword(dev, BSSIDR+2, ((u32*)(priv->ieee80211->current_network.bssid+2))[0]);
		
	}

}

void
MgntDisconnectAP(
	struct net_device* dev,
	u8 asRsn
) 
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool bFilterOutNonAssociatedBSSID = false;

//
// Commented out by rcnjko, 2005.01.27: 
// I move SecClearAllKeys() to MgntActSet_802_11_DISASSOCIATE().
//
//	//2004/09/15, kcwu, the key should be cleared, or the new handshaking will not success
//	SecClearAllKeys(dev);

	// In WPA WPA2 need to Clear all key ... because new key will set after new handshaking.
#ifdef TO_DO
	if(   pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch ||
		(pMgntInfo->bAPSuportCCKM && pMgntInfo->bCCX8021xenable) )	// In CCKM mode will Clear key
	{
		SecClearAllKeys(dev);
		RT_TRACE(COMP_SEC, DBG_LOUD,("======>CCKM clear key..."))
	}
#endif
	// If disconnect, clear RCR CBSSID bit
	bFilterOutNonAssociatedBSSID = false;
	{
			u32 RegRCR, Type;

			Type = bFilterOutNonAssociatedBSSID;
			//dev->HalFunc.GetHwRegHandler(dev, HW_VAR_RCR, (pu1Byte)(&RegRCR));
			RegRCR = read_nic_dword(dev,RCR);
			priv->ReceiveConfig = RegRCR;
			
			if (Type == true)
				RegRCR |= (RCR_CBSSID);
			else if (Type == false)
				RegRCR &= (~RCR_CBSSID);

			write_nic_dword(dev, RCR,RegRCR);
			priv->ReceiveConfig = RegRCR;
			
			
	}
	// 2004.10.11, by rcnjko.
	//MlmeDisassociateRequest( dev, pMgntInfo->Bssid, disas_lv_ss );
	MlmeDisassociateRequest( dev, priv->ieee80211->current_network.bssid, asRsn );

	priv->ieee80211->state = IEEE80211_NOLINK;
	//pMgntInfo->AsocTimestamp = 0;
}

bool
MgntDisconnect(
	struct net_device* dev,
	u8 asRsn
) 
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	//
	// Schedule an workitem to wake up for ps mode, 070109, by rcnjko.
	//
#ifdef TO_DO
	if(pMgntInfo->mPss != eAwake)
	{
		// 
		// Using AwkaeTimer to prevent mismatch ps state.
		// In the timer the state will be changed according to the RF is being awoke or not. By Bruce, 2007-10-31. 
		//
		// PlatformScheduleWorkItem( &(pMgntInfo->AwakeWorkItem) );
		PlatformSetTimer( dev, &(pMgntInfo->AwakeTimer), 0 );
	}
#endif
	// Follow 8180 AP mode, 2005.05.30, by rcnjko.
#ifdef TO_DO
	if(pMgntInfo->mActingAsAp)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntDisconnect() ===> AP_DisassociateAllStation\n"));
		AP_DisassociateAllStation(dev, unspec_reason);
		return TRUE;
	}	
#endif
	// Indication of disassociation event.
	//DrvIFIndicateDisassociation(dev, asRsn);

	// In adhoc mode, update beacon frame.
	if( priv->ieee80211->state == IEEE80211_LINKED )
	{
		if( priv->ieee80211->iw_mode == IW_MODE_ADHOC )
		{
			//RT_TRACE(COMP_MLME, "MgntDisconnect() ===> MgntDisconnectIBSS\n");
			MgntDisconnectIBSS(dev);
		}
		if( priv->ieee80211->iw_mode == IW_MODE_INFRA )
		{
			// We clear key here instead of MgntDisconnectAP() because that  
			// MgntActSet_802_11_DISASSOCIATE() is an interface called by OS, 
			// e.g. OID_802_11_DISASSOCIATE in Windows while as MgntDisconnectAP() is 
			// used to handle disassociation related things to AP, e.g. send Disassoc
			// frame to AP.  2005.01.27, by rcnjko.
			//IEEE80211_DEBUG(IEEE80211_DL_TRACE,"MgntDisconnect() ===> MgntDisconnectAP\n");
			MgntDisconnectAP(dev, asRsn);
		}

		// Inidicate Disconnect, 2005.02.23, by rcnjko.
		//MgntIndicateMediaStatus( dev, RT_MEDIA_DISCONNECT, GENERAL_INDICATE); 
	}

	return true;
}
//
//
//	Description: 
//		Chang RF Power State.
//		Note that, only MgntActSet_RF_State() is allowed to set HW_VAR_RF_STATE.
//
//	Assumption:
//		PASSIVE LEVEL.
//
bool
MgntActSet_RF_State(
	struct net_device* dev,
	RT_RF_POWER_STATE	StateToSet,
	RT_RF_CHANGE_SOURCE ChangeSource
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool 			bActionAllowed = false; 
	bool 			bConnectBySSID = false;
	RT_RF_POWER_STATE	rtState;
	u16					RFWaitCounter = 0;
	unsigned long flag;
	RT_TRACE((COMP_PS && COMP_RF), "===>MgntActSet_RF_State(): StateToSet(%d)\n",StateToSet);

	//1//
	//1//<1>Prevent the race condition of RF state change. 
	//1//
	// Only one thread can change the RF state at one time, and others should wait to be executed. By Bruce, 2007-11-28.

	while(true)
	{
		spin_lock_irqsave(&priv->rf_ps_lock,flag);
		if(priv->RFChangeInProgress)
		{
			spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
			RT_TRACE((COMP_PS && COMP_RF), "MgntActSet_RF_State(): RF Change in progress! Wait to set..StateToSet(%d).\n", StateToSet);
			printk("MgntActSet_RF_State(): RF Change in progress! Wait to set..StateToSet(%d).\n", StateToSet);
			#if 0	
			// Set RF after the previous action is done. 
			while(priv->RFChangeInProgress)
			{
				RFWaitCounter ++;
				RT_TRACE((COMP_PS && COMP_RF), "MgntActSet_RF_State(): Wait 1 ms (%d times)...\n", RFWaitCounter);
				udelay(1000); // 1 ms

				// Wait too long, return FALSE to avoid to be stuck here.
				if(RFWaitCounter > 100)
				{
					RT_TRACE(COMP_ERR, "MgntActSet_RF_State(): Wait too logn to set RF\n");
					// TODO: Reset RF state?
					return false;
				}
			}
			#endif
			return false;
		}
		else
		{
			priv->RFChangeInProgress = true;
			spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
			break;
		}
	}

	rtState = priv->ieee80211->eRFPowerState;

	switch(StateToSet) 
	{
	case eRfOn:
		//
		// Turn On RF no matter the IPS setting because we need to update the RF state to Ndis under Vista, or
		// the Windows does not allow the driver to perform site survey any more. By Bruce, 2007-10-02. 
		//

		priv->ieee80211->RfOffReason &= (~ChangeSource);

		if((ChangeSource == RF_CHANGE_BY_HW) && (priv->bHwRadioOff == true)){
			priv->bHwRadioOff = false;
		}

		if(! priv->ieee80211->RfOffReason)
		{
			priv->ieee80211->RfOffReason = 0;
			bActionAllowed = true;

			
			if(rtState == eRfOff && ChangeSource >=RF_CHANGE_BY_HW )
			{
				bConnectBySSID = true;
			}
		}
		else{
			RT_TRACE((COMP_PS && COMP_RF), "MgntActSet_RF_State - eRfon reject pMgntInfo->RfOffReason= 0x%x, ChangeSource=0x%X\n", priv->ieee80211->RfOffReason, ChangeSource);
                }

		break;

	case eRfOff:

		if(priv->ieee80211->iw_mode != IW_MODE_MASTER)
		{
			if (priv->ieee80211->RfOffReason > RF_CHANGE_BY_IPS)
			{
				//
				// 060808, Annie: 
				// Disconnect to current BSS when radio off. Asked by QuanTa.
				//
				// Set all link status falg, by Bruce, 2007-06-26.
				//MgntActSet_802_11_DISASSOCIATE( dev, disas_lv_ss );			
				MgntDisconnect(dev, disas_lv_ss);
	
	
				// Clear content of bssDesc[] and bssDesc4Query[] to avoid reporting old bss to UI. 
				// 2007.05.28, by shien chang.
				
			}
		}
		if((ChangeSource == RF_CHANGE_BY_HW) && (priv->bHwRadioOff == false)){
			priv->bHwRadioOff = true;
		}
		priv->ieee80211->RfOffReason |= ChangeSource;
		bActionAllowed = true;
		break;

	case eRfSleep:
		priv->ieee80211->RfOffReason |= ChangeSource;
		bActionAllowed = true;
		break;

	default:
		break;
	}

	if(bActionAllowed)
	{
		RT_TRACE((COMP_PS && COMP_RF), "MgntActSet_RF_State(): Action is allowed.... StateToSet(%d), RfOffReason(%#X)\n", StateToSet, priv->ieee80211->RfOffReason);
				// Config HW to the specified mode.
		PHY_SetRFPowerState(dev, StateToSet);	
		// Turn on RF.
		if(StateToSet == eRfOn) 
		{				
			//dev->HalFunc.HalEnableRxHandler(dev);
			if(bConnectBySSID)
			{				
				//MgntActSet_802_11_SSID(dev, dev->MgntInfo.Ssid.Octet, dev->MgntInfo.Ssid.Length, TRUE );
			}
		}
		// Turn off RF.
		else if(StateToSet == eRfOff)
		{		
			//dev->HalFunc.HalDisableRxHandler(dev);
		}		
	}
	else
	{
		RT_TRACE((COMP_PS && COMP_RF), "MgntActSet_RF_State(): Action is rejected.... StateToSet(%d), ChangeSource(%#X), RfOffReason(%#X)\n", StateToSet, ChangeSource, priv->ieee80211->RfOffReason);
	}

	// Release RF spinlock
	spin_lock_irqsave(&priv->rf_ps_lock,flag);
	priv->RFChangeInProgress = false;
	spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
	
	RT_TRACE((COMP_PS && COMP_RF), "<===MgntActSet_RF_State()\n");
	return bActionAllowed;
}
#ifdef TO_DO_LIST
/*-----------------------------------------------------------------------------
 * Function:	GPIOChangeRFWorkItemCallBack()
 *
 * Overview:	Trun on/off RF according to GPIO1. In register 0x108 bit 1.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	12/25/2008	MHC		When we disable all clock tree for 92SE, OS will think that we are
 *						supprise remove. We have to do HW radio GPIO check in the
 *						header of mpcheckforhang..
 *
 *---------------------------------------------------------------------------*/
void
HW_RadioGpioChk92SE(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u1Byte				u1Tmp = 0;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(pAdapter);
	RT_RF_POWER_STATE	eRfPowerStateToSet;
	BOOLEAN				bActuallySet = FALSE;

	// Marked by Bruce, 2008-12-29.
	// Fix it if the GPIO has been ready.
#if 0
	if (!RT_IN_PS_LEVEL(pAdapter, RT_RF_OFF_LEVL_PCI_D3) &&
		pMgntInfo->RfOffReason != RF_CHANGE_BY_HW)
	{
		return;
	}
	
		// Permit to do Any IO
		// Disable Clock request
		PlatformSwitchClkReq(pAdapter, 0x00);

	if (RT_IN_PS_LEVEL(pAdapter, RT_RF_OFF_LEVL_PCI_D3))
	{		
		// Exit D3 mode
		RT_LEAVE_D3(pAdapter, FALSE);
		RT_CLEAR_PS_LEVEL(pAdapter, RT_RF_OFF_LEVL_PCI_D3);
		Power_DomainInit92SE(pAdapter);
	}

	// Advised from Jong SD1. Using GPIO 3.
	// Added by Bruce, 2008-12-01.
	PlatformEFIOWrite1Byte(pAdapter, MAC_PINMUX_CFG, (GPIOMUX_EN | GPIOSEL_GPIO));

	u1Tmp = PlatformEFIORead1Byte(pAdapter, GPIO_IO_SEL);
	u1Tmp &= HAL_8192S_HW_GPIO_OFF_MASK;
	PlatformEFIOWrite1Byte(pAdapter, GPIO_IO_SEL, u1Tmp);

	RT_TRACE(COMP_CMD, DBG_LOUD, 
	("HW_RadioGpioChk92SE HW_RadioGpioChk92SE=%02x\n", HW_RadioGpioChk92SE));
	
	u1Tmp = PlatformEFIORead1Byte(pAdapter, GPIO_IN);

	eRfPowerStateToSet = (u1Tmp & HAL_8192S_HW_GPIO_OFF_BIT) ? eRfOn : eRfOff;

	if( (pHalData->bHwRadioOff == TRUE) && (eRfPowerStateToSet == eRfOn))
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("HW_RadioGpioChk92SE  - HW Radio ON\n"));
		pHalData->bHwRadioOff = FALSE;
		bActuallySet = TRUE;
	}
	else if ( (pHalData->bHwRadioOff == FALSE) && (eRfPowerStateToSet == eRfOff))
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("HW_RadioGpioChk92SE  - HW Radio OFF\n"));
		pHalData->bHwRadioOff = TRUE;
		bActuallySet = TRUE;
	}
			
	if(bActuallySet)
	{
		pHalData->bHwRfOffAction = 1;
		MgntActSet_RF_State(pAdapter, eRfPowerStateToSet, RF_CHANGE_BY_HW);
		DrvIFIndicateCurrentPhyStatus(pAdapter);

	
		{
			PMP_ADAPTER		pDevice = &(pAdapter->NdisAdapter);
			if(pDevice->RegHwSwRfOffD3 == 1 || pDevice->RegHwSwRfOffD3 == 2) // ASPM
				(eRfPowerStateToSet == eRfOff) ? RT_ENABLE_ASPM(pAdapter) : RT_DISABLE_ASPM(pAdapter);
		}
	}
	RT_TRACE(COMP_RF, DBG_TRACE, ("HW_RadioGpioChk92SE() <--------- \n"));
#endif
}/* HW_RadioGpioChk92SE */
#endif
#endif //#ifdef RTL8192SE
