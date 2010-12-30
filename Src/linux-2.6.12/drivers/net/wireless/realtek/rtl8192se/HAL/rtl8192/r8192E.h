/* 
   This is part of rtl8187 OpenSource driver.
   Copyright (C) Andrea Merello 2004-2005  <andreamrl@tiscali.it> 
   Released under the terms of GPL (General Public Licence)
   
   Parts of this driver are based on the GPL part of the 
   official realtek driver
   
   Parts of this driver are based on the rtl8192 driver skeleton 
   from Patric Schenke & Andres Salomon
   
   Parts of this driver are based on the Intel Pro Wireless 2100 GPL driver
   
   We want to tanks the Authors of those projects and the Ndiswrapper 
   project Authors.
*/

#ifndef R819xE_H
#define R819xE_H

#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/config.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
//#include <linux/usb.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/rtnetlink.h>	//for rtnl_lock()
#include <linux/wireless.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>	// Necessary because we use the proc fs
#include <linux/if_arp.h>
#include <linux/random.h>
#include <linux/version.h>
#include <asm/io.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
#include <asm/semaphore.h>
#endif
#include "ieee80211.h"

#ifdef RTL8192SE
#include "r8192S_firmware.h"
#include  "r8192SE_hw.h"
#else
#include "r819xE_firmware.h"
#include "r8192E_hw.h"
#endif

#define kassert() do { printk("%s, %d, %s - Assert", __FILE__, __LINE__, __FUNCTION__); while(1) { msleep(10);}} while(0)
                    
//#define kassert() 

#define RTL819xE_MODULE_NAME "rtl819xE"
//added for HW security, john.0629
#define FALSE 0
#define TRUE 1
#define MAX_KEY_LEN     61
#define KEY_BUF_SIZE    5

#if 0
#define BIT0            0x00000001 
#define BIT1            0x00000002 
#define BIT2            0x00000004 
#define BIT3            0x00000008 
#define BIT4            0x00000010 
#define BIT5            0x00000020 
#define BIT6            0x00000040 
#define BIT7            0x00000080 
#define BIT8            0x00000100 
#define BIT9            0x00000200 
#define BIT10           0x00000400 
#define BIT11           0x00000800 
#define BIT12           0x00001000 
#define BIT13           0x00002000 
#define BIT14           0x00004000 
#define BIT15           0x00008000 
#define BIT16           0x00010000 
#define BIT17           0x00020000 
#define BIT18           0x00040000 
#define BIT19           0x00080000 
#define BIT20           0x00100000 
#define BIT21           0x00200000 
#define BIT22           0x00400000 
#define BIT23           0x00800000 
#define BIT24           0x01000000 
#define BIT25           0x02000000 
#define BIT26           0x04000000 
#define BIT27           0x08000000 
#define BIT28           0x10000000 
#define BIT29           0x20000000 
#define BIT30           0x40000000 
#define BIT31           0x80000000 
#else
#ifndef BIT
#define BIT(_i)	(1<<(_i))
#endif
#endif
// Rx smooth factor
#define	Rx_Smooth_Factor		20
/* 2007/06/04 MH Define sliding window for RSSI history. */
#define		PHY_RSSI_SLID_WIN_MAX				100
#define		PHY_Beacon_RSSI_SLID_WIN_MAX		10

#define IC_VersionCut_D	0x3
#define IC_VersionCut_E	0x4

#if 0 //we need to use RT_TRACE instead DMESG as RT_TRACE will clearly show debug level wb.
#define DMESG(x,a...) printk(KERN_INFO RTL819xE_MODULE_NAME ": " x "\n", ## a)
#define DMESGW(x,a...) printk(KERN_WARNING RTL819xE_MODULE_NAME ": WW:" x "\n", ## a)
#define DMESGE(x,a...) printk(KERN_WARNING RTL819xE_MODULE_NAME ": EE:" x "\n", ## a)
#else
#define DMESG(x,a...)
#define DMESGW(x,a...)
#define DMESGE(x,a...)
extern u32 rt_global_debug_component;
#define RT_TRACE(component, x, args...) \
do { if(rt_global_debug_component & component) \
	printk(KERN_DEBUG RTL819xE_MODULE_NAME ":" x "\n" , \
	       ##args);\
}while(0);

#define COMP_TRACE				BIT0		// For function call tracing.
#define COMP_DBG				BIT1		// Only for temporary debug message.
#define COMP_INIT				BIT2		// during driver initialization / halt / reset.


#define COMP_RECV				BIT3		// Reveive part data path.
#define COMP_SEND				BIT4		// Send part path.
#define COMP_CMD				BIT5		// I/O Related. Added by Annie, 2006-03-02.
#define COMP_POWER				BIT6		// 802.11 Power Save mode or System/Device Power state related.
#define COMP_EPROM				BIT7		// 802.11 link related: join/start BSS, leave BSS.
#define COMP_SWBW				BIT8	// For bandwidth switch.
#define COMP_SEC				BIT9// For Security.


#define COMP_LPS				BIT10	// For Turbo Mode related. By Annie, 2005-10-21.
#define COMP_QOS				BIT11	// For QoS.

#define COMP_RATE				BIT12	// For Rate Adaptive mechanism, 2006.07.02, by rcnjko. #define COMP_EVENTS				0x00000080	// Event handling
#define COMP_RXDESC			        BIT13	// Show Rx desc information for SD3 debug. Added by Annie, 2006-07-15.
#define COMP_PHY				BIT14	
#define COMP_DIG				BIT15	// For DIG, 2006.09.25, by rcnjko.
#define COMP_TXAGC				BIT16	// For Tx power, 060928, by rcnjko. 
#define COMP_HALDM				BIT17	// For HW Dynamic Mechanism, 061010, by rcnjko. 
#define COMP_POWER_TRACKING	                BIT18	//FOR 8190 TX POWER TRACKING
#define COMP_CH			        BIT19	// Event handling

#define COMP_RF					BIT20	// For RF.
//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
//1//1Attention Please!!!<11n or 8190 specific code should be put below this line>
//1!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define COMP_FIRMWARE			        BIT21	//for firmware downloading
#define COMP_HT					BIT22	// For 802.11n HT related information. by Emily 2006-8-11

#define COMP_RESET				BIT23
#define COMP_CMDPKT			        BIT24
#define COMP_SCAN				BIT25
#define COMP_PS					BIT26
#define COMP_DOWN				BIT27  // for rm driver module
#define COMP_INTR 				BIT28  // for interrupt
#define COMP_LED				BIT29  // for led
#define COMP_MLME				BIT30  
#define COMP_ERR				BIT31  // for error out, always on
#endif

#define RTL819x_DEBUG
#ifdef RTL819x_DEBUG
#define assert(expr) \
        if (!(expr)) {                                  \
                printk( "Assertion failed! %s,%s,%s,line=%d\n", \
                #expr,__FILE__,__FUNCTION__,__LINE__);          \
        }
//wb added to debug out data buf
//if you want print DATA buffer related BA, please set ieee80211_debug_level to DATA|BA
#define RT_DEBUG_DATA(level, data, datalen)      \
        do{ if ((rt_global_debug_component & (level)) == (level))   \
                {       \
                        int _i;                                  \
                        u8* _pdata = (u8*) data;                 \
                        printk(KERN_DEBUG RTL819xE_MODULE_NAME ": %s()\n", __FUNCTION__);   \
                        for(_i=0; _i<(int)(datalen); _i++)                 \
                        {                                               \
                                printk("%2x ", _pdata[_i]);               \
                                if ((_i+1)%16 == 0) printk("\n");        \
                        }                               \
                        printk("\n");                   \
                }                                       \
        } while (0)  
#else
#define assert(expr) do {} while (0)
#define RT_DEBUG_DATA(level, data, datalen) do {} while(0)
#endif /* RTL8169_DEBUG */


//
// Queue Select Value in TxDesc
//
#ifdef RTL8192SE
#define QSLT_BK					0x2//0x01
#define QSLT_BE					0x0
#define QSLT_VI					0x5//0x4
#define QSLT_VO					0x7//0x6
#define	QSLT_BEACON			0x10
#define	QSLT_HIGH				0x11
#define	QSLT_MGNT				0x12
#define	QSLT_CMD				0x13
#else
#define QSLT_BK                                 0x1
#define QSLT_BE                                 0x0
#define QSLT_VI                                 0x4
#define QSLT_VO                                 0x6
#define QSLT_BEACON                             0x10
#define QSLT_HIGH                               0x11
#define QSLT_MGNT                               0x12
#define QSLT_CMD                                0x13
#endif

#define IS_UNDER_11N_AES_MODE(_ieee)  ((_ieee->pHTInfo->bCurrentHTSupport==TRUE) &&\
												(_ieee->pairwise_key_type==KEY_TYPE_CCMP))

//added by amy 090331
#ifdef RTL8192SE_CONFIG_ASPM_OR_D3
#define	RT_DISABLE_ASPM(dev)					PlatformDisableASPM(dev)
#define	RT_ENABLE_ASPM(dev)					PlatformEnableASPM(dev)
#define	RT_ENTER_D3(dev, _bTempSetting)		PlatformSetPMCSR(dev, 0x03, _bTempSetting)
#define	RT_LEAVE_D3(dev, _bTempSetting)			PlatformSetPMCSR(dev, 0, _bTempSetting)
#endif
//added by amy 090331 end

#define HAL_HW_PCI_REVISION_ID_8192PCIE   0x1
#define HAL_HW_PCI_REVISION_ID_8192SE 	0x10
typedef enum{
	NIC_UNKNOWN = 0,
	NIC_8192E = 1,
	NIC_8190P = 2,
	NIC_8192SE = 4,
	} nic_t;



#define DESC90_RATE1M                           0x00
#define DESC90_RATE2M                           0x01
#define DESC90_RATE5_5M                         0x02
#define DESC90_RATE11M                          0x03
#define DESC90_RATE6M                           0x04
#define DESC90_RATE9M                           0x05
#define DESC90_RATE12M                          0x06
#define DESC90_RATE18M                          0x07
#define DESC90_RATE24M                          0x08
#define DESC90_RATE36M                          0x09
#define DESC90_RATE48M                          0x0a
#define DESC90_RATE54M                          0x0b
#define DESC90_RATEMCS0                         0x00
#define DESC90_RATEMCS1                         0x01
#define DESC90_RATEMCS2                         0x02
#define DESC90_RATEMCS3                         0x03
#define DESC90_RATEMCS4                         0x04
#define DESC90_RATEMCS5                         0x05
#define DESC90_RATEMCS6                         0x06
#define DESC90_RATEMCS7                         0x07
#define DESC90_RATEMCS8                         0x08
#define DESC90_RATEMCS9                         0x09
#define DESC90_RATEMCS10                        0x0a
#define DESC90_RATEMCS11                        0x0b
#define DESC90_RATEMCS12                        0x0c
#define DESC90_RATEMCS13                        0x0d
#define DESC90_RATEMCS14                        0x0e
#define DESC90_RATEMCS15                        0x0f
#define DESC90_RATEMCS32                        0x20

#ifdef RTL8192SE
#define DESC92S_RATE1M				DESC90_RATE1M	
#define DESC92S_RATE2M				DESC90_RATE2M	
#define DESC92S_RATE5_5M			DESC90_RATE5_5M	
#define DESC92S_RATE11M				DESC90_RATE11M	
#define DESC92S_RATE6M				DESC90_RATE6M	
#define DESC92S_RATE9M				DESC90_RATE9M	
#define DESC92S_RATE12M				DESC90_RATE12M	
#define DESC92S_RATE18M				DESC90_RATE18M	
#define DESC92S_RATE24M				DESC90_RATE24M	
#define DESC92S_RATE36M				DESC90_RATE36M	
#define DESC92S_RATE48M				DESC90_RATE48M	
#define DESC92S_RATE54M				DESC90_RATE54M	
#define DESC92S_RATEMCS0			DESC90_RATEMCS0	
#define DESC92S_RATEMCS1			DESC90_RATEMCS1	
#define DESC92S_RATEMCS2			DESC90_RATEMCS2	
#define DESC92S_RATEMCS3			DESC90_RATEMCS3	
#define DESC92S_RATEMCS4			DESC90_RATEMCS4	
#define DESC92S_RATEMCS5			DESC90_RATEMCS5	
#define DESC92S_RATEMCS6			DESC90_RATEMCS6	
#define DESC92S_RATEMCS7			DESC90_RATEMCS7	
#define DESC92S_RATEMCS8			DESC90_RATEMCS8	
#define DESC92S_RATEMCS9			DESC90_RATEMCS9	
#define DESC92S_RATEMCS10			DESC90_RATEMCS10	
#define DESC92S_RATEMCS11			DESC90_RATEMCS11	
#define DESC92S_RATEMCS12			DESC90_RATEMCS12	
#define DESC92S_RATEMCS13			DESC90_RATEMCS13	
#define DESC92S_RATEMCS14			DESC90_RATEMCS14	
#define DESC92S_RATEMCS15			DESC90_RATEMCS15	
#define DESC92S_RATEMCS15_SG			0x1c
#define DESC92S_RATEMCS32			DESC90_RATEMCS32	
#endif

#define RTL819X_DEFAULT_RF_TYPE RF_1T2R
#define IEEE80211_WATCH_DOG_TIME    2000

/* For rtl819x */
#ifdef RTL8192SE
//Tx Descriptor for RLT8192SE
typedef struct _tx_desc_8192se{

	// DWORD 0
	u32		PktSize:16;
	u32		Offset:8;
	u32		Type:2;	// Reserved for MAC header Frame Type subfield.
	u32		LastSeg:1;
	u32		FirstSeg:1;
	u32		LINIP:1;
	u32		AMSDU:1;
	u32		GF:1;
	u32		OWN:1;	
	
	// DWORD 1
	u32		MacID:5;			
	u32		MoreData:1;
	u32		MoreFrag:1;
	u32		PIFS:1;
	u32		QueueSel:5;
	u32		AckPolicy:2;
	u32		NoACM:1;
	u32		NonQos:1;
	u32		KeyID:2;
	u32		OUI:1;
	u32		PktType:1;
	u32		EnDescID:1;
	u32		SecType:2;
	u32		HTC:1;	//padding0
	u32		WDS:1;	//padding1
	u32		PktOffset:5;	//padding_len (hw)	
	u32		HWPC:1;		
	
	// DWORD 2
	u32		DataRetryLmt:6;
	u32		RetryLmtEn:1;
	u32		BroadMultiCast:1; 	//Indicate Broadcast and Multicast frame is carried
	u32		TimeStamp:4;
	u32		RTSRC:6;	// Reserved for HW RTS Retry Count.
	u32		DATARC:6;	// Reserved for HW DATA Retry Count.
	u32		Rsvd1:5;
	u32		AggEn:1;
	u32		BK:1;	//Aggregation break.
	u32		OwnMAC:1;
	
	// DWORD 3
	u32		NextHeadPage:8;
	u32		TailPage:8;
	u32		Seq:12;
	u32		Frag:4;	

	// DWORD 4
	u32		RTSRate:6;
	u32		DisRTSFB:1;
	u32		RTSRateFBLmt:4;
	u32		CTS2Self:1;
	u32		RTSEn:1;
	u32		RaBRSRID:3;	//Rate adaptive BRSR ID.
	u32		TXHT:1;
	u32		TxShort:1;//for data
	u32		TxBw:1;
	u32		TXSC:2;
	u32		STBC:2;
	u32		RD:1;
	u32		RTSHT:1;
	u32		RTSShort:1;
	u32		RTSBW:1;
	u32		RTSSC:2;
	u32		RTSSTBC:2;
	u32		UserRate:1;	

	// DWORD 5
	u32		PktID:9;
	u32		TxRate:6;
	u32		DISFB:1;
	u32		DataRateFBLmt:5;
	u32		Rsvd2:3;
	u32		FWDoNotFillDuration:1;	//RAW: leave it as raw packet
	u32		InitDataRate:6;
	u32		TIDFst:1;

	// DWORD 6
	u32		IPChkSum:16;
	u32		TCPChkSum:16;

	// DWORD 7
	u32		TxBufferSize:16;//pcie
	u32		IPHdrOffset:8;
	u32		Rsvd3:7;
	u32		TCPEn:1;

	// DWORD 8
	u32		TxBuffAddr;//pcie

	// DWORD 9
	u32		NextDescAddress;//pcie

	// 2008/05/15 MH Because PCIE HW memory R/W 4K limit. And now,  our descriptor
	// size is 40 bytes. If you use more than 102 descriptor( 103*40>4096), HW will execute
	// memoryR/W CRC error. And then all DMA fetch will fail. We must decrease descriptor
	// number or enlarge descriptor size as 64 bytes.
	u32		Reserve_Pass_92S_PCIE_MM_Limit[6];
	
} tx_desc, *ptx_desc;


typedef struct _tx_desc_cmd_8192se{  
       // DWORD 0
	u32		PktSize:16;
	u32		Offset:8;	
	u32		Rsvd0:2;
	u32		FirstSeg:1;
	u32		LastSeg:1;
	u32		LINIP:1;
	u32		Rsvd1:2;
	u32		OWN:1;	
	
	// DWORD 1, 2, 3, 4, 5, 6 are all reserved.		
	u32		Rsvd2;
	u32		Rsvd3;
	u32		Rsvd4;
	u32		Rsvd5;
	u32		Rsvd6;
	u32		Rsvd7;	

	// DWORD 7
	u32		TxBufferSize:16;//pcie
	u32		Rsvd8:16;

	// DWORD 8
	u32		TxBufferAddr;//pcie

	// DWORD 9
	u32		NextTxDescAddress;//pcie

	// 2008/05/15 MH Because PCIE HW memory R/W 4K limit. And now,  our descriptor
	// size is 40 bytes. If you use more than 102 descriptor( 103*40>4096), HW will execute
	// memoryR/W CRC error. And then all DMA fetch will fail. We must decrease descriptor
	// number or enlarge descriptor size as 64 bytes.
	u32		Reserve_Pass_92S_PCIE_MM_Limit[6];
	
}tx_desc_cmd, *ptx_desc_cmd;


typedef struct _tx_status_desc_8192se{ 

	// DWORD 0
	u32		PktSize:16;
	u32		Offset:8;
	u32		Type:2;	// Reserved for MAC header Frame Type subfield.
	u32		LastSeg:1;
	u32		FirstSeg:1;
	u32		LINIP:1;
	u32		AMSDU:1;
	u32		GF:1;
	u32		OWN:1;	
	
	// DWORD 1
	u32		MacID:5;			
	u32		MoreData:1;
	u32		MoreFrag:1;
	u32		PIFS:1;
	u32		QueueSel:5;
	u32		AckPolicy:2;
	u32		NoACM:1;
	u32		NonQos:1;
	u32		KeyID:2;
	u32		OUI:1;
	u32		PktType:1;
	u32		EnDescID:1;
	u32		SecType:2;
	u32		HTC:1;	//padding0
	u32		WDS:1;	//padding1
	u32		PktOffset:5;	//padding_len (hw)	
	u32		HWPC:1;		
	
	// DWORD 2
	u32		DataRetryLmt:6;
	u32		RetryLmtEn:1;
	u32		TSFL:5;	
	u32		RTSRC:6;	// Reserved for HW RTS Retry Count.
	u32		DATARC:6;	// Reserved for HW DATA Retry Count.
	u32		Rsvd1:5;
	u32		AggEn:1;
	u32		BK:1;	//Aggregation break.
	u32		OwnMAC:1;
	
	// DWORD 3
	u32		NextHeadPage:8;
	u32		TailPage:8;
	u32		Seq:12;
	u32		Frag:4;	

	// DWORD 4
	u32		RTSRate:6;
	u32		DisRTSFB:1;
	u32		RTSRateFBLmt:4;
	u32		CTS2Self:1;
	u32		RTSEn:1;
	u32		RaBRSRID:3;	//Rate adaptive BRSR ID.
	u32		TXHT:1;
	u32		TxShort:1;//for data
	u32		TxBw:1;
	u32		TXSC:2;
	u32		STBC:2;
	u32		RD:1;
	u32		RTSHT:1;
	u32		RTSShort:1;
	u32		RTSBW:1;
	u32		RTSSC:2;
	u32		RTSSTBC:2;
	u32		UserRate:1;	

	// DWORD 5
	u32		PktID:9;
	u32		TxRate:6;
	u32		DISFB:1;
	u32		DataRateFBLmt:5;
	u32		TxAGC:11;	

	// DWORD 6
	u32		IPChkSum:16;
	u32		TCPChkSum:16;

	// DWORD 7
	u32		TxBufferSize:16;//pcie
	u32		IPHdrOffset:8;
	u32		Rsvd2:7;
	u32		TCPEn:1;

	// DWORD 8
	u32		TxBufferAddr;//pcie

	// DWORD 9
	u32		NextDescAddress;//pcie

	// 2008/05/15 MH Because PCIE HW memory R/W 4K limit. And now,  our descriptor
	// size is 40 bytes. If you use more than 102 descriptor( 103*40>4096), HW will execute
	// memoryR/W CRC error. And then all DMA fetch will fail. We must decrease descriptor
	// number or enlarge descriptor size as 64 bytes.
	u32		Reserve_Pass_92S_PCIE_MM_Limit[6];

}tx_status_desc, *ptx_status_desc;


typedef struct _rx_desc_8192se{
	//DWORD 0
	u32		Length:14;
	u32		CRC32:1;
	u32		ICVError:1;
	u32		DrvInfoSize:4;
	u32		Security:3;
	u32		Qos:1;
	u32		Shift:2;
	u32		PHYStatus:1;
	u32		SWDec:1;
	u32		LastSeg:1;
	u32		FirstSeg:1;
	u32		EOR:1;
	u32		OWN:1;	

	//DWORD 1
	u32		MACID:5;
	u32		TID:4;
	u32		HwRsvd:5;
	u32		PAGGR:1;
	u32		FAGGR:1;
	u32		A1_FIT:4;
	u32		A2_FIT:4;
	u32		PAM:1;
	u32		PWR:1;
	u32		MoreData:1;
	u32		MoreFrag:1;
	u32		Type:2;
	u32		MC:1;
	u32		BC:1;

	//DWORD 2
	u32		Seq:12;
	u32		Frag:4;
	u32		NextPktLen:14;
	u32		NextIND:1;
	u32		Rsvd:1;

	//DWORD 3
	u32		RxMCS:6;
	u32		RxHT:1;
	u32		AMSDU:1;
	u32		SPLCP:1;
	u32		BandWidth:1;
	u32		HTC:1;
	u32		TCPChkRpt:1;
	u32		IPChkRpt:1;
	u32		TCPChkValID:1;
	u32		HwPCErr:1;
	u32		HwPCInd:1;
	u32		IV0:16;

	//DWORD 4
	u32		IV1;

	//DWORD 5
	u32		TSFL;

	//DWORD 6
	u32		BufferAddress;

	//DWORD 7	
	u32		NextRxDescAddress;

	//DWORD 8
#if 0		// 92SE RX descriptor does not contain the info. They are saved at driver info.
	u32		BA_SSN:12;
	u32		BA_VLD:1;
	u32		RSVD:19;
#endif

} rx_desc, *prx_desc;



typedef struct _rx_desc_status_8192se{
	//DWORD 0
	u32		Length:14;
	u32		CRC32:1;
	u32		ICVError:1;
	u32		DrvInfoSize:4;
	u32		Security:3;
	u32		Qos:1;
	u32		Shift:2;
	u32		PHYStatus:1;
	u32		SWDec:1;
	u32		LastSeg:1;
	u32		FirstSeg:1;
	u32		EOR:1;
	u32		OWN:1;	

	//DWORD 1
	u32		MACID:5;
	u32		TID:4;
	u32		HwRsvd:5;
	u32		PAGGR:1;
	u32		FAGGR:1;
	u32		A1_FIT:4;
	u32		A2_FIT:4;
	u32		PAM:1;
	u32		PWR:1;
	u32		MoreData:1;
	u32		MoreFrag:1;
	u32		Type:2;
	u32		MC:1;
	u32		BC:1;

	//DWORD 2
	u32		Seq:12;
	u32		Frag:4;
	u32		NextPktLen:14;
	u32		NextIND:1;
	u32		Rsvd:1;

	//DWORD 3
	u32		RxMCS:6;
	u32		RxHT:1;
	u32		AMSDU:1;
	u32		SPLCP:1;
	u32		BW:1;
	u32		HTC:1;
	u32		TCPChkRpt:1;
	u32		IPChkRpt:1;
	u32		TCPChkValID:1;
	u32		HwPCErr:1;
	u32		HwPCInd:1;
	u32		IV0:16;

	//DWORD 4
	u32		IV1;

	//DWORD 5
	u32		TSFL;


	//DWORD 6
	u32		BufferAddress;

	//DWORD 7	
	u32		NextRxDescAddress;

	//DWORD 8
#if 0		// 92SE RX descriptor does not contain the info. They are saved at driver info.
	u32		BA_SSN:12;
	u32		BA_VLD:1;
	u32		RSVD:19;
#endif
}rx_desc_status, *prx_desc_status;

//just treat this as fwinfo for compatible with 9xP //WB
typedef struct  _rx_fwinfo_8192s{
	//
	// Driver info contain PHY status and other variabel size info	
	// PHY Status content as below
	//
	
	//DWORD 0
	/*u32			gain_0:7;
	u32			trsw_0:1;
	u32			gain_1:7;
	u32			trsw_1:1;
	u32			gain_2:7;
	u32			trsw_2:1;
	u32			gain_3:7;
	u32			trsw_3:1;	*/
	u8			gain_trsw[4];

	//DWORD 1	
	/*u32			pwdb_all:8;
	u32			cfosho_0:8;
	u32			cfosho_1:8;
	u32			cfosho_2:8;*/
	u8			pwdb_all;
	u8			cfosho[4];

	//DWORD 2
	/*u32			cfosho_3:8;
	u32			cfotail_0:8;
	u32			cfotail_1:8;
	u32			cfotail_2:8;*/
	u8			cfotail[4];

	//DWORD 3
	/*u32			cfotail_3:8;
	u32			rxevm_0:8;
	u32			rxevm_1:8;
	u32			rxsnr_0:8;*/
	s8			rxevm[2];
	s8			rxsnr[4];

	//DWORD 4
	/*u32			rxsnr_1:8;
	u32			rxsnr_2:8;
	u32			rxsnr_3:8;
	u32			pdsnr_0:8;*/
	u8			pdsnr[2];

	//DWORD 5
	/*u32			pdsnr_1:8;
	u32			csi_current_0:8;
	u32			csi_current_1:8;
	u32			csi_target_0:8;*/
	u8			csi_current[2];
	u8			csi_target[2];

	//DWORD 6
	/*u32			csi_target_1:8;
	u32			sigevm:8;
	u32			max_ex_pwr:8;
	u32			ex_intf_flag:1;
	u32			sgi_en:1;
	u32			rxsc:2;
	u32			reserve:4;*/
	u8			sigevm;
	u8			max_ex_pwr;
	u8			ex_intf_flag:1;
	u8			sgi_en:1;
	u8			rxsc:2;
	u8			reserve:4;
	
}rx_fwinfo, *prx_fwinfo;

#else
typedef struct _tx_desc_819x_pci {
        //DWORD 0
        u16	PktSize;
        u8	Offset;
        u8	Reserved1:3;
        u8	CmdInit:1;
        u8	LastSeg:1;
        u8	FirstSeg:1;
        u8	LINIP:1;
        u8	OWN:1;

        //DWORD 1
        u8	TxFWInfoSize;
        u8	RATid:3;
        u8	DISFB:1;
        u8	USERATE:1;
        u8	MOREFRAG:1;
        u8	NoEnc:1;
        u8	PIFS:1;
        u8	QueueSelect:5;
        u8	NoACM:1;
        u8	Resv:2;
        u8	SecCAMID:5;
        u8	SecDescAssign:1;
        u8	SecType:2;

        //DWORD 2
        u16	TxBufferSize;
        u8	PktId:7;
        u8	Resv1:1;
        u8	Reserved2;

        //DWORD 3
	u32 	TxBuffAddr;

	//DWORD 4
	u32	NextDescAddress;

	//DWORD 5,6,7
        u32	Reserved5;
        u32	Reserved6;
        u32	Reserved7;
}tx_desc, *ptx_desc;


typedef struct _tx_desc_cmd_819x_pci {
        //DWORD 0
	u16	PktSize;
	u8	Reserved1;
	u8	CmdType:3;
	u8	CmdInit:1;
	u8	LastSeg:1;
	u8	FirstSeg:1;
	u8	LINIP:1;
	u8	OWN:1;

        //DOWRD 1
	u16	ElementReport;
	u16	Reserved2;

        //DOWRD 2
	u16 	TxBufferSize;
	u16	Reserved3;
	
       //DWORD 3,4,5
	u32	TxBuffAddr;
	u32	NextDescAddress;
	u32	Reserved4;
	u32	Reserved5;
	u32	Reserved6;
}tx_desc_cmd, *ptx_desc_cmd;

typedef struct _rx_desc_819x_pci{
	//DOWRD 0
	u16			Length:14;
	u16			CRC32:1;
	u16			ICV:1;
	u8			RxDrvInfoSize;
	u8			Shift:2;
	u8			PHYStatus:1;
	u8			SWDec:1;
	u8					LastSeg:1;
	u8					FirstSeg:1;
	u8					EOR:1;
	u8					OWN:1;

	//DWORD 1
	u32			Reserved2;

	//DWORD 2
	u32			Reserved3;

	//DWORD 3
	u32	BufferAddress;
	
}rx_desc, *prx_desc;


typedef struct _rx_fwinfo_819x_pci{
	//DWORD 0
	u16			Reserved1:12;
	u16			PartAggr:1;
	u16			FirstAGGR:1;
	u16			Reserved2:2;

	u8			RxRate:7;
	u8			RxHT:1;

	u8			BW:1;
	u8			SPLCP:1;
	u8			Reserved3:2;
	u8			PAM:1;	
	u8			Mcast:1;	
	u8			Bcast:1;	
	u8			Reserved4:1;	

	//DWORD 1
	u32			TSFL;	
	
}rx_fwinfo, *prx_fwinfo;

#endif

#define MAX_DEV_ADDR_SIZE		8  /* support till 64 bit bus width OS */
#define MAX_FIRMWARE_INFORMATION_SIZE   32 /*2006/04/30 by Emily forRTL8190*/
#define MAX_802_11_HEADER_LENGTH        (40 + MAX_FIRMWARE_INFORMATION_SIZE)
#define ENCRYPTION_MAX_OVERHEAD		128
#define MAX_FRAGMENT_COUNT		8
#define MAX_TRANSMIT_BUFFER_SIZE  	(1600+(MAX_802_11_HEADER_LENGTH+ENCRYPTION_MAX_OVERHEAD)*MAX_FRAGMENT_COUNT) 

#define scrclng					4		// octets for crc32 (FCS, ICV)
/* 8190 Loopback Mode definition */
typedef enum _rtl819x_loopback{
	RTL819X_NO_LOOPBACK = 0,
	RTL819X_MAC_LOOPBACK = 1,
	RTL819X_DMA_LOOPBACK = 2,
	RTL819X_CCK_LOOPBACK = 3,
}rtl819x_loopback_e;

//+by amy 080507
#define MAX_RECEIVE_BUFFER_SIZE	9100	// Add this to 9100 bytes to receive A-MSDU from RT-AP

/* Firmware Queue Layout */
#define NUM_OF_FIRMWARE_QUEUE		10
#define NUM_OF_PAGES_IN_FW		0x100
#define NUM_OF_PAGE_IN_FW_QUEUE_BE	0x07
#define NUM_OF_PAGE_IN_FW_QUEUE_BK	0x07
#define NUM_OF_PAGE_IN_FW_QUEUE_VI	0x07
#define NUM_OF_PAGE_IN_FW_QUEUE_VO	0x07
#define NUM_OF_PAGE_IN_FW_QUEUE_HCCA	0x0
#define NUM_OF_PAGE_IN_FW_QUEUE_CMD	0x0
#define NUM_OF_PAGE_IN_FW_QUEUE_MGNT	0x02
#define NUM_OF_PAGE_IN_FW_QUEUE_HIGH	0x02
#define NUM_OF_PAGE_IN_FW_QUEUE_BCN		0x02
#define NUM_OF_PAGE_IN_FW_QUEUE_PUB		0xA1

#ifdef RTL8192E
#define APPLIED_RESERVED_QUEUE_IN_FW	0x80000000
#define RSVD_FW_QUEUE_PAGE_BK_SHIFT	0x00
#define RSVD_FW_QUEUE_PAGE_BE_SHIFT	0x08
#define RSVD_FW_QUEUE_PAGE_VI_SHIFT	0x10
#define RSVD_FW_QUEUE_PAGE_VO_SHIFT	0x18
#define RSVD_FW_QUEUE_PAGE_MGNT_SHIFT	0x10
#define RSVD_FW_QUEUE_PAGE_CMD_SHIFT	0x08
#define RSVD_FW_QUEUE_PAGE_BCN_SHIFT	0x00
#define RSVD_FW_QUEUE_PAGE_PUB_SHIFT	0x08
#endif

typedef	enum _RT_EEPROM_TYPE{
	EEPROM_93C46,
	EEPROM_93C56,
	EEPROM_BOOT_EFUSE,
}RT_EEPROM_TYPE,*PRT_EEPROM_TYPE;

#define DEFAULT_FRAG_THRESHOLD 2342U
#define MIN_FRAG_THRESHOLD     256U
#define DEFAULT_BEACONINTERVAL 0x64U
#define DEFAULT_BEACON_ESSID "Rtl819xU"

#define DEFAULT_SSID ""
#define DEFAULT_RETRY_RTS 7
#define DEFAULT_RETRY_DATA 7
#define PRISM_HDR_SIZE 64

#define		PHY_RSSI_SLID_WIN_MAX				100


typedef enum _WIRELESS_MODE {
	WIRELESS_MODE_UNKNOWN = 0x00,
	WIRELESS_MODE_A = 0x01,
	WIRELESS_MODE_B = 0x02,
	WIRELESS_MODE_G = 0x04,
	WIRELESS_MODE_AUTO = 0x08,
	WIRELESS_MODE_N_24G = 0x10,
	WIRELESS_MODE_N_5G = 0x20
} WIRELESS_MODE;

#define RTL_IOCTL_WPA_SUPPLICANT		SIOCIWFIRSTPRIV+30

typedef struct buffer
{
	struct buffer *next;
	u32 *buf;
	dma_addr_t dma;
	
} buffer;

typedef struct rtl_reg_debug{
        unsigned int  cmd;
        struct {
                unsigned char type;
                unsigned char addr;
                unsigned char page;
                unsigned char length;
        } head;
        unsigned char buf[0xff];
}rtl_reg_debug;

#if 0

typedef struct tx_pendingbuf
{
	struct ieee80211_txb *txb;
	short ispending;
	short descfrag;
} tx_pendigbuf;

#endif

typedef struct _rt_9x_tx_rate_history {
	u32             cck[4];
	u32             ofdm[8];
	// HT_MCS[0][]: BW=0 SG=0
	// HT_MCS[1][]: BW=1 SG=0
	// HT_MCS[2][]: BW=0 SG=1
	// HT_MCS[3][]: BW=1 SG=1
	u32             ht_mcs[4][16];
}rt_tx_rahis_t, *prt_tx_rahis_t;

typedef	struct _RT_SMOOTH_DATA_4RF {
	char	elements[4][100];//array to store values
	u32	index;			//index to current array to store
	u32	TotalNum;		//num of valid elements
	u32	TotalVal[4];		//sum of valid elements
}RT_SMOOTH_DATA_4RF, *PRT_SMOOTH_DATA_4RF;

typedef enum _tag_TxCmd_Config_Index{
	TXCMD_TXRA_HISTORY_CTRL				= 0xFF900000,
	TXCMD_RESET_TX_PKT_BUFF				= 0xFF900001,
	TXCMD_RESET_RX_PKT_BUFF				= 0xFF900002,
	TXCMD_SET_TX_DURATION				= 0xFF900003,
	TXCMD_SET_RX_RSSI						= 0xFF900004,
	TXCMD_SET_TX_PWR_TRACKING			= 0xFF900005,
	TXCMD_XXXX_CTRL,
}DCMD_TXCMD_OP;

typedef struct Stats
{
	unsigned long txrdu;
	unsigned long rxrdu;
	//unsigned long rxnolast;
	//unsigned long rxnodata;
//	unsigned long rxreset;
//	unsigned long rxnopointer;
	unsigned long rxok;
	unsigned long rxframgment;
	unsigned long rxcmdpkt[4];		//08/05/08 amy rx cmd element txfeedback/bcn report/cfg set/query
	unsigned long rxurberr;
	unsigned long rxstaterr;
	unsigned long rxcrcerrmin;//crc error (0-500) 
	unsigned long rxcrcerrmid;//crc error (500-1000)
	unsigned long rxcrcerrmax;//crc error (>1000)
	unsigned long received_rate_histogram[4][32];	//0: Total, 1:OK, 2:CRC, 3:ICV, 2007 07 03 cosa
	unsigned long received_preamble_GI[2][32];		//0: Long preamble/GI, 1:Short preamble/GI
	unsigned long	rx_AMPDUsize_histogram[5]; // level: (<4K), (4K~8K), (8K~16K), (16K~32K), (32K~64K)
	unsigned long rx_AMPDUnum_histogram[5]; // level: (<5), (5~10), (10~20), (20~40), (>40)
	unsigned long numpacket_matchbssid;	// debug use only.
	unsigned long numpacket_toself;		// debug use only.
	unsigned long num_process_phyinfo;		// debug use only.
	unsigned long numqry_phystatus;
	unsigned long numqry_phystatusCCK;
	unsigned long numqry_phystatusHT;
	unsigned long received_bwtype[5];              //0: 20M, 1: funn40M, 2: upper20M, 3: lower20M, 4: duplicate
	unsigned long txnperr;
	unsigned long txnpdrop;
	unsigned long txresumed;
//	unsigned long rxerr;
	unsigned long rxoverflow;
	unsigned long rxint;
	unsigned long txnpokint;
//	unsigned long txhpokint;
//	unsigned long txhperr;
	unsigned long ints;
	unsigned long shints;
	unsigned long txoverflow;
//	unsigned long rxdmafail;
//	unsigned long txbeacon;
//	unsigned long txbeaconerr;
	unsigned long txlpokint;
	unsigned long txlpdrop;
	unsigned long txlperr;
	unsigned long txbeokint;
	unsigned long txbedrop;
	unsigned long txbeerr;
	unsigned long txbkokint;
	unsigned long txbkdrop;
	unsigned long txbkerr;
	unsigned long txviokint;
	unsigned long txvidrop;
	unsigned long txvierr;
	unsigned long txvookint;
	unsigned long txvodrop;
	unsigned long txvoerr;
	unsigned long txbeaconokint;
	unsigned long txbeacondrop;
	unsigned long txbeaconerr;
	unsigned long txmanageokint;
	unsigned long txmanagedrop;
	unsigned long txmanageerr;
	unsigned long txcmdpktokint;
	unsigned long txdatapkt;
	unsigned long txfeedback;
	unsigned long txfeedbackok;
	unsigned long txoktotal;
	unsigned long txokbytestotal;
	unsigned long txokinperiod;
	unsigned long txmulticast;
	unsigned long txbytesmulticast;
	unsigned long txbroadcast;
	unsigned long txbytesbroadcast;
	unsigned long txunicast;
	unsigned long txbytesunicast;
	unsigned long rxbytesunicast;
	unsigned long txfeedbackfail;
	unsigned long txerrtotal;
	unsigned long txerrbytestotal;
	unsigned long txerrmulticast;
	unsigned long txerrbroadcast;
	unsigned long txerrunicast;
	unsigned long txretrycount;
	unsigned long txfeedbackretry;
	u8			last_packet_rate;
	unsigned long slide_signal_strength[100];
	unsigned long slide_evm[100];
	unsigned long	slide_rssi_total;	// For recording sliding window's RSSI value
	unsigned long slide_evm_total;	// For recording sliding window's EVM value
	long signal_strength; // Transformed, in dbm. Beautified signal strength for UI, not correct.
	long signal_quality;
	long last_signal_strength_inpercent;
	long	recv_signal_power;	// Correct smoothed ss in Dbm, only used in driver to report real power now.
	u8 rx_rssi_percentage[4];
	u8 rx_evm_percentage[2];
	long rxSNRdB[4];
	rt_tx_rahis_t txrate;
	u32 Slide_Beacon_pwdb[100];	//cosa add for beacon rssi
	u32 Slide_Beacon_Total;		//cosa add for beacon rssi
	RT_SMOOTH_DATA_4RF		cck_adc_pwdb;
	u32	CurrentShowTxate;


} Stats;


// Bandwidth Offset                     	
#define HAL_PRIME_CHNL_OFFSET_DONT_CARE		0
#define HAL_PRIME_CHNL_OFFSET_LOWER			1
#define HAL_PRIME_CHNL_OFFSET_UPPER			2

//+by amy 080507

typedef struct 	ChnlAccessSetting {
	u16 SIFS_Timer;
	u16 DIFS_Timer; 
	u16 SlotTimeTimer;
	u16 EIFS_Timer;
	u16 CWminIndex;
	u16 CWmaxIndex;
}*PCHANNEL_ACCESS_SETTING,CHANNEL_ACCESS_SETTING;

typedef struct _BB_REGISTER_DEFINITION{
	u32 rfintfs; 			// set software control: //		0x870~0x877[8 bytes]
	u32 rfintfi; 			// readback data: //		0x8e0~0x8e7[8 bytes]
	u32 rfintfo; 			// output data: //		0x860~0x86f [16 bytes]
	u32 rfintfe; 			// output enable: //		0x860~0x86f [16 bytes]
	u32 rf3wireOffset; 		// LSSI data: //		0x840~0x84f [16 bytes]
	u32 rfLSSI_Select; 		// BB Band Select: //		0x878~0x87f [8 bytes]
	u32 rfTxGainStage;		// Tx gain stage: //		0x80c~0x80f [4 bytes]
	u32 rfHSSIPara1; 		// wire parameter control1 : //		0x820~0x823,0x828~0x82b, 0x830~0x833, 0x838~0x83b [16 bytes]
	u32 rfHSSIPara2; 		// wire parameter control2 : //		0x824~0x827,0x82c~0x82f, 0x834~0x837, 0x83c~0x83f [16 bytes]
	u32 rfSwitchControl; 	//Tx Rx antenna control : //		0x858~0x85f [16 bytes]
	u32 rfAGCControl1; 	//AGC parameter control1 : //		0xc50~0xc53,0xc58~0xc5b, 0xc60~0xc63, 0xc68~0xc6b [16 bytes] 
	u32 rfAGCControl2; 	//AGC parameter control2 : //		0xc54~0xc57,0xc5c~0xc5f, 0xc64~0xc67, 0xc6c~0xc6f [16 bytes] 
	u32 rfRxIQImbalance; 	//OFDM Rx IQ imbalance matrix : //		0xc14~0xc17,0xc1c~0xc1f, 0xc24~0xc27, 0xc2c~0xc2f [16 bytes]
	u32 rfRxAFE;  			//Rx IQ DC ofset and Rx digital filter, Rx DC notch filter : //		0xc10~0xc13,0xc18~0xc1b, 0xc20~0xc23, 0xc28~0xc2b [16 bytes]
	u32 rfTxIQImbalance; 	//OFDM Tx IQ imbalance matrix //		0xc80~0xc83,0xc88~0xc8b, 0xc90~0xc93, 0xc98~0xc9b [16 bytes]
	u32 rfTxAFE; 			//Tx IQ DC Offset and Tx DFIR type //		0xc84~0xc87,0xc8c~0xc8f, 0xc94~0xc97, 0xc9c~0xc9f [16 bytes]
	u32 rfLSSIReadBack; 	//LSSI RF readback data SI mode //		0x8a0~0x8af [16 bytes]
	u32 rfLSSIReadBackPi; 	//LSSI RF readback data PI mode 0x8b7~0x8bc for Path A and B
}BB_REGISTER_DEFINITION_T, *PBB_REGISTER_DEFINITION_T;

typedef enum _RT_RF_TYPE_819xU{
        RF_TYPE_MIN = 0,
        RF_8225,
        RF_8256,
        RF_8258,
        RF_6052=4,		// 4 11b/g/n RF
        RF_PSEUDO_11N = 5,
}RT_RF_TYPE_819xU, *PRT_RF_TYPE_819xU;


typedef struct _rate_adaptive
{
	u8				rate_adaptive_disabled;
	u8				ratr_state;
	u16				reserve;	
	
	u32				high_rssi_thresh_for_ra;
	u32				high2low_rssi_thresh_for_ra;
	u8				low2high_rssi_thresh_for_ra40M;
	u32				low_rssi_thresh_for_ra40M;
	u8				low2high_rssi_thresh_for_ra20M;
	u32				low_rssi_thresh_for_ra20M;
	u32				upper_rssi_threshold_ratr;
	u32				middle_rssi_threshold_ratr;
	u32				low_rssi_threshold_ratr;
	u32				low_rssi_threshold_ratr_40M;
	u32				low_rssi_threshold_ratr_20M;
	u8				ping_rssi_enable;	//cosa add for test
	u32				ping_rssi_ratr;	//cosa add for test
	u32				ping_rssi_thresh_for_ra;//cosa add for test
	u32				last_ratr;
	
} rate_adaptive, *prate_adaptive;
#define TxBBGainTableLength 37
#define	CCKTxBBGainTableLength 23
typedef struct _txbbgain_struct
{
	long	txbb_iq_amplifygain;
	u32	txbbgain_value;
} txbbgain_struct, *ptxbbgain_struct;

typedef struct _ccktxbbgain_struct
{
	//The Value is from a22 to a29 one Byte one time is much Safer 
	u8	ccktxbb_valuearray[8];
} ccktxbbgain_struct,*pccktxbbgain_struct;


typedef struct _init_gain
{
	u8				xaagccore1;
	u8				xbagccore1;
	u8				xcagccore1;
	u8				xdagccore1;
	u8				cca;

} init_gain, *pinit_gain;

/* 2007/11/02 MH Define RF mode temporarily for test. */
typedef enum tag_Rf_Operatetion_State
{    
    RF_STEP_INIT = 0,
    RF_STEP_NORMAL,   
    RF_STEP_MAX
}RF_STEP_E;

typedef enum _RT_STATUS{
	RT_STATUS_SUCCESS,
	RT_STATUS_FAILURE,
	RT_STATUS_PENDING,
	RT_STATUS_RESOURCE
}RT_STATUS,*PRT_STATUS;

typedef enum _RT_CUSTOMER_ID
{
	RT_CID_DEFAULT = 0,
	RT_CID_8187_ALPHA0 = 1,
	RT_CID_8187_SERCOMM_PS = 2,
	RT_CID_8187_HW_LED = 3,
	RT_CID_8187_NETGEAR = 4,
	RT_CID_WHQL = 5,
	RT_CID_819x_CAMEO  = 6, 
	RT_CID_819x_RUNTOP = 7,
	RT_CID_819x_Senao = 8,
	RT_CID_TOSHIBA = 9,	// Merge by Jacken, 2008/01/31.
	RT_CID_819x_Netcore = 10,
	RT_CID_Nettronix = 11,
	RT_CID_DLINK = 12,
	RT_CID_PRONET = 13,
	RT_CID_COREGA = 14,
	RT_CID_819x_ALPHA = 15,
	RT_CID_819x_Sitecom = 16,
	RT_CID_CCX = 17, // It's set under CCX logo test and isn't demanded for CCX functions, but for test behavior like retry limit and tx report. By Bruce, 2009-02-17.	
	RT_CID_819x_Lenovo = 18,	
}RT_CUSTOMER_ID, *PRT_CUSTOMER_ID;

//================================================================================
// LED customization.
//================================================================================

typedef	enum _LED_STRATEGY_8190{
	SW_LED_MODE0, // SW control 1 LED via GPIO0. It is default option.
	SW_LED_MODE1, // SW control for PCI Express
	SW_LED_MODE2, // SW control for Cameo.
	SW_LED_MODE3, // SW contorl for RunTop.
	SW_LED_MODE4, // SW control for Netcore
	SW_LED_MODE5, //added by vivi, for led new mode, DLINK
	SW_LED_MODE6, //added by vivi, for led new mode, PRONET
	SW_LED_MODE7, //added by chiyokolin, for Lenovo
	HW_LED, // HW control 2 LEDs, LED0 and LED1 (there are 4 different control modes)
}LED_STRATEGY_8190, *PLED_STRATEGY_8190;
typedef enum _LED_PIN_8190{
	LED_PIN_GPIO0,
	LED_PIN_LED0,
	LED_PIN_LED1
}LED_PIN_8190;
typedef	enum _LED_STATE_8190{
	LED_UNKNOWN = 0,
	LED_ON = 1,
	LED_OFF = 2,
	LED_BLINK_NORMAL = 3,
	LED_BLINK_SLOWLY = 4,
	LED_POWER_ON_BLINK = 5,
	LED_SCAN_BLINK = 6, // LED is blinking during scanning period, the # of times to blink is depend on time for scanning.
	LED_NO_LINK_BLINK = 7, // LED is blinking during no link state.
	LED_BLINK_StartToBlink = 8, 
	LED_BLINK_TXRX = 9,
	LED_BLINK_RUNTOP = 10, // Customized for RunTop
	LED_BLINK_CAMEO = 11,
}LED_STATE_8190;
typedef struct _LED_8190{
	void *			dev;

	LED_PIN_8190	LedPin;	// Identify how to implement this SW led.

	LED_STATE_8190	CurrLedState; // Current LED state.
	bool				bLedOn; // TRUE if LED is ON, FALSE if LED is OFF.

	bool				bLedBlinkInProgress; // TRUE if it is blinking, FALSE o.w..

	bool				bLedSlowBlinkInProgress;//added by vivi, for led new mode
	u32				BlinkTimes; // Number of times to toggle led state for blinking.
	LED_STATE_8190	BlinkingLedState; // Next state for blinking, either LED_ON or LED_OFF are.

	struct timer_list 	BlinkTimer; // Timer object for led blinking.
} LED_8190, *PLED_8190;
#define CHANNEL_PLAN_LEN				10

#define sCrcLng 		4

//#ifdef RTL8192SE
typedef struct _TX_FWINFO_STRUCUTRE{
	//DOWRD 0
	u8			TxRate:7;
	u8			CtsEnable:1;
	u8			RtsRate:7;
	u8			RtsEnable:1;
	u8			TxHT:1;
	u8			Short:1;					
	u8			TxBandwidth:1;				
	u8			TxSubCarrier:2; 			
	u8			STBC:2;
	u8			AllowAggregation:1;
	u8			RtsHT:1;					
	u8			RtsShort:1; 				
	u8			RtsBandwidth:1; 			
	u8			RtsSubcarrier:2;			
	u8			RtsSTBC:2;
	u8			EnableCPUDur:1; 			
	
	//DWORD 1
	u32			RxMF:2;
	u32			RxAMD:3;
	u32			Reserved1:3;
	u32			TxAGCOffset:4;
	u32			TxAGCSign:1;
	u32			Tx_INFO_RSVD:6;
	u32			PacketID:13;
}TX_FWINFO_T;
//#endif

typedef struct _TX_FWINFO_8190PCI{
	//DOWRD 0
	u8			TxRate:7;
	u8			CtsEnable:1;
	u8			RtsRate:7;
	u8			RtsEnable:1;
	u8			TxHT:1;
	u8			Short:1;						//Short PLCP for CCK, or short GI for 11n MCS
	u8			TxBandwidth:1;				// This is used for HT MCS rate only.
	u8			TxSubCarrier:2; 			// This is used for legacy OFDM rate only.
	u8			STBC:2;
	u8			AllowAggregation:1;
	u8			RtsHT:1;						//Interpre RtsRate field as high throughput data rate
	u8			RtsShort:1; 				//Short PLCP for CCK, or short GI for 11n MCS
	u8			RtsBandwidth:1; 			// This is used for HT MCS rate only.
	u8			RtsSubcarrier:2;				// This is used for legacy OFDM rate only.
	u8			RtsSTBC:2;
	u8			EnableCPUDur:1; 			//Enable firmware to recalculate and assign packet duration
	
	//DWORD 1
	u32			RxMF:2;
	u32			RxAMD:3;
	u32			TxPerPktInfoFeedback:1; 	// 1: indicate that the transimission info of this packet should be gathered by Firmware and retured by Rx Cmd.
	u32			Reserved1:2;
	u32			TxAGCOffset:4;		// Only 90 support
	u32			TxAGCSign:1;		// Only 90 support
	u32			RAW_TXD:1;			// MAC will send data in txpktbuffer without any processing,such as CRC check
	u32			Retry_Limit:4;		// CCX Support relative retry limit FW page only support 4 bits now.
	u32			Reserved2:1;		
	u32			PacketID:13;

	// DW 2
	
}TX_FWINFO_8190PCI, *PTX_FWINFO_8190PCI;

#ifdef RTL8192E
typedef struct _phy_ofdm_rx_status_report_819xpci
{	
	u8	trsw_gain_X[4]; 
	u8	pwdb_all;
	u8	cfosho_X[4];
	u8	cfotail_X[4];
	u8	rxevm_X[2];
	u8	rxsnr_X[4];
	u8	pdsnr_X[2];
	u8	csi_current_X[2];
	u8	csi_target_X[2];
	u8	sigevm;
	u8	max_ex_pwr;
	u8	sgi_en;
	u8	rxsc_sgien_exflg;
}phy_sts_ofdm_819xpci_t;
#endif

typedef struct _phy_cck_rx_status_report_819xpci
{
	/* For CCK rate descriptor. This is a unsigned 8:1 variable. LSB bit presend
	   0.5. And MSB 7 bts presend a signed value. Range from -64~+63.5. */
	u8	adc_pwdb_X[4];
	u8	sq_rpt; 
	u8	cck_agc_rpt;
}phy_sts_cck_819xpci_t, phy_sts_cck_8192s_t;


typedef struct _phy_ofdm_rx_status_rxsc_sgien_exintfflag{
	u8			reserved:4;
	u8			rxsc:2; 
	u8			sgi_en:1;	
	u8			ex_intf_flag:1; 
}phy_ofdm_rx_status_rxsc_sgien_exintfflag;

typedef enum _RT_OP_MODE{
	RT_OP_MODE_AP,
	RT_OP_MODE_INFRASTRUCTURE,
	RT_OP_MODE_IBSS,
	RT_OP_MODE_NO_LINK,
}RT_OP_MODE, *PRT_OP_MODE;


/* 2007/11/02 MH Define RF mode temporarily for test. */
typedef enum tag_Rf_OpType
{    
    RF_OP_By_SW_3wire = 0,
    RF_OP_By_FW,   
    RF_OP_MAX
}RF_OpType_E;

typedef enum _RESET_TYPE {
	RESET_TYPE_NORESET = 0x00,
	RESET_TYPE_NORMAL = 0x01,
	RESET_TYPE_SILENT = 0x02
} RESET_TYPE;

typedef struct _tx_ring{
	u32 * desc;
	u8 nStuckCount;
	struct _tx_ring * next;
}__attribute__ ((packed)) tx_ring, * ptx_ring;

struct rtl8192_tx_ring {
    tx_desc *desc;
    dma_addr_t dma;
    unsigned int idx;
    unsigned int entries;
    struct sk_buff_head queue;
};

#define NIC_SEND_HANG_THRESHOLD_NORMAL		4        
#define NIC_SEND_HANG_THRESHOLD_POWERSAVE 	8
#define MAX_TX_QUEUE				9	// BK, BE, VI, VO, HCCA, MANAGEMENT, COMMAND, HIGH, BEACON. 

#define MAX_RX_COUNT                            64
#define MAX_TX_QUEUE_COUNT                      9        



//definded by WB. Ready to fill handlers for different NIC types.
//add handle here when necessary.
struct rtl819x_ops{
	nic_t nic_type;
	//init private variables
//	void (* init_priv_variable)(struct net_device* dev);
	//get eeprom types
	void (* get_eeprom_size)(struct net_device* dev);
//	u8 (* read_eeprom_info)(struct net_device* dev);
//	u8 (* read_dev_info)(struct net_device* dev);
	bool (* initialize_adapter)(struct net_device* dev);
	void (*link_change)(struct net_device* dev);
	void (* tx_fill_descriptor)(struct net_device* dev, tx_desc * tx_desc, cb_desc * cb_desc, struct sk_buff *skb);
	void (* tx_fill_cmd_descriptor)(struct net_device* dev, tx_desc_cmd * entry, cb_desc * cb_desc, struct sk_buff *skb); 
	bool (* rx_query_status_descriptor)(struct net_device* dev, struct ieee80211_rx_stats*  stats, rx_desc *pdesc, struct sk_buff* skb);
	void (* stop_adapter)(struct net_device *dev, bool reset);
};

typedef struct r8192_priv
{
	struct pci_dev *pdev;
	//add for handles different nics' operations. WB
	struct rtl819x_ops* ops;
	bool bfirst_init;
	RT_CUSTOMER_ID CustomerID;
	bool	being_init_adapter;
	//bool bDcut;
	u8	IC_Cut;
	int irq;
	short irq_enabled;
	struct ieee80211_device *ieee80211;
	u8 Rf_Mode;
	nic_t card_8192; /* 1: rtl8192E, 2:rtl8190, 4:8192SE */
	u8 card_8192_version; /* B,C,D or later cut */
//	short phy_ver; /* meaningful for rtl8225 1:A 2:B 3:C */
	short enable_gpio0;
	enum card_type {PCI,MINIPCI,CARDBUS,USB}card_type;
	short hw_plcp_len;
	short plcp_preamble_mode;
	u8 ScanDelay;	
	spinlock_t irq_lock;
	spinlock_t irq_th_lock;
	spinlock_t tx_lock;
	spinlock_t rf_ps_lock;
#ifdef RTL8192SE_CONFIG_ASPM_OR_D3
	spinlock_t D3_lock;
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	struct semaphore mutex;
#else
        struct mutex mutex;
#endif
	spinlock_t rf_lock; //used to lock rf write operation added by wb
	spinlock_t ps_lock;

	u32 irq_mask;
//	short irq_enabled;
//	struct net_device *dev; //comment this out.
	short chan;
	short sens;
	short max_sens;
	u32 rx_prevlen;
/*RX stuff*/
        rx_desc *rx_ring;
        dma_addr_t rx_ring_dma;
        unsigned int rx_idx;
        struct sk_buff *rx_buf[MAX_RX_COUNT];
	int rxringcount;
	u16 rxbuffersize;


	struct sk_buff *rx_skb;
	u32 *rxring;
	u32 *rxringtail;
	dma_addr_t rxringdma;
	struct buffer *rxbuffer;
	struct buffer *rxbufferhead;
	short rx_skb_complete;
/*TX stuff*/
        struct rtl8192_tx_ring tx_ring[MAX_TX_QUEUE_COUNT];
	int txringcount;
//{
	int txbuffsize;
	int txfwbuffersize;
	//struct tx_pendingbuf txnp_pending;
	//struct tasklet_struct irq_tx_tasklet;
	struct tasklet_struct irq_rx_tasklet;
	struct tasklet_struct irq_tx_tasklet;
        struct tasklet_struct irq_prepare_beacon_tasklet;
	struct buffer *txmapbufs;
	struct buffer *txbkpbufs;
	struct buffer *txbepbufs;
	struct buffer *txvipbufs;	
	struct buffer *txvopbufs;
	struct buffer *txcmdbufs;
	struct buffer *txmapbufstail;
	struct buffer *txbkpbufstail;
	struct buffer *txbepbufstail;
	struct buffer *txvipbufstail;      
	struct buffer *txvopbufstail;
	struct buffer *txcmdbufstail;
	/* adhoc/master mode stuff */
	ptx_ring txbeaconringtail;
	dma_addr_t txbeaconringdma;
	ptx_ring txbeaconring;
	int txbeaconcount;
	struct buffer *txbeaconbufs;
	struct buffer *txbeaconbufstail;
	ptx_ring txmapring;
	ptx_ring txbkpring;
	ptx_ring txbepring;
	ptx_ring txvipring;
	ptx_ring txvopring;
	ptx_ring txcmdring;
	ptx_ring txmapringtail;
	ptx_ring txbkpringtail;
	ptx_ring txbepringtail;
	ptx_ring txvipringtail;
	ptx_ring txvopringtail;
	ptx_ring txcmdringtail;
	ptx_ring txmapringhead;
	ptx_ring txbkpringhead;
	ptx_ring txbepringhead;
	ptx_ring txvipringhead;
	ptx_ring txvopringhead;
	ptx_ring txcmdringhead;
	dma_addr_t txmapringdma;
	dma_addr_t txbkpringdma;
	dma_addr_t txbepringdma;
	dma_addr_t txvipringdma;
	dma_addr_t txvopringdma;
	dma_addr_t txcmdringdma;
	//	u8 chtxpwr[15]; //channels from 1 to 14, 0 not used
//	u8 chtxpwr_ofdm[15]; //channels from 1 to 14, 0 not used
//	u8 cck_txpwr_base;
//	u8 ofdm_txpwr_base;
//	u8 challow[15]; //channels from 1 to 14, 0 not used
	short up;
	short crcmon; //if 1 allow bad crc frame reception in monitor mode
//	short prism_hdr;
	
//	struct timer_list scan_timer;
	/*short scanpending;
	short stopscan;*/
//	spinlock_t scan_lock;
//	u8 active_probe;
	//u8 active_scan_num;
	struct semaphore wx_sem;
	struct semaphore rf_sem; //used to lock rf write operation added by wb, modified by david
//	short hw_wep;
		
//	short digphy;
//	short antb;
//	short diversity;
//	u8 cs_treshold;
//	short rcr_csense;
	u8 rf_type; //0 means 1T2R, 1 means 2T4R
	RT_RF_TYPE_819xU rf_chip;
	
//	u32 key0[4];
	short (*rf_set_sens)(struct net_device *dev,short sens);
	u8 (*rf_set_chan)(struct net_device *dev,u8 ch);
	void (*rf_close)(struct net_device *dev);
	void (*rf_init)(struct net_device *dev);
	//short rate;
	short promisc;	
	/*stats*/
	struct Stats stats;
	struct iw_statistics wstats;
	struct proc_dir_entry *dir_dev;
	
	/*RX stuff*/
//	u32 *rxring;
//	u32 *rxringtail;
//	dma_addr_t rxringdma;
	
#ifdef THOMAS_BEACON
	u32 *oldaddr;
#endif
#ifdef THOMAS_TASKLET
	atomic_t irt_counter;//count for irq_rx_tasklet
#endif
#ifdef JACKSON_NEW_RX
        struct sk_buff **pp_rxskb;
        int     rx_inx;
#endif

/* modified by davad for Rx process */
       struct sk_buff_head rx_queue;
       struct sk_buff_head skb_queue;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	struct tq_struct qos_activate;
#else
       struct work_struct qos_activate;
#endif
	short  tx_urb_index;
	atomic_t tx_pending[0x10];//UART_PRIORITY+1

	struct urb *rxurb_task;

	//2 Tx Related variables
	u16	ShortRetryLimit;
	u16	LongRetryLimit;
	u32	TransmitConfig;
	u8	RegCWinMin;		// For turbo mode CW adaptive. Added by Annie, 2005-10-27.

	u32     LastRxDescTSFHigh;
	u32     LastRxDescTSFLow;


	//2 Rx Related variables
	u16	EarlyRxThreshold;
	u32	ReceiveConfig;
	u8	AcmControl;

	u8	RFProgType;
	
	u8 retry_data;
	u8 retry_rts;
	u16 rts;

	struct 	ChnlAccessSetting  ChannelAccessSetting;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
	struct work_struct reset_wq;
#else
	struct tq_struct reset_wq;
#endif

/**********************************************************/
//for rtl819xPci
	// Data Rate Config. Added by Annie, 2006-04-13.
	u16	basic_rate;
	u8	short_preamble;
	u8 	slot_time;
	u16 SifsTime;
/* WirelessMode*/
	u8 RegWirelessMode;
/*Firmware*/
	prt_firmware		pFirmware;
	rtl819x_loopback_e	LoopbackMode;
	firmware_source_e	firmware_source;
	u16 rf_pathmap;
	bool AutoloadFailFlag;
	//added for maintain info from eeprom
	short epromtype;
	u16 eeprom_vid;
	u16 eeprom_did;
	u16 eeprom_svid;
	u16 eeprom_smid;
	u8  eeprom_CustomerID;
	u16  eeprom_ChannelPlan;
	u8 eeprom_version;
	//efuse map
	u8					EfuseMap[2][HWSET_MAX_SIZE_92S];
	u16					EfuseUsedBytes;
	u8					EfuseUsedPercentage;

#if (EFUSE_REPG_WORKAROUND == 1)
	bool					efuse_RePGSec1Flag;
	u8					efuse_RePGData[8];
#endif
#if (EEPROM_OLD_FORMAT_SUPPORT == 1)
	u8					EEPROMTxPowerLevelCCK[14];		// CCK 2.4G channel 1~14
	u8					EEPROMTxPowerLevelOFDM24G[14];	// OFDM 2.4G channel 1~14
	u8					EEPROMTxPowerLevelOFDM5G[24];	// OFDM 5G
#else
	// F92S new definition
	//RF-A&B CCK/OFDM Tx Power Level at three channel are [1-3] [4-9] [10-14]
	u8					RfCckChnlAreaTxPwr[2][3];	
	u8					RfOfdmChnlAreaTxPwr1T[2][3];	
	u8					RfOfdmChnlAreaTxPwr2T[2][3];	
#endif	
	// For EEPROM TX Power Index like 8190 series
	u8					EEPROMRfACCKChnl1TxPwLevel[3];	//RF-A CCK Tx Power Level at channel 7
	u8					EEPROMRfAOfdmChnlTxPwLevel[3];//RF-A CCK Tx Power Level at [0],[1],[2] = channel 1,7,13
	u8					EEPROMRfCCCKChnl1TxPwLevel[3];	//RF-C CCK Tx Power Level at channel 7
	u8					EEPROMRfCOfdmChnlTxPwLevel[3];//RF-C CCK Tx Power Level at [0],[1],[2] = channel 1,7,13

	u16 EEPROMTxPowerDiff;
	u16 EEPROMAntPwDiff;		// Antenna gain offset from B/C/D to A
	u8 EEPROMThermalMeter;
	u8 EEPROMPwDiff;
	u8 EEPROMCrystalCap;
	u8 EEPROMBluetoothCoexist;	
	u8 EEPROMBoardType;
	u8 EEPROM_Def_Ver;
	u8 EEPROMHT2T_TxPwr[6];			// For channel 1, 7 and 13 on path A/B.
	u8 EEPROMTSSI_A;
	u8 EEPROMTSSI_B;
	// The following definition is for eeprom 93c56
//	u8 EEPROMRfACCKChnl1TxPwLevel[3];	//RF-A CCK Tx Power Level at channel 7
//	u8 EEPROMRfAOfdmChnlTxPwLevel[3];//RF-A CCK Tx Power Level at [0],[1],[2] = channel 1,7,13
//	u8 EEPROMRfCCCKChnl1TxPwLevel[3];	//RF-C CCK Tx Power Level at channel 7
//	u8 EEPROMRfCOfdmChnlTxPwLevel[3];//RF-C CCK Tx Power Level at [0],[1],[2] = channel 1,7,13
	u8 EEPROMTxPowerLevelCCK_V1[3];
//	u8 EEPROMTxPowerLevelOFDM24G[14]; // OFDM 2.4G channel 1~14
//	u8 EEPROMTxPowerLevelOFDM5G[24];	// OFDM 5G
	u8 EEPROMLegacyHTTxPowerDiff;	// Legacy to HT rate power diff
	bool bTXPowerDataReadFromEEPORM;
/*channel plan*/
	u16 RegChannelPlan; // Channel Plan specifed by user, 15: following setting of EEPROM, 0-14: default channel plan index specified by user.
	u16 ChannelPlan;
/*PS related*/
	bool RegRfOff;
	bool isRFOff;//added by amy for IPS 090331
	bool bInPowerSaveMode;//added by amy for IPS 090331
	// Rf off action for power save
	u8	bHwRfOffAction;	//0:No action, 1:By GPIO, 2:By Disable
/*PHY related*/
	BB_REGISTER_DEFINITION_T	PHYRegDef[4];	//Radio A/B/C/D
	// Read/write are allow for following hardware information variables
	u8					pwrGroupCnt;
#ifdef RTL8192SE
	u32	MCSTxPowerLevelOriginalOffset[3][7];
#else
	u32	MCSTxPowerLevelOriginalOffset[6];
#endif
	u32	CCKTxPowerLevelOriginalOffset;
	u8	TxPowerLevelCCK[14];			// CCK channel 1~14
	u8	TxPowerLevelCCK_A[14];			// RF-A, CCK channel 1~14
	u8 	TxPowerLevelCCK_C[14];
	u8	TxPowerLevelOFDM24G[14];		// OFDM 2.4G channel 1~14
	u8	TxPowerLevelOFDM5G[14];			// OFDM 5G
	u8	TxPowerLevelOFDM24G_A[14];	// RF-A, OFDM 2.4G channel 1~14 
	u8	TxPowerLevelOFDM24G_C[14];	// RF-C, OFDM 2.4G channel 1~14
	u8	LegacyHTTxPowerDiff;			// Legacy to HT rate power diff
	u8	TxPowerDiff;
	s8	RF_C_TxPwDiff;					// Antenna gain offset, rf-c to rf-a
	s8	RF_B_TxPwDiff;
	u8 	RfTxPwrLevelCck[2][14];
	u8	RfTxPwrLevelOfdm1T[2][14];
	u8	RfTxPwrLevelOfdm2T[2][14];
	u8	AntennaTxPwDiff[3];				// Antenna gain offset, index 0 for B, 1 for C, and 2 for D
		// 2009/01/20 MH Add for new EEPROM format.
	u8	TxPwrHt20Diff[2][14];				// HT 20<->40 Pwr diff
	u8	TxPwrLegacyHtDiff[2][14];		// For HT<->legacy pwr diff
#if 0//cosa, remove band-edge related stuff by SD3's request
	u8	TxPwrbandEdgeHt40[2][2];		// Band edge for HY 40MHZlow/up channel
	u8	TxPwrbandEdgeHt20[2][2];		// Band edge for HY 40MHZ low/up channel
	u8	TxPwrbandEdgeLegacyOfdm[2][2];	// Band edge for legacy ofdm low/up channel
#endif
	u8	TxPwrSafetyFlag;				// Band edge enable flag
			// Antenna gain offset, rf-c to rf-a
	u8	HT2T_TxPwr_A[14]; 				// For channel 1~14 on path A.
	u8	HT2T_TxPwr_B[14]; 				// For channel 1~14 on path B.
	//the current Tx power level
	u8	CurrentCckTxPwrIdx;
	u8 	CurrentOfdm24GTxPwrIdx;

	u8	CrystalCap;						// CrystalCap.
	u8	ThermalMeter[2];	// ThermalMeter, index 0 for RFIC0, and 1 for RFIC1
	u8      BluetoothCoexist;
	//05/27/2008 cck power enlarge
	u8	CckPwEnl;
	u16	TSSI_13dBm;
	u32 	Pwr_Track;
	u8				CCKPresentAttentuation_20Mdefault;
	u8				CCKPresentAttentuation_40Mdefault;
	char				CCKPresentAttentuation_difference;
	char				CCKPresentAttentuation;
	// Use to calculate PWBD.
	u8	bCckHighPower;
	long	undecorated_smoothed_pwdb;
	long	undecorated_smoothed_cck_adc_pwdb[4];
	//for set channel 
	u8	SwChnlInProgress;
	u8 	SwChnlStage;
	u8	SwChnlStep;
	u8	SetBWModeInProgress;
	HT_CHANNEL_WIDTH		CurrentChannelBW;

	// 8190 40MHz mode
	//
	u8	nCur40MhzPrimeSC;	// Control channel sub-carrier
	// Joseph test for shorten RF configuration time.
	// We save RF reg0 in this variable to reduce RF reading.
	//
	u32					RfReg0Value[4];
	u8 					NumTotalRFPath;	
	bool 				brfpath_rxenable[4];
//+by amy 080507
	struct timer_list watch_dog_timer;	

//+by amy 080515 for dynamic mechenism
	//Add by amy Tx Power Control for Near/Far Range 2008/05/15
	bool	bdynamic_txpower;  //bDynamicTxPower
	bool	bDynamicTxHighPower;  // Tx high power state
	bool	bDynamicTxLowPower;  // Tx low power state
	bool	bLastDTPFlag_High;
	bool	bLastDTPFlag_Low;
	
	bool	bstore_last_dtpflag;
	bool	bstart_txctrl_bydtp;   //Define to discriminate on High power State or on sitesuvey to change Tx gain index	
	//Add by amy for Rate Adaptive
	rate_adaptive rate_adaptive;
	//Add by amy for TX power tracking
	//2008/05/15  Mars OPEN/CLOSE TX POWER TRACKING
       txbbgain_struct txbbgain_table[TxBBGainTableLength];
	u8			   txpower_count;//For 6 sec do tracking again
	bool			   btxpower_trackingInit;
	u8			   OFDM_index;
	u8			   CCK_index;
	u8			   Record_CCK_20Mindex;
	u8			   Record_CCK_40Mindex;
	//2007/09/10 Mars Add CCK TX Power Tracking
	ccktxbbgain_struct	cck_txbbgain_table[CCKTxBBGainTableLength];
	ccktxbbgain_struct	cck_txbbgain_ch14_table[CCKTxBBGainTableLength];
	u8 rfa_txpowertrackingindex;
	u8 rfa_txpowertrackingindex_real;
	u8 rfa_txpowertracking_default;
	u8 rfc_txpowertrackingindex;
	u8 rfc_txpowertrackingindex_real;
	u8 rfc_txpowertracking_default;
	bool btxpower_tracking;
	bool bcck_in_ch14;

	//For Backup Initial Gain
	init_gain initgain_backup;
	u8 		DefaultInitialGain[4];
	// For EDCA Turbo mode, Added by amy 080515.
	bool		bis_any_nonbepkts;
	bool		bcurrent_turbo_EDCA;

	bool SetFwCmdInProgress; //is set FW CMD in Progress? 92S only
	u8 CurrentFwCmdIO;
	
		// L1 and L2 high power threshold.
	u8 	MidHighPwrTHR_L1;
	u8 	MidHighPwrTHR_L2;


	bool		bis_cur_rdlstate;
	struct timer_list fsync_timer;
	bool bfsync_processing;	// 500ms Fsync timer is active or not
	u32 	rate_record;
	u32 	rateCountDiffRecord;
	u32	ContiuneDiffCount;
	bool bswitch_fsync;

	u8	framesync;
	u32 	framesyncC34;
	u8   	framesyncMonitor;
        	//Added by amy 080516  for RX related
	u16 	nrxAMPDU_size;
	u8 	nrxAMPDU_aggr_num;
	
	/*Last RxDesc TSF value*/
	u32 last_rxdesc_tsf_high;
	u32 last_rxdesc_tsf_low;

	//by amy for gpio
	bool bHwRadioOff;
	bool pwrdown;
	//by amy for ps
	bool RFChangeInProgress; // RF Chnage in progress, by Bruce, 2007-10-30
	bool SetRFPowerStateInProgress;
	bool bdisable_nic;//added by amy for IPS 090408
	RT_OP_MODE OpMode;
	//by amy for reset_count
	u32 reset_count;
	bool bpbc_pressed;
	//by amy for debug
	u32 txpower_checkcnt;
	u32 txpower_tracking_callback_cnt;
	u8 thermal_read_val[40];
	u8 thermal_readback_index;
	u32 ccktxpower_adjustcnt_not_ch14;
	u32 ccktxpower_adjustcnt_ch14;
	u8 tx_fwinfo_force_subcarriermode;
	u8 tx_fwinfo_force_subcarrierval;

	//by amy for silent reset
	RESET_TYPE	ResetProgress;
	bool		bForcedSilentReset;
	bool		bDisableNormalResetCheck;
	u16		TxCounter;
	u16		RxCounter;
	int		IrpPendingCount;
	bool		bResetInProgress;
	bool		force_reset;
	u8		InitialGainOperateType;
	//by amy for LED control 090313
	//
	// LED control.
	//
	LED_STRATEGY_8190	LedStrategy;  
	LED_8190			SwLed0;
	LED_8190			SwLed1;
	//define work item by amy 080526
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	struct delayed_work update_beacon_wq;
	struct delayed_work watch_dog_wq;    
	struct delayed_work txpower_tracking_wq;
	struct delayed_work rfpath_check_wq;
	struct delayed_work gpio_change_rf_wq;
	struct delayed_work initialgain_operate_wq;
#else
	struct work_struct update_beacon_wq;
	struct work_struct watch_dog_wq;
	struct work_struct txpower_tracking_wq;
	struct work_struct rfpath_check_wq;
	struct work_struct gpio_change_rf_wq;
	struct work_struct initialgain_operate_wq;
#endif
	struct workqueue_struct *priv_wq;
#else
	struct tq_struct update_beacon_wq;
	/* used for periodly scan */
	struct tq_struct txpower_tracking_wq;
	struct tq_struct rfpath_check_wq;
	struct tq_struct watch_dog_wq;
	struct tq_struct gpio_change_rf_wq;
	struct tq_struct initialgain_operate_wq;
#endif
//#ifdef POLLING_METHOD_FOR_RADIO
	struct timer_list gpio_polling_timer;
	u8     polling_timer_on;
//#endif
}r8192_priv;

// for rtl8187
// now mirging to rtl8187B
/*
typedef enum{ 
	LOW_PRIORITY = 0x02,
	NORM_PRIORITY 
	} priority_t;
*/
//for rtl8187B
#if 0
typedef enum{
	BULK_PRIORITY = 0x01,
	//RSVD0,
	//RSVD1,
	LOW_PRIORITY,
	NORM_PRIORITY, 
	VO_PRIORITY,
	VI_PRIORITY, //0x05
	BE_PRIORITY,
	BK_PRIORITY,
	CMD_PRIORITY,//0x8
	RSVD3,
	BEACON_PRIORITY, //0x0A
	HIGH_PRIORITY,
	MANAGE_PRIORITY,
	RSVD4,
	RSVD5,
	UART_PRIORITY //0x0F
} priority_t;
#endif


#if 0 //defined in Qos.h
//typedef u32 AC_CODING;
#define AC0_BE	0		// ACI: 0x00	// Best Effort
#define AC1_BK	1		// ACI: 0x01	// Background
#define AC2_VI	2		// ACI: 0x10	// Video
#define AC3_VO	3		// ACI: 0x11	// Voice
#define AC_MAX	4		// Max: define total number; Should not to be used as a real enum.

//
// ECWmin/ECWmax field.
// Ref: WMM spec 2.2.2: WME Parameter Element, p.13.
//
typedef	union _ECW{
	u8	charData;
	struct
	{
		u8	ECWmin:4;
		u8	ECWmax:4;
	}f;	// Field
}ECW, *PECW;

//
// ACI/AIFSN Field.
// Ref: WMM spec 2.2.2: WME Parameter Element, p.12.
//
typedef	union _ACI_AIFSN{
	u8	charData;
	
	struct
	{
		u8	AIFSN:4;
		u8	ACM:1;
		u8	ACI:2;
		u8	Reserved:1;
	}f;	// Field
}ACI_AIFSN, *PACI_AIFSN;

//
// AC Parameters Record Format.
// Ref: WMM spec 2.2.2: WME Parameter Element, p.12.
//
typedef	union _AC_PARAM{
	u32	longData;
	u8	charData[4];

	struct
	{
		ACI_AIFSN	AciAifsn;
		ECW		Ecw;
		u16		TXOPLimit;
	}f;	// Field
}AC_PARAM, *PAC_PARAM;

#endif
#ifdef RTL8192SE
//static void rtl8192se_init_priv_variable(struct net_device* dev);
void rtl8192se_get_eeprom_size(struct net_device* dev);
bool rtl8192se_adapter_start(struct net_device* dev);
void rtl8192se_link_change(struct net_device *dev);
void  rtl8192se_tx_fill_cmd_desc(struct net_device* dev, tx_desc_cmd * entry, cb_desc * cb_desc, struct sk_buff* skb);
void  rtl8192se_tx_fill_desc(struct net_device* dev, tx_desc * pDesc, cb_desc * cb_desc, struct sk_buff* skb);
bool rtl8192se_rx_query_status_desc(struct net_device* dev, struct ieee80211_rx_stats*  stats, rx_desc *pdesc, struct sk_buff* skb);
void rtl8192se_rtx_disable(struct net_device *dev, bool bReset);
#else
//static void rtl8192_init_priv_variable(struct net_device* dev);
void rtl8192_get_eeprom_size(struct net_device* dev);
bool rtl8192_adapter_start(struct net_device *dev);
void rtl8192_link_change(struct net_device *dev);
void  rtl8192_tx_fill_cmd_desc(struct net_device* dev, tx_desc_cmd * entry, cb_desc * cb_desc, struct sk_buff* skb);
void  rtl8192_tx_fill_desc(struct net_device* dev, tx_desc * pdesc, cb_desc * cb_desc, struct sk_buff* skb);
bool rtl8192_rx_query_status_desc(struct net_device* dev, struct ieee80211_rx_stats*  stats, rx_desc *pdesc, struct sk_buff* skb);
void rtl8192_rtx_disable(struct net_device *dev, bool reset);
#endif

bool init_firmware(struct net_device *dev);
void rtl819xE_tx_cmd(struct net_device *dev, struct sk_buff *skb);
short rtl8192_tx(struct net_device *dev, struct sk_buff* skb);
u32 read_cam(struct net_device *dev, u8 addr);
void write_cam(struct net_device *dev, u8 addr, u32 data);
u8 read_nic_byte(struct net_device *dev, int x);
u8 read_nic_byte_E(struct net_device *dev, int x);
u32 read_nic_dword(struct net_device *dev, int x);
u16 read_nic_word(struct net_device *dev, int x) ;
void write_nic_byte(struct net_device *dev, int x,u8 y);
void write_nic_byte_E(struct net_device *dev, int x,u8 y);
void write_nic_word(struct net_device *dev, int x,u16 y);
void write_nic_dword(struct net_device *dev, int x,u32 y);
void force_pci_posting(struct net_device *dev);

void rtl8192_rx_enable(struct net_device *);
void rtl8192_tx_enable(struct net_device *);

void rtl8192_disassociate(struct net_device *dev);
//void fix_rx_fifo(struct net_device *dev);
void rtl8185_set_rf_pins_enable(struct net_device *dev,u32 a);

int rtl8192_hard_start_xmit(struct sk_buff *skb,struct net_device *dev);
void rtl8192_hard_data_xmit(struct sk_buff *skb, struct net_device *dev, int rate);
void rtl8192_data_hard_stop(struct net_device *dev);
void rtl8192_data_hard_resume(struct net_device *dev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_restart(struct work_struct *work);
#else
void rtl8192_restart(struct net_device *dev);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
extern  void    rtl819x_watchdog_wqcallback(struct work_struct *work);
#else
extern  void    rtl819x_watchdog_wqcallback(struct net_device *dev);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_hw_sleep_wq (struct work_struct *work);
#else
void rtl8192_hw_sleep_wq(struct net_device* dev);
#endif
void watch_dog_timer_callback(unsigned long data);
void rtl8192_irq_rx_tasklet(struct r8192_priv *priv);
void rtl8192_irq_tx_tasklet(struct r8192_priv *priv);
//void rtl8192_set_anaparam(struct net_device *dev,u32 a);
//void rtl8185_set_anaparam2(struct net_device *dev,u32 a);
void rtl8192_update_msr(struct net_device *dev);
int rtl8192_down(struct net_device *dev,bool shutdownrf);
int rtl8192_up(struct net_device *dev);
void rtl8192_commit(struct net_device *dev);
void rtl8192_set_chan(struct net_device *dev,short ch);

//short check_nic_enough_desc(struct net_device *dev, priority_t priority);
void rtl8192_start_beacon(struct net_device *dev);
void CamResetAllEntry(struct net_device* dev);
void EnableHWSecurityConfig8192(struct net_device *dev);
void setKey(struct net_device *dev, u8 EntryNo, u8 KeyIndex, u16 KeyType, u8 *MacAddr, u8 DefaultKey, u32 *KeyContent );
void CamPrintDbgReg(struct net_device* dev);
//extern	void	dm_cck_txpower_adjust(struct net_device *dev,bool  binch14);
extern void firmware_init_param(struct net_device *dev);
extern bool cmpk_message_handle_tx(struct net_device *dev, u8* codevirtualaddress, u32 packettype, u32 buffer_len);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_hw_wakeup_wq (struct work_struct *work);
#else
void rtl8192_hw_wakeup_wq(struct net_device *dev);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
void rtl8192_interrupt(int irq, void *netdev, struct pt_regs *regs);
#else
irqreturn_t rtl8192_interrupt(int irq, void *netdev, struct pt_regs *regs);
#endif
#else
irqreturn_t rtl8192_interrupt(int irq, void *netdev);
#endif

short rtl8192_pci_initdescring(struct net_device *dev);

void rtl8192_cancel_deferred_work(struct r8192_priv* priv);

int _rtl8192_up(struct net_device *dev);

void CamRestoreAllEntry(        struct net_device *dev);


short rtl8192_is_tx_queue_empty(struct net_device *dev);
#if defined(RTL8192E) || defined(RTL8192SE)
void rtl8192_hw_wakeup(struct net_device* dev);
void rtl8192_hw_to_sleep(struct net_device *dev, u32 th, u32 tl);
void ieee80211_ips_leave(struct net_device *dev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void IPSLeave_wq (struct work_struct *work);
#else
void IPSLeave_wq(struct net_device *dev);
#endif
#endif
//added by amy for led and ps 090319
#ifdef RTL8192SE
void InitSwLeds(struct net_device *dev);
void DeInitSwLeds(struct net_device *dev);
void LedControl8192SE(struct net_device *dev, LED_CTL_MODE LedAction);
void rtl8192_irq_disable(struct net_device *dev);
void SetHwReg8192SE(struct net_device *dev,u8 variable,u8* val);//added by amy 090330
void SwLedOn(struct net_device *dev , PLED_8190 pLed);//added by amy 090408
void SwLedOff(struct net_device *dev, PLED_8190 pLed);
#endif
#ifdef ENABLE_IPS
void IPSEnter(struct net_device *dev);
void IPSLeave(struct net_device *dev);
#endif
#ifdef ENABLE_LPS
void LeisurePSEnter(struct net_device *dev);
void LeisurePSLeave(struct net_device *dev);
#endif
void check_rfctrl_gpio_timer(unsigned long data);
bool NicIFEnableNIC(struct net_device* dev);//added by amy for IPS 090407
RT_STATUS NicIFDisableNIC(struct net_device* dev);//added by amy for IPS 090407
u8 HalSetSysClk8192SE(struct net_device *dev, u8 Data);
void gen_RefreshLedState(struct net_device *dev);
#define IS_HARDWARE_TYPE_8192SE(dev) (((struct r8192_priv*)ieee80211_priv(dev))->card_8192 == NIC_8192SE)
#endif


