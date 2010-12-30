/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
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
#ifdef ENABLE_DOT11D
//-----------------------------------------------------------------------------
//	File:
//		Dot11d.c
//
//	Description:
//		Implement 802.11d. 
//
//-----------------------------------------------------------------------------
#include "dot11d.h"

typedef struct _CHANNEL_LIST
{
	        u8      Channel[32];
		        u8      Len;
}CHANNEL_LIST, *PCHANNEL_LIST;

static CHANNEL_LIST ChannelPlan[] = {
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,149,153,157,161,165},24},//COUNTRY_CODE_FCC
	{{1,2,3,4,5,6,7,8,9,10,11},11},    //COUNTRY_CODE_IC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64},21},//COUNTRY_CODE_ETSI
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},   //COUNTRY_CODE_SPAIN 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},   //COUNTRY_CODE_FRANCE
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22}, //COUNTRY_CODE_MKK
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22}, //COUNTRY_CODE_MKK1
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},   //COUNTRY_CODE_ISRAEL.
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22}, //COUNTRY_CODE_TELEC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22}, //COUNTRY_CODE_MIC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14},14}, //COUNTRY_CODE_GLOBAL_DOMAIN:1-11:active,12-14 passive. 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},    //COUNTRY_CODE_WORLD_WIDE_13 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64},21}    //COUNTRY_CODE_TELEC_NETGEAR
};

void Dot11d_Init(struct rtllib_device *ieee)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(ieee);

	pDot11dInfo->bEnabled = 0;

	pDot11dInfo->State = DOT11D_STATE_NONE;
	pDot11dInfo->CountryIeLen = 0;
	memset(pDot11dInfo->channel_map, 0, MAX_CHANNEL_NUMBER+1);  
	memset(pDot11dInfo->MaxTxPwrDbmList, 0xFF, MAX_CHANNEL_NUMBER+1);
	RESET_CIE_WATCHDOG(ieee);

	printk("Dot11d_Init()\n");
}

void Dot11d_Channelmap(u8 channel_plan, struct rtllib_device* ieee)
{
	int i, max_chan = 14, min_chan = 1;

	ieee->bGlobalDomain = false;

	if (ChannelPlan[channel_plan].Len != 0) {
		// Clear old channel map
		memset(GET_DOT11D_INFO(ieee)->channel_map, 0, sizeof(GET_DOT11D_INFO(ieee)->channel_map));
		// Set new channel map
		for (i = 0; i < ChannelPlan[channel_plan].Len; i++) {
			if (ChannelPlan[channel_plan].Channel[i] < min_chan ||
					ChannelPlan[channel_plan].Channel[i] > max_chan)
				break;
			GET_DOT11D_INFO(ieee)->channel_map[ChannelPlan[channel_plan].Channel[i]] = 1;
		}
	}

	switch (channel_plan) {
		case COUNTRY_CODE_GLOBAL_DOMAIN:
			//this flag enabled to follow 11d country IE setting, 
			//otherwise, it shall follow global domain setting
			ieee->bGlobalDomain = true;
			// 1~11 active scan
			// 12~14 passive scan
			for (i = 12; i <= 14; i++) {
				GET_DOT11D_INFO(ieee)->channel_map[i] = 2;
			}
			ieee->IbssStartChnl= 10;
			ieee->ibss_maxjoin_chal = 11;
			break;

		case COUNTRY_CODE_WORLD_WIDE_13:
			//this flag enabled to follow 11d country IE setting, 
			//otherwise, it shall follow global domain setting
			// 1~11 active scan
			// 12~13 passive scan
			for (i = 12; i <= 13; i++) {
				GET_DOT11D_INFO(ieee)->channel_map[i] = 2;
			}
			ieee->IbssStartChnl = 10;
			ieee->ibss_maxjoin_chal = 11;
			break;

		default:
			ieee->IbssStartChnl = 1;
			ieee->ibss_maxjoin_chal = 14;
			break;
	}
}
						

//
//	Description:
//		Reset to the state as we are just entering a regulatory domain.
//
void Dot11d_Reset(struct rtllib_device *ieee)
{
	u32 i;
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(ieee);
#if 0
	if(!pDot11dInfo->bEnabled)
		return;
#endif
	// Clear old channel map
	memset(pDot11dInfo->channel_map, 0, MAX_CHANNEL_NUMBER+1);
	memset(pDot11dInfo->MaxTxPwrDbmList, 0xFF, MAX_CHANNEL_NUMBER+1);
	// Set new channel map
	for (i=1; i<=11; i++) {
		(pDot11dInfo->channel_map)[i] = 1;
	}
	for (i=12; i<=14; i++) {
		(pDot11dInfo->channel_map)[i] = 2;
	}

	pDot11dInfo->State = DOT11D_STATE_NONE;
	pDot11dInfo->CountryIeLen = 0;
	RESET_CIE_WATCHDOG(ieee);

	//printk("Dot11d_Reset()\n");
}

//
//	Description:
//		Update country IE from Beacon or Probe Resopnse 
//		and configure PHY for operation in the regulatory domain.
//
//	TODO: 
//		Configure Tx power.
//
//	Assumption:
//		1. IS_DOT11D_ENABLE() is true.
//		2. Input IE is an valid one.
//
void Dot11d_UpdateCountryIe(struct rtllib_device *dev, u8 *pTaddr,
	                    u16 CoutryIeLen, u8* pCoutryIe)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(dev);
	u8 i, j, NumTriples, MaxChnlNum;
	PCHNL_TXPOWER_TRIPLE pTriple;

	memset(pDot11dInfo->channel_map, 0, MAX_CHANNEL_NUMBER+1);
	memset(pDot11dInfo->MaxTxPwrDbmList, 0xFF, MAX_CHANNEL_NUMBER+1);
	MaxChnlNum = 0;
	NumTriples = (CoutryIeLen - 3) / 3; // skip 3-byte country string.
	pTriple = (PCHNL_TXPOWER_TRIPLE)(pCoutryIe + 3);
	for(i = 0; i < NumTriples; i++)
	{
		if(MaxChnlNum >= pTriple->FirstChnl)
		{ // It is not in a monotonically increasing order, so stop processing.
			printk("Dot11d_UpdateCountryIe(): Invalid country IE, skip it........1\n");
			return; 
		}
		if(MAX_CHANNEL_NUMBER < (pTriple->FirstChnl + pTriple->NumChnls))
		{ // It is not a valid set of channel id, so stop processing.
			printk("Dot11d_UpdateCountryIe(): Invalid country IE, skip it........2\n");
			return; 
		}

		for(j = 0 ; j < pTriple->NumChnls; j++)
		{
			pDot11dInfo->channel_map[pTriple->FirstChnl + j] = 1;
			pDot11dInfo->MaxTxPwrDbmList[pTriple->FirstChnl + j] = pTriple->MaxTxPowerInDbm;
			MaxChnlNum = pTriple->FirstChnl + j;
		}	

		pTriple = (PCHNL_TXPOWER_TRIPLE)((u8*)pTriple + 3);
	}
#if 1
	//printk("Dot11d_UpdateCountryIe(): Channel List:\n");
	printk("Channel List:");
	for(i=1; i<= MAX_CHANNEL_NUMBER; i++)
		if(pDot11dInfo->channel_map[i] > 0)
			printk(" %d", i);
	printk("\n");
#endif

	UPDATE_CIE_SRC(dev, pTaddr);

	pDot11dInfo->CountryIeLen = CoutryIeLen;
	memcpy(pDot11dInfo->CountryIeBuf, pCoutryIe,CoutryIeLen);
	pDot11dInfo->State = DOT11D_STATE_LEARNED;
}

u8 DOT11D_GetMaxTxPwrInDbm( struct rtllib_device *dev, u8 Channel)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(dev);
	u8 MaxTxPwrInDbm = 255;

	if(MAX_CHANNEL_NUMBER < Channel)
	{ 
		printk("DOT11D_GetMaxTxPwrInDbm(): Invalid Channel\n");
		return MaxTxPwrInDbm; 
	}
	if(pDot11dInfo->channel_map[Channel])
	{
		MaxTxPwrInDbm = pDot11dInfo->MaxTxPwrDbmList[Channel];	
	}

	return MaxTxPwrInDbm;
}

void DOT11D_ScanComplete( struct rtllib_device * dev)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(dev);

	switch(pDot11dInfo->State)
	{
	case DOT11D_STATE_LEARNED:
		pDot11dInfo->State = DOT11D_STATE_DONE;
		break;

	case DOT11D_STATE_DONE:
		if( GET_CIE_WATCHDOG(dev) == 0 )
		{ // Reset country IE if previous one is gone. 
			Dot11d_Reset(dev); 
		}
		break;
	case DOT11D_STATE_NONE:
		break;
	}
}

int IsLegalChannel( struct rtllib_device * dev, u8 channel)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(dev);

	if(MAX_CHANNEL_NUMBER < channel)
	{ 
		printk("IsLegalChannel(): Invalid Channel\n");
		return 0; 
	}
	if(pDot11dInfo->channel_map[channel] > 0)
		return 1;
	return 0;
}

int ToLegalChannel( struct rtllib_device * dev, u8 channel)
{
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(dev);
	u8 default_chn = 0;
	u32 i = 0;

	for (i=1; i<= MAX_CHANNEL_NUMBER; i++)
	{
		if(pDot11dInfo->channel_map[i] > 0)
		{
			default_chn = i;
			break;
		}
	}

	if(MAX_CHANNEL_NUMBER < channel)
	{ 
		printk("IsLegalChannel(): Invalid Channel\n");
		return default_chn; 
	}
	
	if(pDot11dInfo->channel_map[channel] > 0)
		return channel;
	
	return default_chn;
}

#ifndef BUILT_IN_RTLLIB
EXPORT_SYMBOL_RSL(Dot11d_Init);
EXPORT_SYMBOL_RSL(Dot11d_Channelmap);
EXPORT_SYMBOL_RSL(Dot11d_Reset);
EXPORT_SYMBOL_RSL(Dot11d_UpdateCountryIe);
EXPORT_SYMBOL_RSL(DOT11D_GetMaxTxPwrInDbm);
EXPORT_SYMBOL_RSL(DOT11D_ScanComplete);
EXPORT_SYMBOL_RSL(IsLegalChannel);
EXPORT_SYMBOL_RSL(ToLegalChannel);
#endif

#endif
