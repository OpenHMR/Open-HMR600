/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * Based on the r8180 driver, which is:
 * Copyright 2004-2005 Andrea Merello <andreamrl@tiscali.it>, et al.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
******************************************************************************/

#ifndef __INC_DOT11D_H
#define __INC_DOT11D_H

#ifdef ENABLE_DOT11D
#include "../../rtllib/rtllib.h"

//#define ENABLE_DOT11D

//#define DOT11D_MAX_CHNL_NUM 83

typedef struct _CHNL_TXPOWER_TRIPLE {
	u8 FirstChnl;
	u8  NumChnls;
	u8  MaxTxPowerInDbm;
}CHNL_TXPOWER_TRIPLE, *PCHNL_TXPOWER_TRIPLE;

typedef enum _DOT11D_STATE {
	DOT11D_STATE_NONE = 0,
	DOT11D_STATE_LEARNED,
	DOT11D_STATE_DONE,
}DOT11D_STATE;

typedef struct _RT_DOT11D_INFO {
	//DECLARE_RT_OBJECT(RT_DOT11D_INFO);

	bool bEnabled; // dot11MultiDomainCapabilityEnabled

	u16 CountryIeLen; // > 0 if CountryIeBuf[] contains valid country information element.
	u8  CountryIeBuf[MAX_IE_LEN];
	u8  CountryIeSrcAddr[6]; // Source AP of the country IE.
	u8  CountryIeWatchdog; 

	u8  channel_map[MAX_CHANNEL_NUMBER+1];  //!!!Value 0: Invalid, 1: Valid (active scan), 2: Valid (passive scan)
	//u8  ChnlListLen; // #Bytes valid in ChnlList[].
	//u8  ChnlList[DOT11D_MAX_CHNL_NUM];
	u8  MaxTxPwrDbmList[MAX_CHANNEL_NUMBER+1];

	DOT11D_STATE State;
}RT_DOT11D_INFO, *PRT_DOT11D_INFO;
#define eqMacAddr(a,b)		( ((a)[0]==(b)[0] && (a)[1]==(b)[1] && (a)[2]==(b)[2] && (a)[3]==(b)[3] && (a)[4]==(b)[4] && (a)[5]==(b)[5]) ? 1:0 )
#define cpMacAddr(des,src)	      ((des)[0]=(src)[0],(des)[1]=(src)[1],(des)[2]=(src)[2],(des)[3]=(src)[3],(des)[4]=(src)[4],(des)[5]=(src)[5])
#define GET_DOT11D_INFO(__pIeeeDev) ((PRT_DOT11D_INFO)((__pIeeeDev)->pDot11dInfo))

#define IS_DOT11D_ENABLE(__pIeeeDev) GET_DOT11D_INFO(__pIeeeDev)->bEnabled
#define IS_COUNTRY_IE_VALID(__pIeeeDev) (GET_DOT11D_INFO(__pIeeeDev)->CountryIeLen > 0)

#define IS_EQUAL_CIE_SRC(__pIeeeDev, __pTa) eqMacAddr(GET_DOT11D_INFO(__pIeeeDev)->CountryIeSrcAddr, __pTa) 
#define UPDATE_CIE_SRC(__pIeeeDev, __pTa) cpMacAddr(GET_DOT11D_INFO(__pIeeeDev)->CountryIeSrcAddr, __pTa)

#define IS_COUNTRY_IE_CHANGED(__pIeeeDev, __Ie) \
	(((__Ie).Length == 0 || (__Ie).Length != GET_DOT11D_INFO(__pIeeeDev)->CountryIeLen) ? \
	false : \
	(!memcmp(GET_DOT11D_INFO(__pIeeeDev)->CountryIeBuf, (__Ie).Octet, (__Ie).Length)))

#define CIE_WATCHDOG_TH 1
#define GET_CIE_WATCHDOG(__pIeeeDev) GET_DOT11D_INFO(__pIeeeDev)->CountryIeWatchdog
#define RESET_CIE_WATCHDOG(__pIeeeDev) GET_CIE_WATCHDOG(__pIeeeDev) = 0 
#define UPDATE_CIE_WATCHDOG(__pIeeeDev) ++GET_CIE_WATCHDOG(__pIeeeDev)

#define IS_DOT11D_STATE_DONE(__pIeeeDev) (GET_DOT11D_INFO(__pIeeeDev)->State == DOT11D_STATE_DONE)


void
Dot11d_Init(
	struct rtllib_device *dev
	);

void
Dot11d_Reset(
	struct rtllib_device *dev
	);

void
Dot11d_UpdateCountryIe(
	struct rtllib_device *dev,
	u8 *		pTaddr,
	u16	CoutryIeLen,
	u8 * pCoutryIe	 
	);

u8
DOT11D_GetMaxTxPwrInDbm(
	struct rtllib_device *dev,
	u8 Channel
	);

void
DOT11D_ScanComplete(
	struct rtllib_device * dev
	);

int IsLegalChannel(
	struct rtllib_device * dev,
	u8 channel
);

int ToLegalChannel(
	struct rtllib_device * dev,
	u8 channel
);
#endif //ENABLE_DOT11D
#endif // #ifndef __INC_DOT11D_H
