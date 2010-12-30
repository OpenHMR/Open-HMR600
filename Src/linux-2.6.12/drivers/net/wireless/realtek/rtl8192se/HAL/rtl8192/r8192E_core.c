/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 * Linux device driver for RTL8190P / RTL8192E
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
 * Jerry chuang <wlanfae@realtek.com>
 */

#ifndef CONFIG_FORCE_HARD_FLOAT
double __floatsidf (int i) { return i; }
unsigned int __fixunsdfsi (double d) { return d; }
double __adddf3(double a, double b) { return a+b; }
double __addsf3(float a, float b) { return a+b; }
double __subdf3(double a, double b) { return a-b; }
double __extendsfdf2(float a) {return a;}
#endif

#undef LOOP_TEST
#undef RX_DONT_PASS_UL
#undef DEBUG_EPROM
#undef DEBUG_RX_VERBOSE
#undef DUMMY_RX
#undef DEBUG_ZERO_RX
#undef DEBUG_RX_SKB
#undef DEBUG_TX_FRAG
#undef DEBUG_RX_FRAG
#undef DEBUG_TX_FILLDESC
#undef DEBUG_TX
#undef DEBUG_IRQ
#undef DEBUG_RX
#undef DEBUG_RXALLOC
#undef DEBUG_REGISTERS
#undef DEBUG_RING
#undef DEBUG_IRQ_TASKLET
#undef DEBUG_TX_ALLOC
#undef DEBUG_TX_DESC

//#define CONFIG_RTL8192_IO_MAP
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#ifdef RTL8192SE
//#include "r8192SE_hw.h"
#include "r8192E.h"
#include "r8192S_phy.h" 
#include "r8192S_phyreg.h"
#include "r8192S_rtl6052.h"
#include "r8192S_Efuse.h"
#else
//#include "r8192E_hw.h"
#include "r8192E.h"
#include "r819xE_phy.h" //added by WB 4.30.2008
#include "r819xE_phyreg.h"
#include "r8190_rtl8256.h" /* RTL8225 Radio frontend */
#include "r819xE_cmdpkt.h"
#endif

#include "r8180_93cx6.h"   /* Card EEPROM */
#include "r8192E_wx.h"
#include "r8192E_dm.h"
//#include "r8192xU_phyreg.h"
//#include <linux/usb.h>
// FIXME: check if 2.6.7 is ok

#ifdef CONFIG_PM_RTL
#include "r8192_pm.h"
#endif

#ifdef ENABLE_DOT11D
#include "dot11d.h"
#endif

//set here to open your trace code. //WB
u32 rt_global_debug_component = \
				COMP_INIT    	|
//				COMP_EPROM   	|
//				COMP_PHY	|
//				COMP_RF		|
//				COMP_FIRMWARE	|
				//COMP_TRACE	|
				COMP_DOWN	|
		//		COMP_SWBW	|
				COMP_SEC	|
//				COMP_MLME	|
				//COMP_PS		|
				//COMP_LPS	|
//				COMP_CMD	|
//				COMP_CH	|
		//		COMP_RECV	|
		//		COMP_SEND	|
		//		COMP_POWER	|
			//	COMP_EVENTS	|
			//	COMP_RESET	|
			//	COMP_CMDPKT	|
			//	COMP_POWER_TRACKING	|
                        // 	COMP_INTR       |
				COMP_ERR ; //always open err flags on
#ifndef PCI_DEVICE
#define PCI_DEVICE(vend,dev)\
	.vendor=(vend),.device=(dev),\
	.subvendor=PCI_ANY_ID,.subdevice=PCI_ANY_ID
#endif
static struct pci_device_id rtl8192_pci_id_tbl[] __devinitdata = {
#ifdef RTL8190P
	/* Realtek */
	/* Dlink */
	{ PCI_DEVICE(0x10ec, 0x8190) },
	/* Corega */
	{ PCI_DEVICE(0x07aa, 0x0045) },
	{ PCI_DEVICE(0x07aa, 0x0046) },
#elif defined(RTL8192E)
	/* Realtek */
	{ PCI_DEVICE(0x10ec, 0x8192) },
	
	/* Corega */
	{ PCI_DEVICE(0x07aa, 0x0044) },
	{ PCI_DEVICE(0x07aa, 0x0047) },
#else	/*8192SE*/
	{PCI_DEVICE(0x10ec, 0x8192)},
	{PCI_DEVICE(0x10ec, 0x8171)},
	{PCI_DEVICE(0x10ec, 0x8172)},
	{PCI_DEVICE(0x10ec, 0x8173)},
	{PCI_DEVICE(0x10ec, 0x8174)},
#endif
	{}
};

static char* ifname = "wlan%d";
#if 0
static int hwseqnum = 0;
static int hwwep = 0;
#endif
static int hwwep = 1; //default use hw. set 0 to use software security
static int channels = 0x3fff;

MODULE_LICENSE("GPL");
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
MODULE_VERSION("V 1.1");
#endif
MODULE_DEVICE_TABLE(pci, rtl8192_pci_id_tbl);
//MODULE_AUTHOR("Andrea Merello <andreamrl@tiscali.it>");
MODULE_DESCRIPTION("Linux driver for Realtek RTL819x WiFi cards");

#if 0
MODULE_PARM(ifname,"s");
MODULE_PARM_DESC(devname," Net interface name, wlan%d=default");

MODULE_PARM(hwseqnum,"i");
MODULE_PARM_DESC(hwseqnum," Try to use hardware 802.11 header sequence numbers. Zero=default");

MODULE_PARM(hwwep,"i");
MODULE_PARM_DESC(hwwep," Try to use hardware WEP support. Still broken and not available on all cards");

MODULE_PARM(channels,"i");
MODULE_PARM_DESC(channels," Channel bitmask for specific locales. NYI");
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9)
module_param(ifname, charp, S_IRUGO|S_IWUSR );
//module_param(hwseqnum,int, S_IRUGO|S_IWUSR);
module_param(hwwep,int, S_IRUGO|S_IWUSR);
module_param(channels,int, S_IRUGO|S_IWUSR);
#else
MODULE_PARM(ifname, "s");
//MODULE_PARM(hwseqnum,"i");
MODULE_PARM(hwwep,"i");
MODULE_PARM(channels,"i");
#endif

MODULE_PARM_DESC(ifname," Net interface name, wlan%d=default");
//MODULE_PARM_DESC(hwseqnum," Try to use hardware 802.11 header sequence numbers. Zero=default");
MODULE_PARM_DESC(hwwep," Try to use hardware WEP support. Still broken and not available on all cards");
MODULE_PARM_DESC(channels," Channel bitmask for specific locales. NYI");

static int __devinit rtl8192_pci_probe(struct pci_dev *pdev,
			 const struct pci_device_id *id);
static void __devexit rtl8192_pci_disconnect(struct pci_dev *pdev);

static struct pci_driver rtl8192_pci_driver = {
	.name		= RTL819xE_MODULE_NAME,	          /* Driver name   */
	.id_table	= rtl8192_pci_id_tbl,	          /* PCI_ID table  */
	.probe		= rtl8192_pci_probe,	          /* probe fn      */
	.remove		= __devexit_p(rtl8192_pci_disconnect),	  /* remove fn     */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 5, 0)
#ifdef CONFIG_PM_RTL
	.suspend	= rtl8192E_suspend,	          /* PM suspend fn */
	.resume		= rtl8192E_resume,                 /* PM resume fn  */
#else
	.suspend	= NULL,			          /* PM suspend fn */
	.resume      	= NULL,			          /* PM resume fn  */
#endif
#endif
};

#define TOTAL_CAM_ENTRY 32
#define CAM_CONTENT_COUNT 8
//add initialize hanles here WB 2008.12.19.
#ifdef RTL8192SE
struct rtl819x_ops rtl8192se_ops = {
	.nic_type = NIC_8192SE,
//	.init_priv_variable = rtl8192se_init_priv_variable,
	.get_eeprom_size = rtl8192se_get_eeprom_size,
//	.read_eeprom_info = rtl8192se_read_eeprom_info,
//	.read_adapter_info = rtl8192se_adapter_start,
	.initialize_adapter = rtl8192se_adapter_start,
	.link_change = rtl8192se_link_change,
	.tx_fill_descriptor = rtl8192se_tx_fill_desc,
	.tx_fill_cmd_descriptor = rtl8192se_tx_fill_cmd_desc,
	.rx_query_status_descriptor = rtl8192se_rx_query_status_desc,
	.stop_adapter = rtl8192se_rtx_disable,
};
#else
struct rtl819x_ops rtl819xp_ops = {
	.nic_type = NIC_UNKNOWN,
//	.init_priv_variable = rtl8192_init_priv_variable,
	.get_eeprom_size = rtl8192_get_eeprom_size,
//	.read_eeprom_info = rtl8192_read_eeprom_info,
//	.read_adapter_info = 
	.initialize_adapter = rtl8192_adapter_start,
	.link_change = rtl8192_link_change,
	.tx_fill_descriptor = rtl8192_tx_fill_desc,
	.tx_fill_cmd_descriptor = rtl8192_tx_fill_cmd_desc,
	.rx_query_status_descriptor = rtl8192_rx_query_status_desc,
	.stop_adapter = rtl8192_rtx_disable,
};
#endif


#ifdef ENABLE_DOT11D

typedef struct _CHANNEL_LIST
{
	u8	Channel[32];
	u8	Len;
}CHANNEL_LIST, *PCHANNEL_LIST;

static CHANNEL_LIST ChannelPlan[] = {
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,149,153,157,161,165},24},  		//FCC
	{{1,2,3,4,5,6,7,8,9,10,11},11},                    				//IC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64},21},  	//ETSI
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},    //Spain. Change to ETSI. 
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},  	//France. Change to ETSI.
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},	//MKK					//MKK
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},//MKK1
	{{1,2,3,4,5,6,7,8,9,10,11,12,13},13},	//Israel.
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64},22},			// For 11a , TELEC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64}, 22},    //MIC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14},14}					//For Global Domain. 1-11:active scan, 12-14 passive scan. //+YJ, 080626
};

static void rtl819x_set_channel_map(u8 channel_plan, struct r8192_priv* priv)
{
	int i, max_chan=-1, min_chan=-1;
	struct ieee80211_device* ieee = priv->ieee80211;
	switch (channel_plan)
	{
		case COUNTRY_CODE_FCC:
		case COUNTRY_CODE_IC:
		case COUNTRY_CODE_ETSI:
		case COUNTRY_CODE_SPAIN:
		case COUNTRY_CODE_FRANCE:
		case COUNTRY_CODE_MKK:
		case COUNTRY_CODE_MKK1:
		case COUNTRY_CODE_ISRAEL:
		case COUNTRY_CODE_TELEC:
		case COUNTRY_CODE_MIC:
		{
			Dot11d_Init(ieee);
			ieee->bGlobalDomain = false;
                        //acturally 8225 & 8256 rf chip only support B,G,24N mode
                        if ((priv->rf_chip == RF_8225) || (priv->rf_chip == RF_8256) || (priv->rf_chip == RF_6052))
			{
				min_chan = 1;
				max_chan = 14;
			}
			else
			{
				RT_TRACE(COMP_ERR, "unknown rf chip, can't set channel map in function:%s()\n", __FUNCTION__);
			}
			if (ChannelPlan[channel_plan].Len != 0){
				// Clear old channel map
				memset(GET_DOT11D_INFO(ieee)->channel_map, 0, sizeof(GET_DOT11D_INFO(ieee)->channel_map));
				// Set new channel map
				for (i=0;i<ChannelPlan[channel_plan].Len;i++) 
				{
	                                if (ChannelPlan[channel_plan].Channel[i] < min_chan || ChannelPlan[channel_plan].Channel[i] > max_chan)
					    break; 	
					GET_DOT11D_INFO(ieee)->channel_map[ChannelPlan[channel_plan].Channel[i]] = 1;
				}
			}
			break;
		}
		case COUNTRY_CODE_GLOBAL_DOMAIN:
		{
			GET_DOT11D_INFO(ieee)->bEnabled = 0; //this flag enabled to follow 11d country IE setting, otherwise, it shall follow global domain setting
			Dot11d_Reset(ieee);
			ieee->bGlobalDomain = true;
			break;
		}
		default:
			break;
	}
}
#endif


#define eqMacAddr(a,b) ( ((a)[0]==(b)[0] && (a)[1]==(b)[1] && (a)[2]==(b)[2] && (a)[3]==(b)[3] && (a)[4]==(b)[4] && (a)[5]==(b)[5]) ? 1:0 )
/* 2007/07/25 MH Defien temp tx fw info. */





/****************************************************************************
   -----------------------------IO STUFF-------------------------
*****************************************************************************/
/* basic IO functions*/
////////////////////////////////////////////////////////////
#ifdef CONFIG_RTL8180_IO_MAP

u8 read_nic_byte(struct net_device *dev, int x) 
{
        return 0xff&inb(dev->base_addr +x);
}

u32 read_nic_dword(struct net_device *dev, int x) 
{
        return inl(dev->base_addr +x);
}

u16 read_nic_word(struct net_device *dev, int x) 
{
        return inw(dev->base_addr +x);
}

void write_nic_byte(struct net_device *dev, int x,u8 y)
{
        outb(y&0xff,dev->base_addr +x);
}

void write_nic_word(struct net_device *dev, int x,u16 y)
{
        outw(y,dev->base_addr +x);
}

void write_nic_dword(struct net_device *dev, int x,u32 y)
{
        outl(y,dev->base_addr +x);
}

#else /* RTL_IO_MAP */

u8 read_nic_byte(struct net_device *dev, int x) 
{    
    u8 b = 0xff&readb((u8*)dev->mem_start +x);
    //printk("rb %08x = %02x\n", dev->mem_start +x, b);    
    return b;
}

u32 read_nic_dword(struct net_device *dev, int x) 
{   
    u32 dw = readl((u8*)dev->mem_start +x);
    //printk("rl %08x = %08x\n", dev->mem_start +x, dw);    
    return dw;
}

u16 read_nic_word(struct net_device *dev, int x) 
{    
    u16 w = readw((u8*)dev->mem_start +x);
    //printk("rw %08x = %04x\n", dev->mem_start +x, w);    
    return w;
}

void write_nic_byte(struct net_device *dev, int x,u8 y)
{        
    writeb(y,(u8*)dev->mem_start +x);
    //printk("wb %08x = %02x\n", dev->mem_start +x, y);            
	//udelay(20);
}

void write_nic_dword(struct net_device *dev, int x,u32 y)
{ 
    writel(y,(u8*)dev->mem_start +x);
    //printk("wl %08x = %08x\n", dev->mem_start +x, y);                    
	//udelay(20);
}

void write_nic_word(struct net_device *dev, int x,u16 y) 
{    
    writew(y,(u8*)dev->mem_start +x);
    //printk("ww %08x = %04x\n", dev->mem_start +x, y);                            
	//udelay(20);
}

#endif /* RTL_IO_MAP */


///////////////////////////////////////////////////////////

//u8 read_phy_cck(struct net_device *dev, u8 adr);
//u8 read_phy_ofdm(struct net_device *dev, u8 adr);
/* this might still called in what was the PHY rtl8185/rtl8192 common code 
 * plans are to possibilty turn it again in one common code...
 */
inline void force_pci_posting(struct net_device *dev)
{
}


/****************************************************************************
   -----------------------------PROCFS STUFF-------------------------
*****************************************************************************/
/*This part is related to PROC, which will record some statistics. */

static struct proc_dir_entry *rtl8192_proc = NULL;



static int proc_get_stats_ap(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	struct ieee80211_network *target;
	
	int len = 0;

        list_for_each_entry(target, &ieee->network_list, list) {

		len += snprintf(page + len, count - len,
                "%s ", target->ssid);

		if(target->wpa_ie_len>0 || target->rsn_ie_len>0){
	                len += snprintf(page + len, count - len,
        	        "WPA\n");
		}
		else{
                        len += snprintf(page + len, count - len,
                        "non_WPA\n");
                }
		 
        }
	
	*eof = 1;
	return len;
}

static int proc_get_registers_0(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0x000;
	
#ifdef RTL8192SE
	/* This dump the current register page */
	if(!IS_BB_REG_OFFSET_92S(page0)){
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len,
					"\nD:  %2x > ",n);
			for(i=0;i<16 && n<=max;i++,n++)
				len += snprintf(page + len, count - len,
						"%2.2x ",read_nic_byte(dev,(page0|n)));
		}
	}else
#endif
	{
		len += snprintf(page + len, count - len,
				"\n####################page %x##################\n ", (page0>>8));
		for(n=0;n<=max;)
		{
			len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
			for(i=0;i<4 && n<=max;n+=4,i++)
				len += snprintf(page + len, count - len,
						"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
		}
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}
static int proc_get_registers_1(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0x100;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n####################page %x##################\n ", (page0>>8));
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len,
				"\nD:  %2x > ",n);
		for(i=0;i<16 && n<=max;i++,n++)
			len += snprintf(page + len, count - len,
					"%2.2x ",read_nic_byte(dev,(page0|n)));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}
static int proc_get_registers_2(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0x200;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n####################page %x##################\n ", (page0>>8));
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len,
				"\nD:  %2x > ",n);
		for(i=0;i<16 && n<=max;i++,n++)
			len += snprintf(page + len, count - len,
					"%2.2x ",read_nic_byte(dev,(page0|n)));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}
static int proc_get_registers_3(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0x300;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n####################page %x##################\n ", (page0>>8));
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len,
				"\nD:  %2x > ",n);
		for(i=0;i<16 && n<=max;i++,n++)
			len += snprintf(page + len, count - len,
					"%2.2x ",read_nic_byte(dev,(page0|n)));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}static int proc_get_registers_8(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0x800;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n####################page %x##################\n ", (page0>>8));
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;

}
static int proc_get_registers_9(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0x900;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n####################page %x##################\n ", (page0>>8));
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
static int proc_get_registers_a(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0xa00;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n####################page %x##################\n ", (page0>>8));
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
static int proc_get_registers_b(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0xb00;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n####################page %x##################\n ", (page0>>8));
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
static int proc_get_registers_c(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0xc00;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n####################page %x##################\n ", (page0>>8));
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
static int proc_get_registers_d(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0xd00;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n####################page %x##################\n ", (page0>>8));
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
static int proc_get_registers_e(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n,page0;
			
	int max=0xff;
	page0 = 0xe00;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n####################page %x##################\n ", (page0>>8));
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_QueryBBReg(dev,(page0|n), bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}

static int proc_get_reg_rf_a(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n;
			
	int max=0xff;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n#################### RF-A ##################\n ");
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A,n, bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}

static int proc_get_reg_rf_b(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n;
			
	int max=0xff;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n#################### RF-B ##################\n ");
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_B, n, bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}

static int proc_get_reg_rf_c(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n;
			
	int max=0xff;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n#################### RF-C ##################\n ");
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_C, n, bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}

static int proc_get_reg_rf_d(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	
	int len = 0;
	int i,n;
			
	int max=0xff;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
			"\n#################### RF-D ##################\n ");
	for(n=0;n<=max;)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",n);
		for(i=0;i<4 && n<=max;n+=4,i++)
			len += snprintf(page + len, count - len,
					"%8.8x ",rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_D, n, bMaskDWord));
	}
	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}

static int proc_get_cam_register(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	u32 target_command=0;
	u32 target_content=0;
	u8 entry_i=0;
	u32 ulStatus;
	int len = 0;
	int i=100, j = 0;

	/* This dump the current register page */
	len += snprintf(page + len, count - len,
				"\n#################### SECURITY CAM ##################\n ");
	for(j=0; j<TOTAL_CAM_ENTRY; j++)
	{
		len += snprintf(page + len, count - len, "\nD:  %2x > ",j);
	 	for(entry_i=0;entry_i<CAM_CONTENT_COUNT;entry_i++)
	 	{
	   		// polling bit, and No Write enable, and address
			target_command= entry_i+CAM_CONTENT_COUNT*j;
			target_command= target_command | BIT31;

			//Check polling bit is clear
			while((i--)>=0)
			{
				ulStatus = read_nic_dword(dev, RWCAM);
				if(ulStatus & BIT31){
					continue;
				}
				else{
					break;
				}
			}
	  		write_nic_dword(dev, RWCAM, target_command);
	  	 	target_content = read_nic_dword(dev, RCAMO);
			len += snprintf(page + len, count - len,"%8.8x ",target_content);
	 	}
	}

	len += snprintf(page + len, count - len,"\n");
	*eof = 1;
	return len;
}
static int proc_get_stats_tx(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"TX VI priority ok int: %lu\n"
//		"TX VI priority error int: %lu\n"
		"TX VO priority ok int: %lu\n"
//		"TX VO priority error int: %lu\n"
		"TX BE priority ok int: %lu\n"
//		"TX BE priority error int: %lu\n"
		"TX BK priority ok int: %lu\n"
//		"TX BK priority error int: %lu\n"
		"TX MANAGE priority ok int: %lu\n"
//		"TX MANAGE priority error int: %lu\n"
		"TX BEACON priority ok int: %lu\n"
		"TX BEACON priority error int: %lu\n"
		"TX CMDPKT priority ok int: %lu\n"
//		"TX high priority ok int: %lu\n"
//		"TX high priority failed error int: %lu\n"
//		"TX queue resume: %lu\n"
		"TX queue stopped?: %d\n"
		"TX fifo overflow: %lu\n"
//		"TX beacon: %lu\n"
//		"TX VI queue: %d\n"
//		"TX VO queue: %d\n"
//		"TX BE queue: %d\n"
//		"TX BK queue: %d\n"
//		"TX HW queue: %d\n"
//		"TX VI dropped: %lu\n"
//		"TX VO dropped: %lu\n"
//		"TX BE dropped: %lu\n"
//		"TX BK dropped: %lu\n"
		"TX total data packets %lu\n"		
		"TX total data bytes :%lu\n",
//		"TX beacon aborted: %lu\n",
		priv->stats.txviokint,
//		priv->stats.txvierr,
		priv->stats.txvookint,
//		priv->stats.txvoerr,
		priv->stats.txbeokint,
//		priv->stats.txbeerr,
		priv->stats.txbkokint,
//		priv->stats.txbkerr,
		priv->stats.txmanageokint,
//		priv->stats.txmanageerr,
		priv->stats.txbeaconokint,
		priv->stats.txbeaconerr,
		priv->stats.txcmdpktokint,
//		priv->stats.txhpokint,
//		priv->stats.txhperr,
//		priv->stats.txresumed,
		netif_queue_stopped(dev),
		priv->stats.txoverflow,
//		priv->stats.txbeacon,
//		atomic_read(&(priv->tx_pending[VI_QUEUE])),
//		atomic_read(&(priv->tx_pending[VO_QUEUE])),
//		atomic_read(&(priv->tx_pending[BE_QUEUE])),
//		atomic_read(&(priv->tx_pending[BK_QUEUE])),
//		read_nic_byte(dev, TXFIFOCOUNT),
//		priv->stats.txvidrop,
//		priv->stats.txvodrop,
		priv->ieee80211->stats.tx_packets,
		priv->ieee80211->stats.tx_bytes


//		priv->stats.txbedrop,
//		priv->stats.txbkdrop
			//	priv->stats.txdatapkt
//		priv->stats.txbeaconerr
		);
			
	*eof = 1;
	return len;
}		



static int proc_get_stats_rx(char *page, char **start,
			  off_t offset, int count,
			  int *eof, void *data)
{
	struct net_device *dev = data;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	
	int len = 0;
	
	len += snprintf(page + len, count - len,
		"RX packets: %lu\n"
		"RX desc err: %lu\n"
		"RX rx overflow error: %lu\n"
		"RX invalid urb error: %lu\n",
		priv->stats.rxint,
		priv->stats.rxrdu,
		priv->stats.rxoverflow,
		priv->stats.rxurberr);
			
	*eof = 1;
	return len;
}		

void rtl8192_proc_module_init(void)
{	
	RT_TRACE(COMP_INIT, "Initializing proc filesystem");
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
	rtl8192_proc=create_proc_entry(RTL819xE_MODULE_NAME, S_IFDIR, proc_net);
#else
	rtl8192_proc=create_proc_entry(RTL819xE_MODULE_NAME, S_IFDIR, init_net.proc_net);
#endif
}


void rtl8192_proc_module_remove(void)
{
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
	remove_proc_entry(RTL819xE_MODULE_NAME, proc_net);
#else
	remove_proc_entry(RTL819xE_MODULE_NAME, init_net.proc_net);
#endif
}


void rtl8192_proc_remove_one(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	printk("dev name=======> %s\n",dev->name);

	if (priv->dir_dev) {
		remove_proc_entry("stats-tx", priv->dir_dev);
		remove_proc_entry("stats-rx", priv->dir_dev);
		remove_proc_entry("stats-ap", priv->dir_dev);
		remove_proc_entry("registers-0", priv->dir_dev);
		remove_proc_entry("registers-1", priv->dir_dev);
		remove_proc_entry("registers-2", priv->dir_dev);
		remove_proc_entry("registers-3", priv->dir_dev);
		remove_proc_entry("registers-8", priv->dir_dev);
		remove_proc_entry("registers-9", priv->dir_dev);
		remove_proc_entry("registers-a", priv->dir_dev);
		remove_proc_entry("registers-b", priv->dir_dev);
		remove_proc_entry("registers-c", priv->dir_dev);
		remove_proc_entry("registers-d", priv->dir_dev);
		remove_proc_entry("registers-e", priv->dir_dev);
		remove_proc_entry("RF-A", priv->dir_dev);
		remove_proc_entry("RF-B", priv->dir_dev);
		remove_proc_entry("RF-C", priv->dir_dev);
		remove_proc_entry("RF-D", priv->dir_dev);
		remove_proc_entry("SEC-CAM", priv->dir_dev);
		remove_proc_entry("wlan0", rtl8192_proc);
		priv->dir_dev = NULL;
	}
}


void rtl8192_proc_init_one(struct net_device *dev)
{
	struct proc_dir_entry *e;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dir_dev = create_proc_entry(dev->name, 
					  S_IFDIR | S_IRUGO | S_IXUGO, 
					  rtl8192_proc);
	if (!priv->dir_dev) {
		RT_TRACE(COMP_ERR, "Unable to initialize /proc/net/rtl8192/%s\n",
		      dev->name);
		return;
	}
	e = create_proc_read_entry("stats-rx", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_rx, dev);
				   
	if (!e) {
		RT_TRACE(COMP_ERR,"Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-rx\n",
		      dev->name);
	}
	
	
	e = create_proc_read_entry("stats-tx", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_tx, dev);
				   
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-tx\n",
		      dev->name);
	}
	
	e = create_proc_read_entry("stats-ap", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_stats_ap, dev);
				   
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/stats-ap\n",
		      dev->name);
	}
	
	e = create_proc_read_entry("registers-0", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_0, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-0\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-1", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_1, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-1\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-2", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_2, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-2\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-3", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_3, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-3\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-8", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_8, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-8\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-9", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_9, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-9\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-a", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_a, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-a\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-b", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_b, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-b\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-c", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_c, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-c\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-d", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_d, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-d\n",
		      dev->name);
	}
	e = create_proc_read_entry("registers-e", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_registers_e, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/registers-e\n",
		      dev->name);
	}
	e = create_proc_read_entry("RF-A", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_reg_rf_a, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/RF-A\n",
		      dev->name);
	}
	e = create_proc_read_entry("RF-B", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_reg_rf_b, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/RF-B\n",
		      dev->name);
	}
	e = create_proc_read_entry("RF-C", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_reg_rf_c, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/RF-C\n",
		      dev->name);
	}
	e = create_proc_read_entry("RF-D", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_reg_rf_d, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/RF-D\n",
		      dev->name);
	}
	e = create_proc_read_entry("SEC-CAM", S_IFREG | S_IRUGO,
				   priv->dir_dev, proc_get_cam_register, dev);
	if (!e) {
		RT_TRACE(COMP_ERR, "Unable to initialize "
		      "/proc/net/rtl8192/%s/SEC-CAM\n",
		      dev->name);
	}
}
/****************************************************************************
   -----------------------------GENERAL FUNCTION-------------------------
*****************************************************************************/
/* This part of functions can be used for all of NICs without big changes.*/

/* this is only for debugging */
void print_buffer(u32 *buffer, int len)
{
	int i;
	u8 *buf =(u8*)buffer;
	
	printk("ASCII BUFFER DUMP (len: %x):\n",len);
	
	for(i=0;i<len;i++)
		printk("%c",buf[i]);
		
	printk("\nBINARY BUFFER DUMP (len: %x):\n",len);
	
	for(i=0;i<len;i++)
		printk("%x",buf[i]);

	printk("\n");
}

short check_nic_enough_desc(struct net_device *dev, int prio)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
    struct rtl8192_tx_ring *ring = &priv->tx_ring[prio];

    /* for now we reserve two free descriptor as a safety boundary 
     * between the tail and the head 
     */
    if (ring->entries - skb_queue_len(&ring->queue) >= 2) {
        return 1;
    } else {
        return 0;
    }
}

void tx_timeout(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//rtl8192_commit(dev);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
	schedule_work(&priv->reset_wq);
#else
	schedule_task(&priv->reset_wq);
#endif
	printk("TXTIMEOUT");
}


/* this is only for debug */
void dump_eprom(struct net_device *dev)
{
	int i;
	for(i=0; i<0xff; i++)
		RT_TRACE(COMP_INIT, "EEPROM addr %x : %x", i, eprom_read(dev,i));
}

/* this is only for debug */
void rtl8192_dump_reg(struct net_device *dev)
{
	int i;
	int n;
	int max=0x5ff;
	
	RT_TRACE(COMP_INIT, "Dumping NIC register map");	
	
	for(n=0;n<=max;)
	{
		printk( "\nD: %2x> ", n);
		for(i=0;i<16 && n<=max;i++,n++)
			printk("%2x ",read_nic_byte(dev,n));
	}
	printk("\n");
}

//notice INTA_MASK in 92SE will have 2 DW now. if second DW is used, add it here.
void rtl8192_irq_enable(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);	
	priv->irq_enabled = 1;
	write_nic_dword(dev,INTA_MASK, priv->irq_mask);
}


void rtl8192_irq_disable(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);	

	write_nic_dword(dev,INTA_MASK,0);
	write_nic_dword(dev,INTA_MASK+4,0);//added by amy 090407
	force_pci_posting(dev);
	priv->irq_enabled = 0;
}




void rtl8192_set_chan(struct net_device *dev,short ch)
{
    struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
    RT_TRACE(COMP_CH, "=====>%s()====ch:%d\n", __FUNCTION__, ch);	
    printk("=====>%s()====ch:%d\n", __FUNCTION__, ch);	
    priv->chan=ch;
#if 0
    if(priv->ieee80211->iw_mode == IW_MODE_ADHOC || 
            priv->ieee80211->iw_mode == IW_MODE_MASTER){

        priv->ieee80211->link_state = WLAN_LINK_ASSOCIATED;	
        priv->ieee80211->master_chan = ch;
        rtl8192_update_beacon_ch(dev); 
    }
#endif

    /* this hack should avoid frame TX during channel setting*/


    //	tx = read_nic_dword(dev,TX_CONF);
    //	tx &= ~TX_LOOPBACK_MASK;

#ifndef LOOP_TEST	
    //TODO 
    //	write_nic_dword(dev,TX_CONF, tx |( TX_LOOPBACK_MAC<<TX_LOOPBACK_SHIFT));

    //need to implement rf set channel here WB

    if (priv->rf_set_chan)
        priv->rf_set_chan(dev,priv->chan);
    //	mdelay(10);
    //	write_nic_dword(dev,TX_CONF,tx | (TX_LOOPBACK_NONE<<TX_LOOPBACK_SHIFT));
#endif
}

void rtl8192_update_msr(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 msr;
	LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;//added by amy 090316 for LED
	msr  = read_nic_byte(dev, MSR);
	msr &= ~ MSR_LINK_MASK;
	
	/* do not change in link_state != WLAN_LINK_ASSOCIATED.
	 * msr must be updated if the state is ASSOCIATING. 
	 * this is intentional and make sense for ad-hoc and
	 * master (see the create BSS/IBSS func)
	 */
	if (priv->ieee80211->state == IEEE80211_LINKED){ 
			
		if (priv->ieee80211->iw_mode == IW_MODE_INFRA){
			msr |= (MSR_LINK_MANAGED<<MSR_LINK_SHIFT);
			LedAction = LED_CTL_LINK;
		}
		else if (priv->ieee80211->iw_mode == IW_MODE_ADHOC){
			msr |= (MSR_LINK_ADHOC<<MSR_LINK_SHIFT);
			//set seperate
		}
		else if (priv->ieee80211->iw_mode == IW_MODE_MASTER){
			msr |= (MSR_LINK_MASTER<<MSR_LINK_SHIFT);
		}
	}else
		msr |= (MSR_LINK_NONE<<MSR_LINK_SHIFT);
		
	write_nic_byte(dev, MSR, msr);
	//added by amy for LED 090317
	if(priv->ieee80211->LedControlHandler)
		priv->ieee80211->LedControlHandler(dev, LedAction);
}


#define SHORT_SLOT_TIME 9
#define NON_SHORT_SLOT_TIME 20

void rtl8192_update_cap(struct net_device* dev, u16 cap)
{
	u32 tmp = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_network *net = &priv->ieee80211->current_network;
	priv->short_preamble = cap & WLAN_CAPABILITY_SHORT_PREAMBLE;
	tmp = priv->basic_rate;
	if (priv->short_preamble)
		tmp |= BRSR_AckShortPmb;
#ifndef RTL8192SE	//as 92se RRSR is no longer dw align 
	write_nic_dword(dev, RRSR, tmp);
#endif
	if (net->mode & (IEEE_G|IEEE_N_24G))
	{
		u8 slot_time = 0;
		if ((cap & WLAN_CAPABILITY_SHORT_SLOT)&&(!priv->ieee80211->pHTInfo->bCurrentRT2RTLongSlotTime))
		{//short slot time
			slot_time = SHORT_SLOT_TIME;
		}
		else //long slot time
			slot_time = NON_SHORT_SLOT_TIME;
		priv->slot_time = slot_time;
		write_nic_byte(dev, SLOT_TIME, slot_time);
	}

}


static struct ieee80211_qos_parameters def_qos_parameters = {
        {3,3,3,3},/* cw_min */
        {7,7,7,7},/* cw_max */
        {2,2,2,2},/* aifs */ 
        {0,0,0,0},/* flags */ 
        {0,0,0,0} /* tx_op_limit */
};

#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl8192_update_beacon(struct work_struct * work)
{
        struct r8192_priv *priv = container_of(work, struct r8192_priv, update_beacon_wq.work);
        struct net_device *dev = priv->ieee80211->dev;
#else
void rtl8192_update_beacon(struct net_device *dev)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
#endif
 	struct ieee80211_device* ieee = priv->ieee80211;
	struct ieee80211_network* net = &ieee->current_network;

	if (ieee->pHTInfo->bCurrentHTSupport)
		HTUpdateSelfAndPeerSetting(ieee, net);
	ieee->pHTInfo->bCurrentRT2RTLongSlotTime = net->bssht.bdRT2RTLongSlotTime;
	// Joseph test for turbo mode with AP
	ieee->pHTInfo->RT2RT_HT_Mode = net->bssht.RT2RT_HT_Mode;
	rtl8192_update_cap(dev, net->capability);
}
/*
* background support to run QoS activate functionality
*/
int WDCAPARA_ADD[] = {EDCAPARA_BE,EDCAPARA_BK,EDCAPARA_VI,EDCAPARA_VO};
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void rtl8192_qos_activate(struct work_struct * work)
{
        struct r8192_priv *priv = container_of(work, struct r8192_priv, qos_activate);
        struct net_device *dev = priv->ieee80211->dev;
#else
void rtl8192_qos_activate(struct net_device *dev)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
#endif
        struct ieee80211_qos_parameters *qos_parameters = &priv->ieee80211->current_network.qos_data.parameters;
        u8 mode = priv->ieee80211->current_network.mode;
//        u32 size = sizeof(struct ieee80211_qos_parameters);
	u8  u1bAIFS;
	u32 u4bAcParam;
        int i;
        if (priv == NULL)
                return;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	down(&priv->mutex);
#else
        mutex_lock(&priv->mutex);
#endif
        if(priv->ieee80211->state != IEEE80211_LINKED)
		goto success;
	RT_TRACE(COMP_QOS,"qos active process with associate response received\n");
	/* It better set slot time at first */
	/* For we just support b/g mode at present, let the slot time at 9/20 selection */	
	/* update the ac parameter to related registers */
	for(i = 0; i <  QOS_QUEUE_NUM; i++) {
		//Mode G/A: slotTimeTimer = 9; Mode B: 20
		u1bAIFS = qos_parameters->aifs[i] * ((mode&(IEEE_G|IEEE_N_24G)) ?9:20) + aSifsTime; 
		u4bAcParam = ((((u32)(qos_parameters->tx_op_limit[i]))<< AC_PARAM_TXOP_LIMIT_OFFSET)|
				(((u32)(qos_parameters->cw_max[i]))<< AC_PARAM_ECW_MAX_OFFSET)|
				(((u32)(qos_parameters->cw_min[i]))<< AC_PARAM_ECW_MIN_OFFSET)|
				((u32)u1bAIFS << AC_PARAM_AIFS_OFFSET));
		printk("===>u4bAcParam:%x, ", u4bAcParam);
		//write_nic_dword(dev, WDCAPARA_ADD[i], u4bAcParam);
		write_nic_dword(dev, WDCAPARA_ADD[i], 0x005e4332);
	}

success:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	up(&priv->mutex);
#else
        mutex_unlock(&priv->mutex);
#endif
}

static int rtl8192_qos_handle_probe_response(struct r8192_priv *priv,
		int active_network,
		struct ieee80211_network *network)
{
	int ret = 0;
	u32 size = sizeof(struct ieee80211_qos_parameters);

	if(priv->ieee80211->state !=IEEE80211_LINKED)
                return ret;

        if ((priv->ieee80211->iw_mode != IW_MODE_INFRA))
                return ret;

	if (network->flags & NETWORK_HAS_QOS_MASK) {
		if (active_network &&
				(network->flags & NETWORK_HAS_QOS_PARAMETERS))
			network->qos_data.active = network->qos_data.supported;

		if ((network->qos_data.active == 1) && (active_network == 1) &&
				(network->flags & NETWORK_HAS_QOS_PARAMETERS) &&
				(network->qos_data.old_param_count !=
				 network->qos_data.param_count)) {
			network->qos_data.old_param_count =
				network->qos_data.param_count;
                         priv->ieee80211->wmm_acm = network->qos_data.wmm_acm;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
			queue_work(priv->priv_wq, &priv->qos_activate);
#else
			schedule_task(&priv->qos_activate);
#endif
			RT_TRACE (COMP_QOS, "QoS parameters change call "
					"qos_activate\n");
		}
	} else {
		memcpy(&priv->ieee80211->current_network.qos_data.parameters,\
		       &def_qos_parameters, size);

		if ((network->qos_data.active == 1) && (active_network == 1)) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
			queue_work(priv->priv_wq, &priv->qos_activate);
#else
			schedule_task(&priv->qos_activate);
#endif
			RT_TRACE(COMP_QOS, "QoS was disabled call qos_activate \n");
		}
		network->qos_data.active = 0;
		network->qos_data.supported = 0;
	}

	return 0;
}

/* handle manage frame frame beacon and probe response */
static int rtl8192_handle_beacon(struct net_device * dev,
                              struct ieee80211_beacon * beacon,
                              struct ieee80211_network * network)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	rtl8192_qos_handle_probe_response(priv,1,network);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	queue_delayed_work(priv->priv_wq, &priv->update_beacon_wq, 0);
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	schedule_task(&priv->update_beacon_wq);
#else
	queue_work(priv->priv_wq, &priv->update_beacon_wq);
#endif
#endif
	return 0;

}

/*
* handling the beaconing responses. if we get different QoS setting
* off the network from the associated setting, adjust the QoS
* setting
*/
static int rtl8192_qos_association_resp(struct r8192_priv *priv,
                                    struct ieee80211_network *network)
{
        int ret = 0;
        unsigned long flags;
        u32 size = sizeof(struct ieee80211_qos_parameters);
        int set_qos_param = 0;

        if ((priv == NULL) || (network == NULL))
                return ret;

	if(priv->ieee80211->state !=IEEE80211_LINKED)
                return ret;

        if ((priv->ieee80211->iw_mode != IW_MODE_INFRA))
                return ret;

        spin_lock_irqsave(&priv->ieee80211->lock, flags);
	if(network->flags & NETWORK_HAS_QOS_PARAMETERS) {
		memcpy(&priv->ieee80211->current_network.qos_data.parameters,\
			 &network->qos_data.parameters,\
			sizeof(struct ieee80211_qos_parameters));
		priv->ieee80211->current_network.qos_data.active = 1;
                priv->ieee80211->wmm_acm = network->qos_data.wmm_acm;
#if 0
		if((priv->ieee80211->current_network.qos_data.param_count != \
					network->qos_data.param_count))
#endif
		 {
                        set_qos_param = 1;
			/* update qos parameter for current network */
			priv->ieee80211->current_network.qos_data.old_param_count = \
				 priv->ieee80211->current_network.qos_data.param_count;
			priv->ieee80211->current_network.qos_data.param_count = \
			     	 network->qos_data.param_count;
		}
        } else {
		memcpy(&priv->ieee80211->current_network.qos_data.parameters,\
		       &def_qos_parameters, size);
		priv->ieee80211->current_network.qos_data.active = 0;
		priv->ieee80211->current_network.qos_data.supported = 0;
                set_qos_param = 1;
        }

        spin_unlock_irqrestore(&priv->ieee80211->lock, flags);

	RT_TRACE(COMP_QOS, "%s: network->flags = %d,%d\n",__FUNCTION__,network->flags ,priv->ieee80211->current_network.qos_data.active);
	if (set_qos_param == 1)
	{
		dm_init_edca_turbo(priv->ieee80211->dev);//YJ,add,090320, when roaming to another ap, bcurrent_turbo_EDCA should be resetted so that we can update edca tubo.
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
		queue_work(priv->priv_wq, &priv->qos_activate);
#else
		schedule_task(&priv->qos_activate);
#endif
	}

        return ret;
}


static int rtl8192_handle_assoc_response(struct net_device *dev,
                                     struct ieee80211_assoc_response_frame *resp,
                                     struct ieee80211_network *network)
{
        struct r8192_priv *priv = ieee80211_priv(dev);
        rtl8192_qos_association_resp(priv, network);
        return 0;
}


void rtl8192_prepare_beacon(struct r8192_priv *priv)
{
	struct sk_buff *skb;
	//unsigned long flags;
	cb_desc *tcb_desc; 

	skb = ieee80211_get_beacon(priv->ieee80211);
	tcb_desc = (cb_desc *)(skb->cb + 8);
        //printk("===========> %s\n", __FUNCTION__);
	//spin_lock_irqsave(&priv->tx_lock,flags);
	/* prepare misc info for the beacon xmit */
	tcb_desc->queue_index = BEACON_QUEUE;
	/* IBSS does not support HT yet, use 1M defautly */
	tcb_desc->data_rate = 2;
	tcb_desc->RATRIndex = 7;
	tcb_desc->bTxDisableRateFallBack = 1;
	tcb_desc->bTxUseDriverAssingedRate = 1;
	
	skb_push(skb, priv->ieee80211->tx_headroom);	
	if(skb){
		rtl8192_tx(priv->ieee80211->dev,skb);
	}
	//spin_unlock_irqrestore (&priv->tx_lock, flags);
}

void rtl8192_stop_beacon(struct net_device *dev)
{
	//rtl8192_beacon_disable(dev);
}


void rtl8192_config_rate(struct net_device* dev, u16* rate_config)
{
	 struct r8192_priv *priv = ieee80211_priv(dev);
	 struct ieee80211_network *net;
	 u8 i=0, basic_rate = 0;
	 net = & priv->ieee80211->current_network;
	 
	 for (i=0; i<net->rates_len; i++)
	 {
		 basic_rate = net->rates[i]&0x7f;
		 switch(basic_rate)
		 {
			 case MGN_1M:	*rate_config |= RRSR_1M;	break;
			 case MGN_2M:	*rate_config |= RRSR_2M;	break;
			 case MGN_5_5M:	*rate_config |= RRSR_5_5M;	break;
			 case MGN_11M:	*rate_config |= RRSR_11M;	break;
			 case MGN_6M:	*rate_config |= RRSR_6M;	break;
			 case MGN_9M:	*rate_config |= RRSR_9M;	break;
			 case MGN_12M:	*rate_config |= RRSR_12M;	break;
			 case MGN_18M:	*rate_config |= RRSR_18M;	break;
			 case MGN_24M:	*rate_config |= RRSR_24M;	break;
			 case MGN_36M:	*rate_config |= RRSR_36M;	break;
			 case MGN_48M:	*rate_config |= RRSR_48M;	break;
			 case MGN_54M:	*rate_config |= RRSR_54M;	break;
		 }
	 }
	 for (i=0; i<net->rates_ex_len; i++)
	 {
		 basic_rate = net->rates_ex[i]&0x7f;
		 switch(basic_rate)
		 {
			 case MGN_1M:	*rate_config |= RRSR_1M;	break;
			 case MGN_2M:	*rate_config |= RRSR_2M;	break;
			 case MGN_5_5M:	*rate_config |= RRSR_5_5M;	break;
			 case MGN_11M:	*rate_config |= RRSR_11M;	break;
			 case MGN_6M:	*rate_config |= RRSR_6M;	break;
			 case MGN_9M:	*rate_config |= RRSR_9M;	break;
			 case MGN_12M:	*rate_config |= RRSR_12M;	break;
			 case MGN_18M:	*rate_config |= RRSR_18M;	break;
			 case MGN_24M:	*rate_config |= RRSR_24M;	break;
			 case MGN_36M:	*rate_config |= RRSR_36M;	break;
			 case MGN_48M:	*rate_config |= RRSR_48M;	break;
			 case MGN_54M:	*rate_config |= RRSR_54M;	break;
		 }
	 }
}

bool is_ap_in_wep_tkip(struct net_device*dev)
{
	static u8 ccmp_ie[4] = {0x00,0x50,0xf2,0x04};
	static u8 ccmp_rsn_ie[4] = {0x00, 0x0f, 0xac, 0x04};
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	int wpa_ie_len= ieee->wpa_ie_len;
	struct ieee80211_crypt_data* crypt;
	int encrypt;

	crypt = ieee->crypt[ieee->tx_keyidx];
	encrypt = (ieee->current_network.capability & WLAN_CAPABILITY_PRIVACY) ||\
		  (ieee->host_encrypt && crypt && crypt->ops && \
		   (0 == strcmp(crypt->ops->name,"WEP")));

	/* simply judge  */
	if(encrypt && (wpa_ie_len == 0)) {
		/* wep encryption, no N mode setting */
		return true;
	} else if((wpa_ie_len != 0)) {
		/* parse pairwise key type */
		if (((ieee->wpa_ie[0] == 0xdd) && (!memcmp(&(ieee->wpa_ie[14]),ccmp_ie,4))) || ((ieee->wpa_ie[0] == 0x30) && (!memcmp(&ieee->wpa_ie[10],ccmp_rsn_ie, 4))))
			return false;
		else
			return true;
	} else {
		//RT_TRACE(COMP_ERR,"In %s The GroupEncAlgorithm is [4]\n",__FUNCTION__ );
		return false;
	}

return false;
}

bool GetNmodeSupportBySecCfg8190Pci(struct net_device*dev)
{
#ifdef RTL8192SE
	return true;
#else
	return !is_ap_in_wep_tkip(dev);
#endif
}

void rtl8192_refresh_supportrate(struct r8192_priv* priv)
{
	struct ieee80211_device* ieee = priv->ieee80211;
	//we donot consider set support rate for ABG mode, only HT MCS rate is set here.
	if (ieee->mode == WIRELESS_MODE_N_24G || ieee->mode == WIRELESS_MODE_N_5G)
	{
		memcpy(ieee->Regdot11HTOperationalRateSet, ieee->RegHTSuppRateSet, 16);
		if(priv->rf_type == RF_1T1R) {
			ieee->Regdot11HTOperationalRateSet[1] = 0;
		}
		//RT_DEBUG_DATA(COMP_INIT, ieee->RegHTSuppRateSet, 16);
		//RT_DEBUG_DATA(COMP_INIT, ieee->Regdot11HTOperationalRateSet, 16);
	}
	else
		memset(ieee->Regdot11HTOperationalRateSet, 0, 16);
	return;
}

u8 rtl8192_getSupportedWireleeMode(struct net_device*dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 ret = 0;
	switch(priv->rf_chip)
	{
		case RF_8225:
		case RF_8256:
		case RF_6052:
		case RF_PSEUDO_11N:
			ret = (WIRELESS_MODE_N_24G|WIRELESS_MODE_G|WIRELESS_MODE_B);
			break;
		case RF_8258:
			ret = (WIRELESS_MODE_A|WIRELESS_MODE_N_5G);
			break;
		default:
			ret = WIRELESS_MODE_B;
			break;
	}
	return ret;
}
void rtl8192_SetWirelessMode(struct net_device* dev, u8 wireless_mode)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 bSupportMode = rtl8192_getSupportedWireleeMode(dev);
	printk("===>%s(), wireless_mode:%x, support_mode:%x\n", __FUNCTION__, wireless_mode, bSupportMode);
#if 1  
	if ((wireless_mode == WIRELESS_MODE_AUTO) || ((wireless_mode&bSupportMode)==0))
	{
		if(bSupportMode & WIRELESS_MODE_N_24G)
		{
			wireless_mode = WIRELESS_MODE_N_24G;
		}
		else if(bSupportMode & WIRELESS_MODE_N_5G)
		{
			wireless_mode = WIRELESS_MODE_N_5G;
		}
		else if((bSupportMode & WIRELESS_MODE_A))
		{
			wireless_mode = WIRELESS_MODE_A;
		}
		else if((bSupportMode & WIRELESS_MODE_G))
		{
			wireless_mode = WIRELESS_MODE_G;
		}
		else if((bSupportMode & WIRELESS_MODE_B))
		{
			wireless_mode = WIRELESS_MODE_B;
		}
		else{
			RT_TRACE(COMP_ERR, "%s(), No valid wireless mode supported, SupportedWirelessMode(%x)!!!\n", __FUNCTION__,bSupportMode);
			wireless_mode = WIRELESS_MODE_B;
		}
	}
#ifdef TO_DO_LIST //// TODO: this function doesn't work well at this time, we shoud wait for FPGA
	ActUpdateChannelAccessSetting( pAdapter, pHalData->CurrentWirelessMode, &pAdapter->MgntInfo.Info8185.ChannelAccessSetting );
#endif
//set to G mode if AP is b,g mixed.
	if ((wireless_mode & (WIRELESS_MODE_B | WIRELESS_MODE_G)) == (WIRELESS_MODE_G | WIRELESS_MODE_B))
		wireless_mode = WIRELESS_MODE_G;
#ifdef RTL8192SE
	write_nic_word(dev, SIFS_OFDM, 0x0e0e); //
#endif
	priv->ieee80211->mode = wireless_mode;
	
	if ((wireless_mode == WIRELESS_MODE_N_24G) ||  (wireless_mode == WIRELESS_MODE_N_5G))
		priv->ieee80211->pHTInfo->bEnableHT = 1;	
	else
		priv->ieee80211->pHTInfo->bEnableHT = 0;
	RT_TRACE(COMP_INIT, "Current Wireless Mode is %x\n", wireless_mode);
	rtl8192_refresh_supportrate(priv);
#endif	

}
//init priv variables here

bool GetHalfNmodeSupportByAPs819xPci(struct net_device* dev)
{
#ifdef RTL8192SE
		return false;
#else	
	bool			Reval;
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	
	if(ieee->bHalfWirelessN24GMode == true)
		Reval = true;
	else
		Reval =  false;

	return Reval;
#endif
}


static void rtl8192_init_priv_variable(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));
	u8 i;
//#ifdef POLLING_METHOD_FOR_RADIO
	priv->polling_timer_on = 0;//add for S3/S4
//#endif	

//	priv->card_8192 = type; //recored device type
	priv->being_init_adapter = false;
	priv->bdisable_nic = false;//added by amy for IPS 090408
	priv->bfirst_init = false;
//	priv->txbuffsize = 1600;//1024;
//	priv->txfwbuffersize = 4096;
	priv->txringcount = 64;//32;
	//priv->txbeaconcount = priv->txringcount;
//	priv->txbeaconcount = 2;
	priv->rxbuffersize = 9100;//2048;//1024;
	priv->rxringcount = MAX_RX_COUNT;//64; 
	priv->irq_enabled=0;
//	priv->rx_skb_complete = 1;
	priv->chan = 1; //set to channel 1
	priv->RegWirelessMode = WIRELESS_MODE_AUTO;
	priv->RegChannelPlan = 0xf;
	priv->nrxAMPDU_size = 0;
	priv->nrxAMPDU_aggr_num = 0;
	priv->last_rxdesc_tsf_high = 0;
	priv->last_rxdesc_tsf_low = 0;
	priv->ieee80211->mode = WIRELESS_MODE_AUTO; //SET AUTO 
	priv->ieee80211->iw_mode = IW_MODE_INFRA;
	priv->ieee80211->ieee_up=0;
	priv->retry_rts = DEFAULT_RETRY_RTS;
	priv->retry_data = DEFAULT_RETRY_DATA;
	priv->ieee80211->rts = DEFAULT_RTS_THRESHOLD;
	priv->ieee80211->rate = 110; //11 mbps
	priv->ieee80211->short_slot = 1;
	priv->promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	priv->bcck_in_ch14 = false;
	priv->bfsync_processing  = false;
	priv->CCKPresentAttentuation = 0;
	priv->rfa_txpowertrackingindex = 0;
	priv->rfc_txpowertrackingindex = 0;
	priv->CckPwEnl = 6;
	priv->ScanDelay = 50;//for Scan TODO
	//added by amy for silent reset
	priv->ResetProgress = RESET_TYPE_NORESET;
	priv->bForcedSilentReset = 0;
	priv->bDisableNormalResetCheck = false;
	priv->force_reset = false;
	//added by amy for power save
	priv->bHwRadioOff = false;
	priv->RegRfOff = 0;
	priv->isRFOff = false;//added by amy for IPS 090331
	priv->bInPowerSaveMode = false;//added by amy for IPS 090331
	priv->ieee80211->RfOffReason = 0;
	priv->RFChangeInProgress = false;
	priv->bHwRfOffAction = 0;
	priv->SetRFPowerStateInProgress = false;
	priv->ieee80211->PowerSaveControl.bInactivePs = true;
	priv->ieee80211->PowerSaveControl.bIPSModeBackup = false;
	priv->ieee80211->PowerSaveControl.bLeisurePs = true;//added by amy for Leisure PS 090402
	priv->ieee80211->PowerSaveControl.bFwCtrlLPS = false;//added by amy for Leisure PS 090402
	//just for debug
	priv->txpower_checkcnt = 0;
	priv->thermal_readback_index =0;
	priv->txpower_tracking_callback_cnt = 0;
	priv->ccktxpower_adjustcnt_ch14 = 0;
	priv->ccktxpower_adjustcnt_not_ch14 = 0;
	priv->pwrGroupCnt = 0;
	
	priv->ieee80211->current_network.beacon_interval = DEFAULT_BEACONINTERVAL;	
	priv->ieee80211->iw_mode = IW_MODE_INFRA;
	priv->ieee80211->softmac_features  = IEEE_SOFTMAC_SCAN | 
		IEEE_SOFTMAC_ASSOCIATE | IEEE_SOFTMAC_PROBERQ | 
		IEEE_SOFTMAC_PROBERS | IEEE_SOFTMAC_TX_QUEUE;/* |
		IEEE_SOFTMAC_BEACONS;*///added by amy 080604 //|  //IEEE_SOFTMAC_SINGLE_QUEUE;
	
	priv->ieee80211->active_scan = 1;
	//added by amy 090313
	priv->ieee80211->be_scan_inprogress = false;
	priv->ieee80211->modulation = IEEE80211_CCK_MODULATION | IEEE80211_OFDM_MODULATION;
	priv->ieee80211->host_encrypt = 1;
	priv->ieee80211->host_decrypt = 1;
	//priv->ieee80211->start_send_beacons = NULL;//rtl819xusb_beacon_tx;//-by amy 080604
	//priv->ieee80211->stop_send_beacons = NULL;//rtl8192_beacon_stop;//-by amy 080604
	priv->ieee80211->start_send_beacons = rtl8192_start_beacon;//+by david 081107 
	priv->ieee80211->stop_send_beacons = rtl8192_stop_beacon;//+by david 081107
	priv->ieee80211->softmac_hard_start_xmit = rtl8192_hard_start_xmit;
	priv->ieee80211->set_chan = rtl8192_set_chan;
	priv->ieee80211->link_change = priv->ops->link_change;
	priv->ieee80211->softmac_data_hard_start_xmit = rtl8192_hard_data_xmit;
	priv->ieee80211->data_hard_stop = rtl8192_data_hard_stop;
	priv->ieee80211->data_hard_resume = rtl8192_data_hard_resume;
	priv->ieee80211->fts = DEFAULT_FRAG_THRESHOLD;
	priv->ieee80211->check_nic_enough_desc = check_nic_enough_desc;	
	priv->ieee80211->MaxMssDensity = 0;//added by amy 090330
	priv->ieee80211->MinSpaceCfg = 0;//added by amy 090330
#ifdef RTL8192SE
	priv->ieee80211->tx_headroom = 0;
#else
	priv->ieee80211->tx_headroom = sizeof(TX_FWINFO_8190PCI);
#endif
	priv->ieee80211->qos_support = 1;
	priv->ieee80211->dot11PowerSaveMode = eActive;
	//added by WB
//	priv->ieee80211->SwChnlByTimerHandler = rtl8192_phy_SwChnl;
	priv->ieee80211->SetBWModeHandler = rtl8192_SetBWMode;
	priv->ieee80211->handle_assoc_response = rtl8192_handle_assoc_response;
	priv->ieee80211->handle_beacon = rtl8192_handle_beacon;
#ifndef RTL8190P
	priv->ieee80211->sta_wake_up = rtl8192_hw_wakeup;
//	priv->ieee80211->ps_request_tx_ack = rtl8192_rq_tx_ack;
	priv->ieee80211->enter_sleep_state = rtl8192_hw_to_sleep;
	priv->ieee80211->ps_is_queue_empty = rtl8192_is_tx_queue_empty;
#else
	priv->ieee80211->sta_wake_up = NULL;//modified by amy for Legacy PS 090401
//	priv->ieee80211->ps_request_tx_ack = rtl8192_rq_tx_ack;
	priv->ieee80211->enter_sleep_state = NULL;//modified by amy for Legacy PS 090401
	priv->ieee80211->ps_is_queue_empty = NULL;//modified by amy for Legacy PS 090401
#endif
	//added by david
	priv->ieee80211->GetNmodeSupportBySecCfg = GetNmodeSupportBySecCfg8190Pci;
	priv->ieee80211->SetWirelessMode = rtl8192_SetWirelessMode;
	priv->ieee80211->GetHalfNmodeSupportByAPsHandler = GetHalfNmodeSupportByAPs819xPci;
	//added by amy
#ifdef RTL8192SE	
	priv->ieee80211->SetHwRegHandler = SetHwReg8192SE;
	priv->ieee80211->SetFwCmdHandler = rtl8192se_set_fw_cmd;
	// Default Halt the NIC if RF is OFF.
	pPSC->RegRfPsLevel |= RT_RF_OFF_LEVL_HALT_NIC;
#ifdef ENABLE_IPS
	priv->ieee80211->ieee80211_ips_leave = ieee80211_ips_leave;//added by amy for IPS 090331
#endif
	//priv->ieee80211->InitialGainHandler = PHY_InitialGain8192S;
#else
	priv->ieee80211->SetHwRegHandler = NULL;
	priv->ieee80211->SetFwCmdHandler = NULL;
	priv->ieee80211->InitialGainHandler = InitialGain819xPci;
#ifdef ENABLE_IPS
	priv->ieee80211->ieee80211_ips_leave = ieee80211_ips_leave;//added by amy for IPS 090331
#endif
#endif	
#ifdef ENABLE_LPS
	priv->ieee80211->LeisurePSLeave = LeisurePSLeave;
#endif
	priv->ieee80211->is_ap_in_wep_tkip = is_ap_in_wep_tkip;
#ifdef RTL8192SE
        priv->ieee80211->LedControlHandler = LedControl8192SE;
#else
        priv->ieee80211->LedControlHandler = NULL;
#endif
	priv->MidHighPwrTHR_L1 = 0x3B;
	priv->MidHighPwrTHR_L2 = 0x40;

	priv->card_type = PCI;
	{
		priv->ShortRetryLimit = 0x30;
		priv->LongRetryLimit = 0x30;
	}
	priv->EarlyRxThreshold = 7;
	priv->enable_gpio0 = 0;

	priv->TransmitConfig = 0;
#ifdef RTL8192SE	
	priv->ReceiveConfig = 
	RCR_APPFCS | RCR_APWRMGT | /*RCR_ADD3 |*/
	RCR_AMF	| RCR_ADF | RCR_APP_MIC | RCR_APP_ICV |
       RCR_AICV	| RCR_ACRC32	|				// Accept ICV error, CRC32 Error
	RCR_AB 		| RCR_AM		|				// Accept Broadcast, Multicast	 
     	RCR_APM 	|  								// Accept Physical match
     	/*RCR_AAP		|*/	 						// Accept Destination Address packets
     	RCR_APP_PHYST_STAFF | RCR_APP_PHYST_RXFF |	// Accept PHY status
	(priv->EarlyRxThreshold<<RCR_FIFO_OFFSET)	;
	// Make reference from WMAC code 2006.10.02, maybe we should disable some of the interrupt. by Emily
	
	priv->irq_mask = 
	(IMR_ROK | IMR_VODOK | IMR_VIDOK | IMR_BEDOK | IMR_BKDOK |		\
	IMR_HCCADOK | IMR_MGNTDOK | IMR_COMDOK | IMR_HIGHDOK | 					\
	IMR_BDOK | IMR_RXCMDOK | /*IMR_TIMEOUT0 |*/ IMR_RDU | IMR_RXFOVW	|			\
	IMR_BcnInt/* | IMR_TXFOVW*/ /*| IMR_TBDOK | IMR_TBDER*/);
#else
	priv->ReceiveConfig = RCR_ADD3	|
		RCR_AMF | RCR_ADF |		//accept management/data
		RCR_AICV |			//accept control frame for SW AP needs PS-poll, 2005.07.07, by rcnjko.
		RCR_AB | RCR_AM | RCR_APM |	//accept BC/MC/UC
		RCR_AAP | ((u32)7<<RCR_MXDMA_OFFSET) |
		((u32)7 << RCR_FIFO_OFFSET) | RCR_ONLYERLPKT;

	priv->irq_mask = 	(u32)(IMR_ROK | IMR_VODOK | IMR_VIDOK | IMR_BEDOK | IMR_BKDOK |\
				IMR_HCCADOK | IMR_MGNTDOK | IMR_COMDOK | IMR_HIGHDOK |\
				IMR_BDOK | IMR_RXCMDOK | IMR_TIMEOUT0 | IMR_RDU | IMR_RXFOVW	|\
				IMR_TXFOVW | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);
#endif
	priv->AcmControl = 0;	
	priv->pFirmware = (rt_firmware*)vmalloc(sizeof(rt_firmware));
	if (priv->pFirmware)
	memset(priv->pFirmware, 0, sizeof(rt_firmware));

	/* rx related queue */
        skb_queue_head_init(&priv->rx_queue);
	skb_queue_head_init(&priv->skb_queue);

	/* Tx related queue */
	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->ieee80211->skb_waitQ [i]);
	}
	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
		skb_queue_head_init(&priv->ieee80211->skb_aggQ [i]);
	}
	priv->rf_set_chan = rtl8192_phy_SwChnl;	
}	

//init lock here
static void rtl8192_init_priv_lock(struct r8192_priv* priv)
{
	spin_lock_init(&priv->tx_lock);
	spin_lock_init(&priv->irq_lock);//added by thomas
	spin_lock_init(&priv->irq_th_lock);
	spin_lock_init(&priv->rf_ps_lock);
	spin_lock_init(&priv->ps_lock);
	spin_lock_init(&priv->rf_lock);
#ifdef RTL8192SE_CONFIG_ASPM_OR_D3
	spin_lock_init(&priv->D3_lock);
#endif
	sema_init(&priv->wx_sem,1);
	sema_init(&priv->rf_sem,1);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16))
	sema_init(&priv->mutex, 1);
#else
	mutex_init(&priv->mutex);
#endif
}

//init tasklet and wait_queue here. only 2.6 above kernel is considered
#define DRV_NAME "wlan0"
static void rtl8192_init_priv_task(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);	

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
#ifdef PF_SYNCTHREAD
	priv->priv_wq = create_workqueue(DRV_NAME,0);
#else	
	priv->priv_wq = create_workqueue(DRV_NAME);
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
//	INIT_WORK(&priv->reset_wq, (void(*)(void*)) rtl8192_restart);
	INIT_WORK(&priv->reset_wq,  rtl8192_restart);
#ifdef ENABLE_IPS
	INIT_WORK(&priv->ieee80211->ips_leave_wq, (void*)IPSLeave_wq);  //added by amy for IPS 090331
#endif
//	INIT_DELAYED_WORK(&priv->watch_dog_wq, hal_dm_watchdog);
	INIT_DELAYED_WORK(&priv->watch_dog_wq, rtl819x_watchdog_wqcallback);
#ifndef RTL8192SE	
	INIT_DELAYED_WORK(&priv->txpower_tracking_wq,  dm_txpower_trackingcallback);
#endif
	INIT_DELAYED_WORK(&priv->rfpath_check_wq,  dm_rf_pathcheck_workitemcallback);
	INIT_DELAYED_WORK(&priv->update_beacon_wq, rtl8192_update_beacon);
	//INIT_WORK(&priv->SwChnlWorkItem,  rtl8192_SwChnl_WorkItem);
	//INIT_WORK(&priv->SetBWModeWorkItem,  rtl8192_SetBWModeWorkItem);
	INIT_WORK(&priv->qos_activate, rtl8192_qos_activate);
#ifndef RTL8190P	
	INIT_DELAYED_WORK(&priv->ieee80211->hw_wakeup_wq,(void*) rtl8192_hw_wakeup_wq);
	INIT_DELAYED_WORK(&priv->ieee80211->hw_sleep_wq,(void*) rtl8192_hw_sleep_wq);
#endif

#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	tq_init(&priv->reset_wq, (void*)rtl8192_restart, dev);
#ifdef ENABLE_IPS
	tq_init(&priv->ieee80211->ips_leave_wq,(void*) IPSLeave_wq,dev);//added by amy for IPS 090331
#endif
	tq_init(&priv->watch_dog_wq, (void*)rtl819x_watchdog_wqcallback, dev);
#ifndef RTL8192SE	
	tq_init(&priv->txpower_tracking_wq, (void*)dm_txpower_trackingcallback, dev);
#endif
	tq_init(&priv->rfpath_check_wq, (void*)dm_rf_pathcheck_workitemcallback, dev);
	tq_init(&priv->update_beacon_wq, (void*)rtl8192_update_beacon, dev);
	//tq_init(&priv->SwChnlWorkItem, (void*) rtl8192_SwChnl_WorkItem, dev);
	//tq_init(&priv->SetBWModeWorkItem, (void*)rtl8192_SetBWModeWorkItem, dev);
	tq_init(&priv->qos_activate, (void *)rtl8192_qos_activate, dev);
#ifndef RTL8190P
	tq_init(&priv->ieee80211->hw_wakeup_wq,(void*) rtl8192_hw_wakeup_wq, dev);
	tq_init(&priv->ieee80211->hw_sleep_wq,(void*) rtl8192_hw_sleep_wq, dev);
#endif

#else
	INIT_WORK(&priv->reset_wq,(void(*)(void*)) rtl8192_restart,dev);
#ifdef ENABLE_IPS
	INIT_WORK(&priv->ieee80211->ips_leave_wq, (void*)IPSLeave_wq,dev);//added by amy for IPS 090331
#endif
//	INIT_WORK(&priv->watch_dog_wq, (void(*)(void*)) hal_dm_watchdog,dev);
	INIT_WORK(&priv->watch_dog_wq, (void(*)(void*)) rtl819x_watchdog_wqcallback,dev);
#ifndef RTL8192SE
	INIT_WORK(&priv->txpower_tracking_wq, (void(*)(void*)) dm_txpower_trackingcallback,dev);
#endif
	INIT_WORK(&priv->rfpath_check_wq, (void(*)(void*)) dm_rf_pathcheck_workitemcallback,dev);
	INIT_WORK(&priv->update_beacon_wq, (void(*)(void*))rtl8192_update_beacon,dev);
	//INIT_WORK(&priv->SwChnlWorkItem, (void(*)(void*)) rtl8192_SwChnl_WorkItem, dev);
	//INIT_WORK(&priv->SetBWModeWorkItem, (void(*)(void*)) rtl8192_SetBWModeWorkItem, dev);
	INIT_WORK(&priv->qos_activate, (void(*)(void *))rtl8192_qos_activate, dev);
#ifndef RTL8190P
	INIT_WORK(&priv->ieee80211->hw_wakeup_wq,(void*) rtl8192_hw_wakeup_wq, dev);
	INIT_WORK(&priv->ieee80211->hw_sleep_wq,(void*) rtl8192_hw_sleep_wq, dev);
#endif
#endif
#endif

	tasklet_init(&priv->irq_rx_tasklet,
	     (void(*)(unsigned long))rtl8192_irq_rx_tasklet,
	     (unsigned long)priv);
	tasklet_init(&priv->irq_tx_tasklet,
	     (void(*)(unsigned long))rtl8192_irq_tx_tasklet,
	     (unsigned long)priv);
        tasklet_init(&priv->irq_prepare_beacon_tasklet,
                (void(*)(unsigned long))rtl8192_prepare_beacon,
                (unsigned long)priv);
}



//used to swap endian. as ntohl & htonl are not neccessary to swap endian, so use this instead.
static inline u16 endian_swap(u16* data)
{
	u16 tmp = *data;
	*data = (tmp >> 8) | (tmp << 8);
	return *data;	
}

/*
 *	Note:	dev->EEPROMAddressSize should be set before this function call.
 * 			EEPROM address size can be got through GetEEPROMSize8185()
*/



short rtl8192_get_channel_map(struct net_device * dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
#ifdef ENABLE_DOT11D
	if(priv->ChannelPlan> COUNTRY_CODE_GLOBAL_DOMAIN){
		printk("rtl8180_init:Error channel plan! Set to default.\n");
		priv->ChannelPlan= 0;
	}
	RT_TRACE(COMP_INIT, "Channel plan is %d\n",priv->ChannelPlan);
		
	rtl819x_set_channel_map(priv->ChannelPlan, priv);
#else
	int ch,i;
	//Set Default Channel Plan
	if(!channels){
		DMESG("No channels, aborting");
		return -1;
	}
	ch=channels;
	priv->ChannelPlan= 0;//hikaru
	 // set channels 1..14 allowed in given locale
	for (i=1; i<=14; i++) {
		(priv->ieee80211->channel_map)[i] = (u8)(ch & 0x01);
		ch >>= 1;
	}
#endif
	return 0;
}
void check_rfctrl_gpio_timer(unsigned long data);
short rtl8192_init(struct net_device *dev)
{		
	struct r8192_priv *priv = ieee80211_priv(dev);
	memset(&(priv->stats),0,sizeof(struct Stats));
	rtl8192_init_priv_variable(dev);
	rtl8192_init_priv_lock(priv);
	rtl8192_init_priv_task(dev);
	priv->ops->get_eeprom_size(dev);
	rtl8192_get_channel_map(dev);
	init_hal_dm(dev);
	// added by amy for LED 090313
	//
	// Prepare SW resource for LED.
	//
#ifdef RTL8192SE
	InitSwLeds(dev);
#endif
	
	init_timer(&priv->watch_dog_timer);
	priv->watch_dog_timer.data = (unsigned long)dev;
	priv->watch_dog_timer.function = watch_dog_timer_callback;

//#ifdef POLLING_METHOD_FOR_RADIO
	init_timer(&priv->gpio_polling_timer);
	priv->gpio_polling_timer.data = (unsigned long)dev;
	priv->gpio_polling_timer.function = check_rfctrl_gpio_timer;
//#endif

#if defined(IRQF_SHARED)
        if(request_irq(dev->irq, (void*)rtl8192_interrupt, IRQF_SHARED, dev->name, dev)){
#else
        if(request_irq(dev->irq, (void *)rtl8192_interrupt, SA_SHIRQ, dev->name, dev)){
#endif
		printk("Error allocating IRQ %d",dev->irq);
		return -1;
	}else{ 
		priv->irq=dev->irq;
		printk("IRQ %d",dev->irq);
	}
	if(rtl8192_pci_initdescring(dev)!=0){ 
		printk("Endopoints initialization failed");
		return -1;
	}

	//rtl8192_rx_enable(dev);
	//rtl8192_adapter_start(dev);
//#ifdef DEBUG_EPROM
//	dump_eprom(dev);
//#endif 
	//rtl8192_dump_reg(dev);	
	return 0;
}







#if 0
void rtl8192_beacon_tx_enable(struct net_device *dev)
{
	struct r8180_priv *priv = (struct r8180_priv *)ieee80211_priv(dev);

	rtl8180_set_mode(dev,EPROM_CMD_CONFIG);
#ifdef CONFIG_RTL8185B
	priv->dma_poll_stop_mask &= ~(TPPOLLSTOP_BQ);MgntQuery_MgntFrameTxRateMgntQuery_MgntFrameTxRate
	write_nic_byte(dev,TPPollStop, priv->dma_poll_mask);
#else
	priv->dma_poll_mask &=~(1<<TX_DMA_STOP_BEACON_SHIFT);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
#endif
	rtl8180_set_mode(dev,EPROM_CMD_NORMAL);
}
#endif


/* this configures registers for beacon tx and enables it via
 * rtl8192_beacon_tx_enable(). rtl8192_beacon_tx_disable() might
 * be used to stop beacon transmission
 */

void rtl8192_start_beacon(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct ieee80211_network *net = &priv->ieee80211->current_network;
	u16 BcnTimeCfg = 0;
        u16 BcnCW = 6;
        u16 BcnIFS = 0xf;

	DMESG("Enabling beacon TX");
	//rtl8192_prepare_beacon(dev);
	rtl8192_irq_disable(dev);
	//rtl8192_beacon_tx_enable(dev);

	/* ATIM window */
	write_nic_word(dev, ATIMWND, 2);

	/* Beacon interval (in unit of TU) */
	write_nic_word(dev, BCN_INTERVAL, net->beacon_interval);

	/* 
	 * DrvErlyInt (in unit of TU). 
	 * (Time to send interrupt to notify driver to c
	 * hange beacon content) 
	 * */
#ifdef RTL8192SE
	write_nic_word(dev, BCN_DRV_EARLY_INT, 10<<4);
#else
	write_nic_word(dev, BCN_DRV_EARLY_INT, 10);
#endif
	/* 
	 * BcnDMATIM(in unit of us). 
	 * Indicates the time before TBTT to perform beacon queue DMA 
	 * */
	write_nic_word(dev, BCN_DMATIME, 256); 

	/* 
	 * Force beacon frame transmission even after receiving 
	 * beacon frame from other ad hoc STA 
	 * */
	write_nic_byte(dev, BCN_ERR_THRESH, 100);	
	
	/* Set CW and IFS */
	BcnTimeCfg |= BcnCW<<BCN_TCFG_CW_SHIFT;
	BcnTimeCfg |= BcnIFS<<BCN_TCFG_IFS;
	write_nic_word(dev, BCN_TCFG, BcnTimeCfg);
	

	/* enable the interrupt for ad-hoc process */
	rtl8192_irq_enable(dev);
}
/***************************************************************************
    -------------------------------WATCHDOG STUFF---------------------------
***************************************************************************/

short rtl8192_is_tx_queue_empty(struct net_device *dev)
{
	int i=0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	for (i=0; i<=MGNT_QUEUE; i++)
	{
		if ((i== TXCMD_QUEUE) || (i == HCCA_QUEUE) )
			continue;
		if (skb_queue_len(&(&priv->tx_ring[i])->queue) > 0){
			printk("===>tx queue is not empty:%d, %d\n", i, skb_queue_len(&(&priv->tx_ring[i])->queue));
			return 0;
		}
	}
	return 1;
}
#if 0	
void rtl8192_rq_tx_ack(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	priv->ieee80211->ack_tx_to_ieee = 1;
}
#endif

#if defined(RTL8192E) || defined(RTL8192SE)
void rtl8192_hw_sleep_down(struct net_device *dev)
{
	RT_TRACE(COMP_PS, "%s()============>come to sleep down\n", __FUNCTION__);
	MgntActSet_RF_State(dev, eRfSleep, RF_CHANGE_BY_PS);
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_hw_sleep_wq (struct work_struct *work)
{
        struct delayed_work *dwork = container_of(work,struct delayed_work,work);
        struct ieee80211_device *ieee = container_of(dwork,struct ieee80211_device,hw_sleep_wq);
        struct net_device *dev = ieee->dev;
#else
void rtl8192_hw_sleep_wq(struct net_device* dev)
{
#endif
	//printk("=========>%s()\n", __FUNCTION__);
        rtl8192_hw_sleep_down(dev);
}
//	printk("dev is %d\n",dev);	
//	printk("&*&(^*(&(&=========>%s()\n", __FUNCTION__);
void rtl8192_hw_wakeup(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	unsigned long flags = 0;
//	spin_lock_irqsave(&priv->ps_lock,flags);
	spin_lock_irqsave(&priv->rf_ps_lock,flags);
	if(priv->RFChangeInProgress)
	{
		spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
		RT_TRACE(COMP_RF, "rtl8192_hw_wakeup(): RF Change in progress! \n");
		printk("rtl8192_hw_wakeup(): RF Change in progress! schedule wake up task again\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
		schedule_task(&priv->ieee80211->hw_wakeup_wq);
#else
		queue_delayed_work(priv->ieee80211->wq, &priv->ieee80211->hw_wakeup_wq, MSECS(10)); 
#endif
		return;
	}
	spin_unlock_irqrestore(&priv->rf_ps_lock,flags);
	RT_TRACE(COMP_PS, "%s()============>come to wake up\n", __FUNCTION__);
	MgntActSet_RF_State(dev, eRfOn, RF_CHANGE_BY_PS);
	//FIXME: will we send package stored while nic is sleep?
//	spin_unlock_irqrestore(&priv->ps_lock,flags);
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_hw_wakeup_wq (struct work_struct *work)
{
//	struct r8180_priv *priv = container_of(work, struct r8180_priv, watch_dog_wq);
//	struct ieee80211_device * ieee = (struct ieee80211_device*)
//	                                       container_of(work, struct ieee80211_device, watch_dog_wq);
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
	struct ieee80211_device *ieee = container_of(dwork,struct ieee80211_device,hw_wakeup_wq);  
	struct net_device *dev = ieee->dev;
#else
void rtl8192_hw_wakeup_wq(struct net_device* dev)
{
#endif
	rtl8192_hw_wakeup(dev);

}

#define MIN_SLEEP_TIME 50
#define MAX_SLEEP_TIME 10000
void rtl8192_hw_to_sleep(struct net_device *dev, u32 th, u32 tl)
{

	struct r8192_priv *priv = ieee80211_priv(dev);

	u32 rb = jiffies;
	unsigned long flags;
	
	spin_lock_irqsave(&priv->ps_lock,flags);
	
	/* Writing HW register with 0 equals to disable
	 * the timer, that is not really what we want
	 */
	tl -= MSECS(8+16+7);
	//tl -= MSECS(4+16+7);
	 
	//if(tl == 0) tl = 1;
	
	/* FIXME HACK FIXME HACK */
//	force_pci_posting(dev);
	//mdelay(1);
	
//	rb = read_nic_dword(dev, TSFTR);
	
	/* If the interval in witch we are requested to sleep is too
	 * short then give up and remain awake
	 */
	if(((tl>=rb)&& (tl-rb) <= MSECS(MIN_SLEEP_TIME))
		||((rb>tl)&& (rb-tl) < MSECS(MIN_SLEEP_TIME))) {
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		printk("too short to sleep::%x, %x, %lx\n",tl, rb,  MSECS(MIN_SLEEP_TIME));
		return;
	}	
		
#if 1
	if(((tl > rb) && ((tl-rb) > MSECS(MAX_SLEEP_TIME)))||
		((tl < rb) && (tl>MSECS(69)) && ((rb-tl) > MSECS(MAX_SLEEP_TIME)))||
		((tl<rb)&&(tl<MSECS(69))&&((tl+0xffffffff-rb)>MSECS(MAX_SLEEP_TIME)))) {
		printk("========>too long to sleep:%x, %x, %lx\n", tl, rb,  MSECS(MAX_SLEEP_TIME));
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		return;
	}
#endif	
//	write_nic_dword(dev, TimerInt, tl);
//	rb = read_nic_dword(dev, TSFTR);
	{
		u32 tmp = (tl>rb)?(tl-rb):(rb-tl);
	//	if (tl<rb)
		//printk("==============>%s(): wake up time is %d,%d\n",__FUNCTION__,tmp,jiffies_to_msecs(tmp));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
		schedule_task(&priv->ieee80211->hw_wakeup_wq);
#else
		queue_delayed_work(priv->ieee80211->wq, &priv->ieee80211->hw_wakeup_wq, tmp); //as tl may be less than rb
#endif
	}
	/* if we suspect the TimerInt is gone beyond tl 
	 * while setting it, then give up
	 */
#if 0
	if(((tl > rb) && ((tl-rb) > MSECS(MAX_SLEEP_TIME)))||
		((tl < rb) && (tl>MSECS(69)) && ((rb-tl) > MSECS(MAX_SLEEP_TIME)))||
		((tl<rb)&&(tl<MSECS(69))&&((tl+0xffffffff-rb)>MSECS(MAX_SLEEP_TIME)))) {
		printk("========>too long to sleep:%x, %x, %lx\n", tl, rb,  MSECS(MAX_SLEEP_TIME));
		spin_unlock_irqrestore(&priv->ps_lock,flags);
		return;
	}
#endif	
//	if(priv->rf_sleep)
//		priv->rf_sleep(dev);
	
	//printk("<=========%s()\n", __FUNCTION__);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	schedule_task(&priv->ieee80211->hw_sleep_wq);
#else
	queue_delayed_work(priv->ieee80211->wq, (void *)&priv->ieee80211->hw_sleep_wq,0);
#endif
	spin_unlock_irqrestore(&priv->ps_lock,flags);	
}
#endif

bool HalTxCheckStuck8190Pci(struct net_device *dev)
{
	u16 				RegTxCounter = read_nic_word(dev, 0x128);
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool				bStuck = FALSE;
	RT_TRACE(COMP_RESET,"%s():RegTxCounter is %d,TxCounter is %d\n",__FUNCTION__,RegTxCounter,priv->TxCounter);
#ifdef RTL8192SE
	return bStuck;
#endif
	if(priv->TxCounter==RegTxCounter)
		bStuck = TRUE;

	priv->TxCounter = RegTxCounter;

	return bStuck;
}

/*
*	<Assumption: RT_TX_SPINLOCK is acquired.>
*	First added: 2006.11.19 by emily
*/
RESET_TYPE
TxCheckStuck(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8			QueueID;
	//ptx_ring		head=NULL,tail=NULL,txring = NULL;
	u8			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
	bool			bCheckFwTxCnt = false;
	struct rtl8192_tx_ring  *ring = NULL;
	//unsigned long flags;
	
	//
	// Decide Stuch threshold according to current power save mode
	//
	//printk("++++++++++++>%s()\n",__FUNCTION__);
	switch (priv->ieee80211->dot11PowerSaveMode)
	{
		// The threshold value  may required to be adjusted .
		case eActive:		// Active/Continuous access.
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_NORMAL;
			break;
		case eMaxPs:		// Max power save mode.
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
			break;
		case eFastPs:	// Fast power save mode.
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
			break;
		default:
			break;
	}
			
	//
	// Check whether specific tcb has been queued for a specific time
	//
	for(QueueID = 0; QueueID < MAX_TX_QUEUE; QueueID++)
	{

	
		if(QueueID == TXCMD_QUEUE)
			continue;

		ring = &priv->tx_ring[QueueID];

		if(skb_queue_len(&ring->queue) == 0)
			continue;
		else
		{
			//txring->nStuckCount++;
			#if 0	
			if(txring->nStuckCount > ResetThreshold)
			{
				RT_TRACE( COMP_RESET, "<== TxCheckStuck()\n" );
				return RESET_TYPE_NORMAL;
			}
			#endif
			bCheckFwTxCnt = true;
		}
	}
#if 1 
	if(bCheckFwTxCnt)
	{
		if(HalTxCheckStuck8190Pci(dev))
		{
			RT_TRACE(COMP_RESET, "TxCheckStuck(): Fw indicates no Tx condition! \n");
			return RESET_TYPE_SILENT;
		}
	}
#endif
	return RESET_TYPE_NORESET;
}


bool HalRxCheckStuck8190Pci(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u16 				RegRxCounter = read_nic_word(dev, 0x130);
	bool				bStuck = FALSE;
	static u8			rx_chk_cnt = 0;
#ifdef RTL8192SE
	return bStuck;
#endif
	RT_TRACE(COMP_RESET,"%s(): RegRxCounter is %d,RxCounter is %d\n",__FUNCTION__,RegRxCounter,priv->RxCounter);
	// If rssi is small, we should check rx for long time because of bad rx.
	// or maybe it will continuous silent reset every 2 seconds.
	rx_chk_cnt++;
	if(priv->undecorated_smoothed_pwdb >= (RateAdaptiveTH_High+5))
	{
		rx_chk_cnt = 0;	//high rssi, check rx stuck right now.
	}
	else if(priv->undecorated_smoothed_pwdb < (RateAdaptiveTH_High+5) &&
		((priv->CurrentChannelBW!=HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb>=RateAdaptiveTH_Low_40M) ||
		(priv->CurrentChannelBW==HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb>=RateAdaptiveTH_Low_20M)) )

	{
		if(rx_chk_cnt < 2)
		{
			return bStuck;
		}
		else
		{
			rx_chk_cnt = 0;
		}
	}
	else if(((priv->CurrentChannelBW!=HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb<RateAdaptiveTH_Low_40M) ||
		(priv->CurrentChannelBW==HT_CHANNEL_WIDTH_20&&priv->undecorated_smoothed_pwdb<RateAdaptiveTH_Low_20M)) &&
		priv->undecorated_smoothed_pwdb >= VeryLowRSSI)
	{
		if(rx_chk_cnt < 4)
		{
			//DbgPrint("RSSI < %d && RSSI >= %d, no check this time \n", RateAdaptiveTH_Low, VeryLowRSSI);
			return bStuck;
		}
		else
		{
			rx_chk_cnt = 0;
			//DbgPrint("RSSI < %d && RSSI >= %d, check this time \n", RateAdaptiveTH_Low, VeryLowRSSI);
		}
	}
	else
	{
		if(rx_chk_cnt < 8)
		{
			//DbgPrint("RSSI <= %d, no check this time \n", VeryLowRSSI);
			return bStuck;
		}
		else
		{
			rx_chk_cnt = 0;
			//DbgPrint("RSSI <= %d, check this time \n", VeryLowRSSI);
		}
	}

	if(priv->RxCounter==RegRxCounter)
		bStuck = true;

	priv->RxCounter = RegRxCounter;

	return bStuck;
}

RESET_TYPE RxCheckStuck(struct net_device *dev)
{
	
	if(HalRxCheckStuck8190Pci(dev))
	{
		RT_TRACE(COMP_RESET, "RxStuck Condition\n");
		return RESET_TYPE_SILENT;
	}
	
	return RESET_TYPE_NORESET;
}

RESET_TYPE
rtl819x_ifcheck_resetornot(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	RESET_TYPE	TxResetType = RESET_TYPE_NORESET;
	RESET_TYPE	RxResetType = RESET_TYPE_NORESET;
	RT_RF_POWER_STATE 	rfState;
	
	rfState = priv->ieee80211->eRFPowerState;
	
	TxResetType = TxCheckStuck(dev);
#if 1 
	if( rfState != eRfOff && 
		/*ADAPTER_TEST_STATUS_FLAG(dev, ADAPTER_STATUS_FW_DOWNLOAD_FAILURE)) &&*/
		(priv->ieee80211->iw_mode != IW_MODE_ADHOC))
	{
		// If driver is in the status of firmware download failure , driver skips RF initialization and RF is 
		// in turned off state. Driver should check whether Rx stuck and do silent reset. And
		// if driver is in firmware download failure status, driver should initialize RF in the following 
		// silent reset procedure Emily, 2008.01.21

		// Driver should not check RX stuck in IBSS mode because it is required to
		// set Check BSSID in order to send beacon, however, if check BSSID is 
		// set, STA cannot hear any packet a all. Emily, 2008.04.12
		RxResetType = RxCheckStuck(dev);
	}
#endif
	RT_TRACE(COMP_RESET,"%s(): TxResetType is %d, RxResetType is %d\n",__FUNCTION__,TxResetType,RxResetType);
	if(TxResetType==RESET_TYPE_NORMAL || RxResetType==RESET_TYPE_NORMAL)
		return RESET_TYPE_NORMAL;
	else if(TxResetType==RESET_TYPE_SILENT || RxResetType==RESET_TYPE_SILENT)
		return RESET_TYPE_SILENT;
	else
		return RESET_TYPE_NORESET;

}

/*
 * This function is used to fix Tx/Rx stop bug temporarily.
 * This function will do "system reset" to NIC when Tx or Rx is stuck.
 * The method checking Tx/Rx stuck of this function is supported by FW,
 * which reports Tx and Rx counter to register 0x128 and 0x130.  
 * */
void rtl819x_ifsilentreset(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8	reset_times = 0;
	int reset_status = 0;
	struct ieee80211_device *ieee = priv->ieee80211;
	

	// 2007.07.20. If we need to check CCK stop, please uncomment this line.
	//bStuck = dev->HalFunc.CheckHWStopHandler(dev);
		
	if(priv->ResetProgress==RESET_TYPE_NORESET)
	{
RESET_START:		

		RT_TRACE(COMP_RESET,"=========>Reset progress!! \n");
		
		// Set the variable for reset.
		priv->ResetProgress = RESET_TYPE_SILENT;
//		rtl8192_close(dev);
#if 1	
		down(&priv->wx_sem);	
		if(priv->up == 0)
		{
			RT_TRACE(COMP_ERR,"%s():the driver is not up! return\n",__FUNCTION__);
			up(&priv->wx_sem);
			return ;
		}
		priv->up = 0;
		RT_TRACE(COMP_RESET,"%s():======>start to down the driver\n",__FUNCTION__);
		if(!netif_queue_stopped(dev))
			netif_stop_queue(dev);
#ifndef RTL8192SE
		dm_backup_dynamic_mechanism_state(dev);
#endif
		rtl8192_irq_disable(dev);
		rtl8192_cancel_deferred_work(priv);
		deinit_hal_dm(dev);
		del_timer_sync(&priv->watch_dog_timer);	
		ieee->sync_scan_hurryup = 1;
		if(ieee->state == IEEE80211_LINKED)
		{
			down(&ieee->wx_sem);
			printk("ieee->state is IEEE80211_LINKED\n");
			ieee80211_stop_send_beacons(priv->ieee80211);
			del_timer_sync(&ieee->associate_timer);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)	
                        cancel_delayed_work(&ieee->associate_retry_wq);	
#endif	
			ieee80211_stop_scan(ieee);
			netif_carrier_off(dev);
			up(&ieee->wx_sem);
		}
		else{
			printk("ieee->state is NOT LINKED\n");
			ieee80211_softmac_stop_protocol(priv->ieee80211,true);		
		}
		priv->ops->stop_adapter(dev, true);
		up(&priv->wx_sem);
		RT_TRACE(COMP_RESET,"%s():<==========down process is finished\n",__FUNCTION__);	
		RT_TRACE(COMP_RESET,"%s():===========>start to up the driver\n",__FUNCTION__);
		reset_status = _rtl8192_up(dev);
		
		RT_TRACE(COMP_RESET,"%s():<===========up process is finished\n",__FUNCTION__);
		if(reset_status == -1)
		{
			if(reset_times < 3)
			{
				reset_times++;
				goto RESET_START;
			}
			else
			{
				RT_TRACE(COMP_ERR," ERR!!! %s():  Reset Failed!!\n",__FUNCTION__);
			}
		}
#endif
		ieee->is_silent_reset = 1;
#if 1 
		EnableHWSecurityConfig8192(dev);
#if 1 
		if(ieee->state == IEEE80211_LINKED && ieee->iw_mode == IW_MODE_INFRA)
		{
			ieee->set_chan(ieee->dev, ieee->current_network.channel);
		
#if 1 
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)		
			queue_work(ieee->wq, &ieee->associate_complete_wq);
#else
			schedule_task(&ieee->associate_complete_wq);
#endif
#endif
	
		}
		else if(ieee->state == IEEE80211_LINKED && ieee->iw_mode == IW_MODE_ADHOC)
		{
			ieee->set_chan(ieee->dev, ieee->current_network.channel);
			ieee->link_change(ieee->dev);
	
		//	notify_wx_assoc_event(ieee);
	
			ieee80211_start_send_beacons(ieee);
	
			if (ieee->data_hard_resume)
				ieee->data_hard_resume(ieee->dev);
			netif_carrier_on(ieee->dev);
		}
#endif
		
		CamRestoreAllEntry(dev);

		// Restore the previous setting for all dynamic mechanism
#ifndef RTL8192SE
		dm_restore_dynamic_mechanism_state(dev);
#endif
		priv->ResetProgress = RESET_TYPE_NORESET;
		priv->reset_count++;

		priv->bForcedSilentReset =false;
		priv->bResetInProgress = false;

		// For test --> force write UFWP.
		write_nic_byte(dev, UFWP, 1);	
		RT_TRACE(COMP_RESET, "Reset finished!! ====>[%d]\n", priv->reset_count);
#endif
	}
}

#ifdef ENABLE_IPS
void InactivePsWorkItemCallback(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));
	//u8							index = 0;

	RT_TRACE(COMP_PS, "InactivePsWorkItemCallback() ---------> \n");			
	//
	// This flag "bSwRfProcessing", indicates the status of IPS procedure, should be set if the IPS workitem
	// is really scheduled.
	// The old code, sets this flag before scheduling the IPS workitem and however, at the same time the
	// previous IPS workitem did not end yet, fails to schedule the current workitem. Thus, bSwRfProcessing
	// blocks the IPS procedure of switching RF.
	// By Bruce, 2007-12-25.
	//
	pPSC->bSwRfProcessing = true;

	RT_TRACE(COMP_PS, "InactivePsWorkItemCallback(): Set RF to %s.\n", \
			pPSC->eInactivePowerState == eRfOff?"OFF":"ON");
	//added by amy 090331
#ifdef RTL8192SE_CONFIG_ASPM_OR_D3
	if(pPSC->eInactivePowerState == eRfOn)
	{

		if((pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM))
		{
			RT_DISABLE_ASPM(dev);
			RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM);
		}
		else if((pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3))
		{
			RT_LEAVE_D3(dev, false);
			RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3);
		}
	}
#endif
	//added by amy 090331 end

	MgntActSet_RF_State(dev, pPSC->eInactivePowerState, RF_CHANGE_BY_IPS);

	//added by amy 090331
#ifdef RTL8192SE_CONFIG_ASPM_OR_D3
	if(pPSC->eInactivePowerState == eRfOff)
	{
		if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM)
		{
			RT_ENABLE_ASPM(dev);
			RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM);
		}
		else if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3)
		{
			RT_ENTER_D3(dev, false);
			RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3);
		}
	}
#endif
	//added by amy 090331 end
	
	//
	// To solve CAM values miss in RF OFF, rewrite CAM values after RF ON. By Bruce, 2007-09-20.
	//
#if 0
	if(pPSC->eInactivePowerState == eRfOn)
	{
		//
		// To solve CAM values miss in RF OFF, rewrite CAM values after RF ON. By Bruce, 2007-09-20.
		//
		while( index < 4 )
		{
			if( ( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == WEP104_Encryption ) ||
				(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == WEP40_Encryption) )
			{
				if( pMgntInfo->SecurityInfo.KeyLen[index] != 0)
				pAdapter->HalFunc.SetKeyHandler(pAdapter, index, 0, FALSE, pMgntInfo->SecurityInfo.PairwiseEncAlgorithm, TRUE, FALSE);

			}
			index++;
		}
	}
#endif
	pPSC->bSwRfProcessing = false;	
	RT_TRACE(COMP_POWER, "InactivePsWorkItemCallback() <--------- \n");			
}

//
//	Description:
//		Enter the inactive power save mode. RF will be off
//	2007.08.17, by shien chang.
//
void
IPSEnter(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_POWER_SAVE_CONTROL		pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));
	RT_RF_POWER_STATE 			rtState;

	if (pPSC->bInactivePs)
	{
		rtState = priv->ieee80211->eRFPowerState;
		//
		// Added by Bruce, 2007-12-25.
		// Do not enter IPS in the following conditions:
		// (1) RF is already OFF or Sleep
		// (2) bSwRfProcessing (indicates the IPS is still under going)
		// (3) Connectted (only disconnected can trigger IPS)
		// (4) IBSS (send Beacon)
		// (5) AP mode (send Beacon)
		//
		if (rtState == eRfOn && !pPSC->bSwRfProcessing 
			&& (priv->ieee80211->state != IEEE80211_LINKED) && (priv->ieee80211->iw_mode != IW_MODE_MASTER))
		{
			RT_TRACE(COMP_PS,"IPSEnter(): Turn off RF.\n");
			pPSC->eInactivePowerState = eRfOff;
			priv->isRFOff = true;//added by amy 090331
			priv->bInPowerSaveMode = true;//added by amy 090331
//			queue_work(priv->priv_wq,&(pPSC->InactivePsWorkItem));
			InactivePsWorkItemCallback(dev);
		}
	}	
}

//
//	Description:
//		Leave the inactive power save mode, RF will be on.
//	2007.08.17, by shien chang.
//
void
IPSLeave(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));
	RT_RF_POWER_STATE 	rtState;

	if (pPSC->bInactivePs)
	{	
		rtState = priv->ieee80211->eRFPowerState;	
		if (rtState != eRfOn  && !pPSC->bSwRfProcessing && priv->ieee80211->RfOffReason <= RF_CHANGE_BY_IPS)
		{
			RT_TRACE(COMP_PS, "IPSLeave(): Turn on RF.\n");
			pPSC->eInactivePowerState = eRfOn;
			priv->bInPowerSaveMode = false;
//			queue_work(priv->priv_wq,&(pPSC->InactivePsWorkItem));
			InactivePsWorkItemCallback(dev);
		}
	}
}
//added by amy 090331
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void IPSLeave_wq (struct work_struct *work)
{
        struct ieee80211_device *ieee = container_of(work,struct ieee80211_device,ips_leave_wq);
        struct net_device *dev = ieee->dev;
#else
void IPSLeave_wq(struct net_device *dev)
{
#endif
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	down(&priv->ieee80211->ips_sem);
	IPSLeave(dev);	
	up(&priv->ieee80211->ips_sem);	
}
void ieee80211_ips_leave(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	RT_RF_POWER_STATE	rtState;
	rtState = priv->ieee80211->eRFPowerState;

	if(priv->ieee80211->PowerSaveControl.bInactivePs){ 
		if(rtState == eRfOff){
			if(priv->ieee80211->RfOffReason > RF_CHANGE_BY_IPS)
			{
				RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
				return;
			}
			else{
				printk("=========>%s(): IPSLeave\n",__FUNCTION__);
				queue_work(priv->ieee80211->wq,&priv->ieee80211->ips_leave_wq);				
			}
		}
	}
}
//added by amy 090331 end
#endif

#ifdef ENABLE_LPS
// 
// Change current and default preamble mode.
// 2005.01.06, by rcnjko.
//
bool MgntActSet_802_11_PowerSaveMode(struct net_device *dev,	u8 rtPsMode)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));
	//u8 RpwmVal, FwPwrMode;

	// Currently, we do not change power save mode on IBSS mode. 
	if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)	
	{
		return false;
	}

	//
	// <RJ_NOTE> If we make HW to fill up the PwrMgt bit for us, 
	// some AP will not response to our mgnt frames with PwrMgt bit set, 
	// e.g. cannot associate the AP. 
	// So I commented out it. 2005.02.16, by rcnjko.
	//
//	// Change device's power save mode.
//	Adapter->HalFunc.SetPSModeHandler( Adapter, rtPsMode );
	
	// Update power save mode configured.
	RT_TRACE(COMP_LPS,"%s(): set ieee->ps = %x\n",__FUNCTION__,rtPsMode);
	priv->ieee80211->ps = rtPsMode;
#if 0
	priv->ieee80211->dot11PowerSaveMode = rtPsMode;

	// Determine ListenInterval. 
	if(priv->ieee80211->dot11PowerSaveMode == eMaxPs)
	{
	//	priv->ieee80211->ListenInterval = priv->ieee80211->dot11DtimPeriod;
	}
	else
	{
	//	priv->ieee80211->ListenInterval = 2;
	}

	// Awake immediately
	if(priv->ieee80211->sta_sleep != 0 && rtPsMode == eActive)
	{
		//PlatformSetTimer(Adapter, &(pMgntInfo->AwakeTimer), 0);
		// Notify the AP we awke.
		rtl8192_hw_wakeup(dev);
		ieee80211_sta_ps_send_null_frame(priv->ieee80211, 0);
	}


	// <FW control LPS>
	// 1. Enter PS mode 
	//    Set RPWM to Fw to turn RF off and send H2C FwPwrMode cmd to set Fw into PS mode.
	// 2. Leave PS mode
	//    Send H2C FwPwrMode cmd to Fw to set Fw into Active mode and set RPWM to turn RF on.
	// By tynli. 2009.01.19.
	if((pPSC->bFwCtrlLPS) && (pPSC->bLeisurePs))
	{	
		if(priv->ieee80211->dot11PowerSaveMode == eActive)
		{
			RpwmVal = 0x0C; // RF on
			FwPwrMode = FW_PS_ACTIVE_MODE;
			//DbgPrint("****RPWM--->RF On\n");
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_SET_RPWM, (pu1Byte)(&RpwmVal));
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_H2C_FW_PWRMODE, (pu1Byte)(&FwPwrMode));
		}
		else
		{
			if(GetFwLPS_Doze(Adapter))
			{
				RpwmVal = 0x02; // RF off
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_H2C_FW_PWRMODE, (pu1Byte)(&pPSC->FWCtrlPSMode));
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_SET_RPWM, (pu1Byte)(&RpwmVal));
				//DbgPrint("****RPWM--->RF Off\n");
			}
			else
			{
				// Reset the power save related parameters.
				pMgntInfo->dot11PowerSaveMode = eActive;
				Adapter->bInPowerSaveMode = FALSE;	
				//DbgPrint("LeisurePSEnter() was return.\n");
			}
		}
	}
#endif
	return true;
}

//================================================================================
// Leisure Power Save in linked state.
//================================================================================

//
//	Description:
//		Enter the leisure power save mode.
//
#define RT_CHECK_FOR_HANG_PERIOD 2
void LeisurePSEnter(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));

	RT_TRACE((COMP_PS && COMP_LPS), "LeisurePSEnter()...\n");
	RT_TRACE((COMP_PS && COMP_LPS), "pPSC->bLeisurePs = %d, ieee->ps = %d\n", 
		pPSC->bLeisurePs, priv->ieee80211->ps);

	if(!((priv->ieee80211->iw_mode == IW_MODE_INFRA) && (priv->ieee80211->state == IEEE80211_LINKED))
		|| (priv->ieee80211->iw_mode == IW_MODE_ADHOC) || (priv->ieee80211->iw_mode == IW_MODE_MASTER))
		return;

	if (pPSC->bLeisurePs)
	{
		// Idle for a while if we connect to AP a while ago.
		if(pPSC->LpsIdleCount >= RT_CHECK_FOR_HANG_PERIOD) //  4 Sec 
		{
	
			if(priv->ieee80211->ps == IEEE80211_PS_DISABLED)
			{

				RT_TRACE(COMP_LPS, "LeisurePSEnter(): Enter 802.11 power save mode...\n");

				if(!pPSC->bFwCtrlLPS)
				{
					// Firmware workaround fix for PS-Poll issue. 2009/1/4, Emily
					if (priv->ieee80211->SetFwCmdHandler)
					{
						priv->ieee80211->SetFwCmdHandler(dev, FW_CMD_LPS_ENTER);
					}
				}	
				MgntActSet_802_11_PowerSaveMode(dev, IEEE80211_PS_MBCAST|IEEE80211_PS_UNICAST);

				/*if(pPSC->RegRfPsLevel & RT_RF_LPS_LEVEL_ASPM)
				{
					RT_ENABLE_ASPM(pAdapter);
					RT_SET_PS_LEVEL(pAdapter, RT_RF_LPS_LEVEL_ASPM);
				}*///move to lps_dozecomplete()

			}	
		}
		else
			pPSC->LpsIdleCount++;
	}	
}


//
//	Description:
//		Leave the leisure power save mode.
//
void LeisurePSLeave(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));


	RT_TRACE((COMP_PS && COMP_LPS), "LeisurePSLeave()...\n");
	RT_TRACE((COMP_PS && COMP_LPS),"pPSC->bLeisurePs = %d, ieee->ps = %d\n", 
		pPSC->bLeisurePs, priv->ieee80211->ps);

	if (pPSC->bLeisurePs)
	{	
		if(priv->ieee80211->ps != IEEE80211_PS_DISABLED)
		{
			/*if(pPSC->RegRfPsLevel & RT_RF_LPS_LEVEL_ASPM && RT_IN_PS_LEVEL(pAdapter, RT_RF_LPS_LEVEL_ASPM))
			{
				RT_DISABLE_ASPM(pAdapter);
				RT_CLEAR_PS_LEVEL(pAdapter, RT_RF_LPS_LEVEL_ASPM);
			}*/ 
			// move to lps_wakecomplete()
			RT_TRACE(COMP_LPS, "LeisurePSLeave(): Busy Traffic , Leave 802.11 power save..\n");
			MgntActSet_802_11_PowerSaveMode(dev, IEEE80211_PS_DISABLED);

			if(!pPSC->bFwCtrlLPS) // Fw will control this by itself in powersave v2.
			{
				// Firmware workaround fix for PS-Poll issue. 2009/1/4, Emily
				if (priv->ieee80211->SetFwCmdHandler)
				{
					priv->ieee80211->SetFwCmdHandler(dev, FW_CMD_LPS_LEAVE);
				}
                    }
		}
	}
}
#endif
void rtl819x_update_rxcounts(
	struct r8192_priv *priv,
	u32* TotalRxBcnNum,
	u32* TotalRxDataNum
)	
{
	u16 			SlotIndex;
	u8			i;

	*TotalRxBcnNum = 0;
	*TotalRxDataNum = 0;
	
	SlotIndex = (priv->ieee80211->LinkDetectInfo.SlotIndex++)%(priv->ieee80211->LinkDetectInfo.SlotNum);
	priv->ieee80211->LinkDetectInfo.RxBcnNum[SlotIndex] = priv->ieee80211->LinkDetectInfo.NumRecvBcnInPeriod;
	priv->ieee80211->LinkDetectInfo.RxDataNum[SlotIndex] = priv->ieee80211->LinkDetectInfo.NumRecvDataInPeriod;
	for( i=0; i<priv->ieee80211->LinkDetectInfo.SlotNum; i++ ){	
		*TotalRxBcnNum += priv->ieee80211->LinkDetectInfo.RxBcnNum[i];
		*TotalRxDataNum += priv->ieee80211->LinkDetectInfo.RxDataNum[i];
	}
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
extern	void	rtl819x_watchdog_wqcallback(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
       struct r8192_priv *priv = container_of(dwork,struct r8192_priv,watch_dog_wq);
       struct net_device *dev = priv->ieee80211->dev;
#else
extern	void	rtl819x_watchdog_wqcallback(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
#endif
	struct ieee80211_device* ieee = priv->ieee80211;
	RESET_TYPE	ResetType = RESET_TYPE_NORESET;
      	static u8	check_reset_cnt=0;
	unsigned long flags;
	bool bBusyTraffic = false;
	bool bEnterPS = false;
	if(!priv->up)
		return;
	hal_dm_watchdog(dev);
#ifdef ENABLE_IPS
//	printk("watch_dog ENABLE_IPS\n");
	if(ieee->actscanning == false){
		if((ieee->iw_mode == IW_MODE_INFRA) && (ieee->state == IEEE80211_NOLINK)
			/*&& (ieee->beinretry == false)*/ && (ieee->eRFPowerState == eRfOn) 
			&& !ieee->is_set_key && (!ieee->proto_stoppping)){
			if(ieee->PowerSaveControl.ReturnPoint == IPS_CALLBACK_NONE){
				printk("====================>haha:IPSEnter()\n");
				IPSEnter(dev);	
				//ieee80211_stop_scan(priv->ieee80211);
			}
		}
	}
#endif
	{//to get busy traffic condition
		if(ieee->state == IEEE80211_LINKED)
		{
			if(	ieee->LinkDetectInfo.NumRxOkInPeriod> 100 ||
				ieee->LinkDetectInfo.NumTxOkInPeriod> 100 ) {
				bBusyTraffic = true;
			}
			//added by amy for Leisure PS
			if(	((ieee->LinkDetectInfo.NumRxUnicastOkInPeriod + ieee->LinkDetectInfo.NumTxOkInPeriod) > 8 ) ||
				(ieee->LinkDetectInfo.NumRxUnicastOkInPeriod > 3) )
			{
				//printk("ieee->LinkDetectInfo.NumRxUnicastOkInPeriod is %d,ieee->LinkDetectInfo.NumTxOkInPeriod is %d\n",ieee->LinkDetectInfo.NumRxUnicastOkInPeriod,ieee->LinkDetectInfo.NumTxOkInPeriod);
				bEnterPS= false;
			}
			else
			{
				bEnterPS= true;
			}

			RT_TRACE(COMP_LPS,"***bEnterPS = %d\n", bEnterPS);
#ifdef ENABLE_LPS
			// LeisurePS only work in infra mode.
			if(bEnterPS)
			{
				LeisurePSEnter(dev);
			}
			else
			{
				LeisurePSLeave(dev);
			}
		
		}
		else
		{
			RT_TRACE(COMP_LPS,"====>no link LPS leave\n");
			LeisurePSLeave(dev);
		}
#else
		}
#endif
	       ieee->LinkDetectInfo.NumRxOkInPeriod = 0;
	       ieee->LinkDetectInfo.NumTxOkInPeriod = 0;
	       ieee->LinkDetectInfo.NumRxUnicastOkInPeriod = 0;
	       ieee->LinkDetectInfo.bBusyTraffic = bBusyTraffic;
	}
	//added by amy for AP roaming
	{
		if(ieee->state == IEEE80211_LINKED && ieee->iw_mode == IW_MODE_INFRA)
		{
			u32	TotalRxBcnNum = 0;
			u32	TotalRxDataNum = 0;	

			rtl819x_update_rxcounts(priv, &TotalRxBcnNum, &TotalRxDataNum);
			if((TotalRxBcnNum+TotalRxDataNum) == 0)
			{
				if( ieee->eRFPowerState == eRfOff)
					RT_TRACE(COMP_ERR,"========>%s()\n",__FUNCTION__);
				printk("===>%s(): AP is power off,connect another one\n",__FUNCTION__);
		//		Dot11d_Reset(dev);
				ieee->state = IEEE80211_ASSOCIATING;
				notify_wx_assoc_event(priv->ieee80211);
                                RemovePeerTS(priv->ieee80211,priv->ieee80211->current_network.bssid);
				ieee->is_roaming = true;
				ieee->is_set_key = false;
                             ieee->link_change(dev);
			     if(ieee->LedControlHandler)
				ieee->LedControlHandler(ieee->dev, LED_CTL_START_TO_LINK); //added by amy for LED 090318
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
                                queue_delayed_work(ieee->wq, &ieee->associate_procedure_wq, 0);
#else
                                schedule_task(&ieee->associate_procedure_wq);
#endif

			}
		}
	      ieee->LinkDetectInfo.NumRecvBcnInPeriod=0;
              ieee->LinkDetectInfo.NumRecvDataInPeriod=0;
		
	}
//	CAM_read_entry(dev,0);
	//check if reset the driver
	spin_lock_irqsave(&priv->tx_lock,flags);
	if(check_reset_cnt++ >= 3 && !ieee->is_roaming)
	{
    		ResetType = rtl819x_ifcheck_resetornot(dev);
		check_reset_cnt = 3;
		//DbgPrint("Start to check silent reset\n");
	}
	spin_unlock_irqrestore(&priv->tx_lock,flags);
	if(!priv->bDisableNormalResetCheck && ResetType == RESET_TYPE_NORMAL)
	{
		priv->ResetProgress = RESET_TYPE_NORMAL;
		RT_TRACE(COMP_RESET,"%s(): NOMAL RESET\n",__FUNCTION__);
		return;
	}
	/* disable silent reset temply 2008.9.11*/
#if 1
	if( ((priv->force_reset) || (!priv->bDisableNormalResetCheck && ResetType==RESET_TYPE_SILENT))) // This is control by OID set in Pomelo
	{
		rtl819x_ifsilentreset(dev);
	}
#endif
	priv->force_reset = false;
	priv->bForcedSilentReset = false;
	priv->bResetInProgress = false;
	RT_TRACE(COMP_TRACE, " <==RtUsbCheckForHangWorkItemCallback()\n");
	
}

void watch_dog_timer_callback(unsigned long data)
{
	struct r8192_priv *priv = ieee80211_priv((struct net_device *) data);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	queue_delayed_work(priv->priv_wq,&priv->watch_dog_wq,0);
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	schedule_task(&priv->watch_dog_wq);
#else
	queue_work(priv->priv_wq,&priv->watch_dog_wq);
#endif
#endif
	mod_timer(&priv->watch_dog_timer, jiffies + MSECS(IEEE80211_WATCH_DOG_TIME));

}

/****************************************************************************
 ---------------------------- NIC TX/RX STUFF---------------------------
*****************************************************************************/
void rtl8192_rx_enable(struct net_device *dev)
{
    struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
    write_nic_dword(dev, RDQDA,priv->rx_ring_dma);
}

/* the TX_DESC_BASE setting is according to the following queue index 
 *  BK_QUEUE       ===>                        0
 *  BE_QUEUE       ===>                        1
 *  VI_QUEUE       ===>                        2
 *  VO_QUEUE       ===>                        3
 *  HCCA_QUEUE     ===>                        4
 *  TXCMD_QUEUE    ===>                        5
 *  MGNT_QUEUE     ===>                        6
 *  HIGH_QUEUE     ===>                        7
 *  BEACON_QUEUE   ===>                        8 
 *  */
 #ifdef RTL8192SE
u32 TX_DESC_BASE[] = {TBKDA, TBEDA, TVIDA, TVODA, HDA, TCDA, TMDA, THPDA, TBDA};
 #else
u32 TX_DESC_BASE[] = {BKQDA, BEQDA, VIQDA, VOQDA, HCCAQDA, CQDA, MQDA, HQDA, BQDA}; 
 #endif
void rtl8192_tx_enable(struct net_device *dev)
{
    struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
    u32 i;
    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++)
        write_nic_dword(dev, TX_DESC_BASE[i], priv->tx_ring[i].dma);

    ieee80211_reset_queue(priv->ieee80211);
}

#if 0
void rtl8192_beacon_tx_enable(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u32 reg;

	reg = read_nic_dword(priv->ieee80211->dev,INTA_MASK);

	/* enable Beacon realted interrupt signal */
	reg |= (IMR_BcnInt | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);
	write_nic_byte(dev,reg);	
}
#endif

static void rtl8192_free_rx_ring(struct net_device *dev)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
    int i;

    for (i = 0; i < priv->rxringcount; i++) {
        struct sk_buff *skb = priv->rx_buf[i];
        if (!skb)
            continue;

        pci_unmap_single(priv->pdev,
                *((dma_addr_t *)skb->cb),
                priv->rxbuffersize, PCI_DMA_FROMDEVICE);
        kfree_skb(skb);
    }

    pci_free_consistent(priv->pdev, sizeof(*priv->rx_ring) * priv->rxringcount,
            priv->rx_ring, priv->rx_ring_dma);
    priv->rx_ring = NULL;
}

static void rtl8192_free_tx_ring(struct net_device *dev, unsigned int prio)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
    struct rtl8192_tx_ring *ring = &priv->tx_ring[prio];

    while (skb_queue_len(&ring->queue)) {
        tx_desc *entry = &ring->desc[ring->idx];
        struct sk_buff *skb = __skb_dequeue(&ring->queue);

        pci_unmap_single(priv->pdev, le32_to_cpu(entry->TxBuffAddr),
                skb->len, PCI_DMA_TODEVICE);
        kfree_skb(skb);
        ring->idx = (ring->idx + 1) % ring->entries;
    }

    pci_free_consistent(priv->pdev, sizeof(*ring->desc)*ring->entries,
            ring->desc, ring->dma);
    ring->desc = NULL;
}


void rtl8192_beacon_disable(struct net_device *dev) 
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u32 reg;

	reg = read_nic_dword(priv->ieee80211->dev,INTA_MASK);

	/* disable Beacon realted interrupt signal */
	reg &= ~(IMR_BcnInt | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);
	write_nic_dword(priv->ieee80211->dev, INTA_MASK, reg);	
}


void rtl8192_data_hard_stop(struct net_device *dev)
{
	//FIXME !!
	#if 0
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask |= (1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
	#endif
}


void rtl8192_data_hard_resume(struct net_device *dev)
{
	// FIXME !!
	#if 0
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	priv->dma_poll_mask &= ~(1<<TX_DMA_STOP_LOWPRIORITY_SHIFT);
	rtl8192_set_mode(dev,EPROM_CMD_CONFIG);
	write_nic_byte(dev,TX_DMA_POLLING,priv->dma_poll_mask);
	rtl8192_set_mode(dev,EPROM_CMD_NORMAL);
	#endif
}

/* this function TX data frames when the ieee80211 stack requires this.
 * It checks also if we need to stop the ieee tx queue, eventually do it
 */
void rtl8192_hard_data_xmit(struct sk_buff *skb, struct net_device *dev, int rate)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	int ret;
	//unsigned long flags;
	cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
	u8 queue_index = tcb_desc->queue_index;
	
	if((priv->bHwRadioOff == true)||(!priv->up)){
		kfree_skb(skb);
		return;	
	}
	
	/* shall not be referred by command packet */
	assert(queue_index != TXCMD_QUEUE);

	//spin_lock_irqsave(&priv->tx_lock,flags);

        memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
#if 0
	tcb_desc->RATRIndex = 7;
	tcb_desc->bTxDisableRateFallBack = 1;
	tcb_desc->bTxUseDriverAssingedRate = 1;
	tcb_desc->bTxEnableFwCalcDur = 1;
#endif
	skb_push(skb, priv->ieee80211->tx_headroom);
	ret = rtl8192_tx(dev, skb);
	if(ret != 0) {
		kfree_skb(skb);
	};

//	
	if(queue_index!=MGNT_QUEUE) {
	priv->ieee80211->stats.tx_bytes+=(skb->len - priv->ieee80211->tx_headroom);
	priv->ieee80211->stats.tx_packets++;
	}

	//spin_unlock_irqrestore(&priv->tx_lock,flags);

//	return ret;
	return;
}

/* This is a rough attempt to TX a frame
 * This is called by the ieee 80211 stack to TX management frames.
 * If the ring is full packet are dropped (for data frame the queue
 * is stopped before this can happen).
 */
int rtl8192_hard_start_xmit(struct sk_buff *skb,struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	
	int ret;
	//unsigned long flags;
        cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
        u8 queue_index = tcb_desc->queue_index;

        if(queue_index != TXCMD_QUEUE){
            if((priv->bHwRadioOff == true)||(!priv->up)){
                kfree_skb(skb);
                return 0;	
            }
        }

	//spin_lock_irqsave(&priv->tx_lock,flags);
	
        memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
	if(queue_index == TXCMD_QUEUE) {
	//	skb_push(skb, USB_HWDESC_HEADER_LEN);
		rtl819xE_tx_cmd(dev, skb);
		ret = 0;
	        //spin_unlock_irqrestore(&priv->tx_lock,flags);	
		return ret;
	} else {
	//	RT_TRACE(COMP_SEND, "To send management packet\n");
		tcb_desc->RATRIndex = 7;
		tcb_desc->bTxDisableRateFallBack = 1;
		tcb_desc->bTxUseDriverAssingedRate = 1;
		tcb_desc->bTxEnableFwCalcDur = 1;
		skb_push(skb, priv->ieee80211->tx_headroom);
		ret = rtl8192_tx(dev, skb);
		if(ret != 0) {
			kfree_skb(skb);
		};
	}
	
//	priv->ieee80211->stats.tx_bytes+=skb->len;
//	priv->ieee80211->stats.tx_packets++;
	
	//spin_unlock_irqrestore(&priv->tx_lock,flags);	
	
	return ret;

}


//void rtl8192_try_wake_queue(struct net_device *dev, int pri);

void rtl8192_tx_isr(struct net_device *dev, int prio)
{
    struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

    struct rtl8192_tx_ring *ring = &priv->tx_ring[prio];

    while (skb_queue_len(&ring->queue)) {
        tx_desc *entry = &ring->desc[ring->idx];
        struct sk_buff *skb;

        /* beacon packet will only use the first descriptor defautly,
         * and the OWN may not be cleared by the hardware 
         * */
        if(prio != BEACON_QUEUE) {
            if(entry->OWN)
                return;
            ring->idx = (ring->idx + 1) % ring->entries;
        }

        skb = __skb_dequeue(&ring->queue);
        pci_unmap_single(priv->pdev, le32_to_cpu(entry->TxBuffAddr),
                skb->len, PCI_DMA_TODEVICE);

        kfree_skb(skb);
    }
    if (prio == MGNT_QUEUE){
//	    printk("====>%s(): skb_queue_len(&ring->queue) is %d,ring->idx is %d\n",__FUNCTION__,skb_queue_len(&ring->queue),ring->idx);
        if (priv->ieee80211->ack_tx_to_ieee){
            if (rtl8192_is_tx_queue_empty(dev)){
                priv->ieee80211->ack_tx_to_ieee = 0;
                ieee80211_ps_tx_ack(priv->ieee80211, 1);
            }
        }
    } 

    if(prio != BEACON_QUEUE) {
        /* try to deal with the pending packets  */
        tasklet_schedule(&priv->irq_tx_tasklet);
    }

}


/*
 * Mapping Software/Hardware descriptor queue id to "Queue Select Field"
 * in TxFwInfo data structure
 * 2006.10.30 by Emily
 *
 * \param QUEUEID       Software Queue       
*/
u8 MapHwQueueToFirmwareQueue(u8 QueueID)
{
	u8 QueueSelect = 0x0;       //defualt set to 

	switch(QueueID) {
#if 0
		case BE_QUEUE:
			QueueSelect = QSLT_BE;  //or QSelect = pTcb->priority;
			break;

		case BK_QUEUE:
			QueueSelect = QSLT_BK;  //or QSelect = pTcb->priority;
			break;

		case VO_QUEUE:
			QueueSelect = QSLT_VO;  //or QSelect = pTcb->priority;
			break;

		case VI_QUEUE:
			QueueSelect = QSLT_VI;  //or QSelect = pTcb->priority;  
			break;
#else
		case BE_QUEUE:
			QueueSelect = 3;
			break;
		case BK_QUEUE:
			QueueSelect = 1;
			break;
		case VO_QUEUE:
			QueueSelect = 7;
			break;
		case VI_QUEUE:
			QueueSelect = 5;
			break;
#endif
		case MGNT_QUEUE:
			QueueSelect = QSLT_BE;
			break;

		case BEACON_QUEUE:
			QueueSelect = QSLT_BEACON;
			break;

			// TODO: 2006.10.30 mark other queue selection until we verify it is OK
			// TODO: Remove Assertions 
//#if (RTL819X_FPGA_VER & RTL819X_FPGA_GUANGAN_070502)
		case TXCMD_QUEUE:
			QueueSelect = QSLT_CMD;
			break;
//#endif
		case HIGH_QUEUE:
			QueueSelect = QSLT_BE;
			//break;

		default:
			RT_TRACE(COMP_ERR, "TransmitTCB(): Impossible Queue Selection: %d \n", QueueID);
			break;
	}
	return QueueSelect;
}




/* 
 * The tx procedure is just as following, 
 * skb->cb will contain all the following information,
 * priority, morefrag, rate, &dev.
 * */

void rtl819xE_tx_cmd(struct net_device *dev, struct sk_buff *skb)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
    struct rtl8192_tx_ring *ring;
    tx_desc_cmd* entry;
    unsigned int idx;
    dma_addr_t mapping;
    cb_desc *tcb_desc;
    unsigned long flags;

    ring = &priv->tx_ring[TXCMD_QUEUE];
    mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);

    spin_lock_irqsave(&priv->irq_th_lock,flags);
    idx = (ring->idx + skb_queue_len(&ring->queue)) % ring->entries;
    entry = (tx_desc_cmd*) &ring->desc[idx];

    tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
    
    priv->ops->tx_fill_cmd_descriptor(dev, entry, tcb_desc, skb);
#if 0	
    memset(entry,0,12);
    entry->LINIP = tcb_desc->bLastIniPkt;
    entry->FirstSeg = 1;//first segment
    entry->LastSeg = 1; //last segment
    if(tcb_desc->bCmdOrInit == DESC_PACKET_TYPE_INIT) {
        entry->CmdInit = DESC_PACKET_TYPE_INIT;
    } else {
        entry->CmdInit = DESC_PACKET_TYPE_NORMAL;
        entry->Offset = sizeof(TX_FWINFO_8190PCI) + 8;
        entry->PktSize = (u16)(tcb_desc->pkt_size + entry->Offset);
        entry->QueueSelect = QSLT_CMD;
        entry->TxFWInfoSize = 0x08;
        entry->RATid = (u8)DESC_PACKET_TYPE_INIT;
    }
    entry->TxBufferSize = skb->len;
    entry->TxBuffAddr = cpu_to_le32(mapping);
    entry->OWN = 1;

#ifdef JOHN_DUMP_TXDESC
    {       int i;
        tx_desc *entry1 =  &ring->desc[0];
        unsigned int *ptr= (unsigned int *)entry1;
        printk("<Tx descriptor>:\n");
        for (i = 0; i < 8; i++)
            printk("%8x ", ptr[i]);
        printk("\n");
    }
#endif
#endif
    __skb_queue_tail(&ring->queue, skb);
    spin_unlock_irqrestore(&priv->irq_th_lock,flags);

    //write_nic_byte(dev, TPPoll, TPPoll_CQ);
    return;
}

short rtl8192_tx(struct net_device *dev, struct sk_buff* skb)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
    struct rtl8192_tx_ring  *ring;
    unsigned long flags;
    cb_desc *tcb_desc = (cb_desc *)(skb->cb + MAX_DEV_ADDR_SIZE);
    tx_desc *pdesc = NULL;
    struct ieee80211_hdr_1addr * header = NULL;
    u16 fc=0, type=0,stype=0;
    dma_addr_t mapping;
    bool  multi_addr=false,broad_addr=false,uni_addr=false;
    u8*   pda_addr = NULL;
    int   idx;
    u32 fwinfo_size = 0;
    mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
    /* collect the tx packets statitcs */
#ifdef RTL8192SE
    fwinfo_size = 0;
#else
    fwinfo_size = sizeof(TX_FWINFO_8190PCI);
#endif
    header = (struct ieee80211_hdr_1addr *)(((u8*)skb->data) + fwinfo_size);
    fc = header->frame_ctl;
    type = WLAN_FC_GET_TYPE(fc);
    stype = WLAN_FC_GET_STYPE(fc);
    pda_addr = header->addr1;
    if(is_multicast_ether_addr(pda_addr))
        multi_addr = true;
    else if(is_broadcast_ether_addr(pda_addr))
        broad_addr = true;
    else
    {
        uni_addr = true;
    }
    if(uni_addr)
        priv->stats.txbytesunicast += skb->len - fwinfo_size;
    else if(multi_addr)
        priv->stats.txbytesmulticast += skb->len - fwinfo_size;
    else 
        priv->stats.txbytesbroadcast += skb->len - fwinfo_size;

    spin_lock_irqsave(&priv->irq_th_lock,flags);
    ring = &priv->tx_ring[tcb_desc->queue_index];
    if (tcb_desc->queue_index != BEACON_QUEUE) {
        idx = (ring->idx + skb_queue_len(&ring->queue)) % ring->entries;
    } else {
        idx = 0;
    }

    pdesc = &ring->desc[idx];
    if((pdesc->OWN == 1) && (tcb_desc->queue_index != BEACON_QUEUE)) {
	    RT_TRACE(COMP_ERR,"No more TX desc@%d, ring->idx = %d,idx = %d,%x", \
			    tcb_desc->queue_index,ring->idx, idx,skb->len);
	    return skb->len;
    } 
    if(tcb_desc->queue_index == MGNT_QUEUE)
    {
//	    printk("===>rtl8192_tx():skb_queue_len(&ring->queue) is %d,ring->idx is %d\n",skb_queue_len(&ring->queue),idx);
//	    printk("===>stype is %d\n",stype);
    }
    //added by amy for LED 090318
    if(type == IEEE80211_FTYPE_DATA)
	    if(priv->ieee80211->LedControlHandler)
		priv->ieee80211->LedControlHandler(dev, LED_CTL_TX);
    priv->ops->tx_fill_descriptor(dev, pdesc, tcb_desc, skb);
#if 0
    /* fill tx firmware */
    pTxFwInfo = (PTX_FWINFO_8190PCI)skb->data;
    memset(pTxFwInfo,0,sizeof(TX_FWINFO_8190PCI));
    pTxFwInfo->TxHT = (tcb_desc->data_rate&0x80)?1:0;
    pTxFwInfo->TxRate = MRateToHwRate8190Pci((u8)tcb_desc->data_rate);
    pTxFwInfo->EnableCPUDur = tcb_desc->bTxEnableFwCalcDur;
    pTxFwInfo->Short	= QueryIsShort(pTxFwInfo->TxHT, pTxFwInfo->TxRate, tcb_desc);

    /* Aggregation related */
    if(tcb_desc->bAMPDUEnable) {
        pTxFwInfo->AllowAggregation = 1;
        pTxFwInfo->RxMF = tcb_desc->ampdu_factor;
        pTxFwInfo->RxAMD = tcb_desc->ampdu_density;
    } else {
        pTxFwInfo->AllowAggregation = 0;
        pTxFwInfo->RxMF = 0;
        pTxFwInfo->RxAMD = 0;
    }

    //
    // Protection mode related
    //
    pTxFwInfo->RtsEnable =	(tcb_desc->bRTSEnable)?1:0;
    pTxFwInfo->CtsEnable =	(tcb_desc->bCTSEnable)?1:0;
    pTxFwInfo->RtsSTBC =	(tcb_desc->bRTSSTBC)?1:0;
    pTxFwInfo->RtsHT=		(tcb_desc->rts_rate&0x80)?1:0;
    pTxFwInfo->RtsRate =		MRateToHwRate8190Pci((u8)tcb_desc->rts_rate);
    pTxFwInfo->RtsBandwidth = 0;
    pTxFwInfo->RtsSubcarrier = tcb_desc->RTSSC;
    pTxFwInfo->RtsShort =	(pTxFwInfo->RtsHT==0)?(tcb_desc->bRTSUseShortPreamble?1:0):(tcb_desc->bRTSUseShortGI?1:0);
    //
    // Set Bandwidth and sub-channel settings.
    //
    if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
    {
        if(tcb_desc->bPacketBW)
        {
            pTxFwInfo->TxBandwidth = 1;
#ifdef RTL8190P
            pTxFwInfo->TxSubCarrier = 3;
#else
            pTxFwInfo->TxSubCarrier = 0;	//By SD3's Jerry suggestion, use duplicated mode, cosa 04012008
#endif
        }
        else
        {
            pTxFwInfo->TxBandwidth = 0;
            pTxFwInfo->TxSubCarrier = priv->nCur40MhzPrimeSC;
        }
    } else {
        pTxFwInfo->TxBandwidth = 0;
        pTxFwInfo->TxSubCarrier = 0;
    }

    if (0)
    {
   	    TX_FWINFO_T 	Tmp_TxFwInfo;
	    /* 2007/07/25 MH  Copy current TX FW info.*/
	    memcpy((void*)(&Tmp_TxFwInfo), (void*)(pTxFwInfo), sizeof(TX_FWINFO_8190PCI));
	    printk("&&&&&&&&&&&&&&&&&&&&&&====>print out fwinf\n");
	    printk("===>enable fwcacl:%d\n", Tmp_TxFwInfo.EnableCPUDur);
	    printk("===>RTS STBC:%d\n", Tmp_TxFwInfo.RtsSTBC);
	    printk("===>RTS Subcarrier:%d\n", Tmp_TxFwInfo.RtsSubcarrier);
	    printk("===>Allow Aggregation:%d\n", Tmp_TxFwInfo.AllowAggregation);
	    printk("===>TX HT bit:%d\n", Tmp_TxFwInfo.TxHT);
	    printk("===>Tx rate:%d\n", Tmp_TxFwInfo.TxRate);
	    printk("===>Received AMPDU Density:%d\n", Tmp_TxFwInfo.RxAMD);
	    printk("===>Received MPDU Factor:%d\n", Tmp_TxFwInfo.RxMF);
	    printk("===>TxBandwidth:%d\n", Tmp_TxFwInfo.TxBandwidth);
	    printk("===>TxSubCarrier:%d\n", Tmp_TxFwInfo.TxSubCarrier);

        printk("<=====**********************out of print\n");

    }
    spin_lock_irqsave(&priv->irq_th_lock,flags);
    ring = &priv->tx_ring[tcb_desc->queue_index];
    if (tcb_desc->queue_index != BEACON_QUEUE) {
        idx = (ring->idx + skb_queue_len(&ring->queue)) % ring->entries;
    } else {
        idx = 0;
    }

    pdesc = &ring->desc[idx];
    if((pdesc->OWN == 1) && (tcb_desc->queue_index != BEACON_QUEUE)) {
	    RT_TRACE(COMP_ERR,"No more TX desc@%d, ring->idx = %d,idx = %d,%x", \
			    tcb_desc->queue_index,ring->idx, idx,skb->len);
	    return skb->len;
    } 

    /* fill tx descriptor */
    memset((u8*)pdesc,0,12);
    /*DWORD 0*/		
    pdesc->LINIP = 0;
    pdesc->CmdInit = 1;
    pdesc->Offset = sizeof(TX_FWINFO_8190PCI) + 8; //We must add 8!! Emily
    pdesc->PktSize = (u16)skb->len-sizeof(TX_FWINFO_8190PCI);

    /*DWORD 1*/
    pdesc->SecCAMID= 0;
    pdesc->RATid = tcb_desc->RATRIndex;


    pdesc->NoEnc = 1;
    pdesc->SecType = 0x0;
    if (tcb_desc->bHwSec) {
        static u8 tmp =0;
        if (!tmp) {
            printk("==>================hw sec\n");
            tmp = 1;
        }
        switch (priv->ieee80211->pairwise_key_type) {
            case KEY_TYPE_WEP40:
            case KEY_TYPE_WEP104:
                pdesc->SecType = 0x1;
                pdesc->NoEnc = 0;
                break;
            case KEY_TYPE_TKIP:
                pdesc->SecType = 0x2;
                pdesc->NoEnc = 0;
                break;
            case KEY_TYPE_CCMP:
                pdesc->SecType = 0x3;
                pdesc->NoEnc = 0;
                break;
            case KEY_TYPE_NA:
                pdesc->SecType = 0x0;
                pdesc->NoEnc = 1;
                break;
        }
    }

    //
    // Set Packet ID
    //
    pdesc->PktId = 0x0;

    pdesc->QueueSelect = MapHwQueueToFirmwareQueue(tcb_desc->queue_index);
    pdesc->TxFWInfoSize = sizeof(TX_FWINFO_8190PCI);

    pdesc->DISFB = tcb_desc->bTxDisableRateFallBack;
    pdesc->USERATE = tcb_desc->bTxUseDriverAssingedRate;

    pdesc->FirstSeg =1;
    pdesc->LastSeg = 1;
    pdesc->TxBufferSize = skb->len;

    pdesc->TxBuffAddr = cpu_to_le32(mapping);
#endif	
    __skb_queue_tail(&ring->queue, skb);
    pdesc->OWN = 1;
    spin_unlock_irqrestore(&priv->irq_th_lock,flags);
    dev->trans_start = jiffies;
    write_nic_word(dev,TPPoll,0x01<<tcb_desc->queue_index);
    return 0;	
}

short rtl8192_alloc_rx_desc_ring(struct net_device *dev)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
    rx_desc *entry = NULL;
    int i;

    priv->rx_ring = pci_alloc_consistent(priv->pdev,
            sizeof(*priv->rx_ring) * priv->rxringcount, &priv->rx_ring_dma);

    if (!priv->rx_ring || (unsigned long)priv->rx_ring & 0xFF) {
        RT_TRACE(COMP_ERR,"Cannot allocate RX ring\n");
        return -ENOMEM;
    }

    memset(priv->rx_ring, 0, sizeof(*priv->rx_ring) * priv->rxringcount);
    priv->rx_idx = 0;

    for (i = 0; i < priv->rxringcount; i++) {
        struct sk_buff *skb = dev_alloc_skb(priv->rxbuffersize);
        dma_addr_t *mapping;
        entry = &priv->rx_ring[i];
        if (!skb)
            return 0;
        skb->dev = dev;
        priv->rx_buf[i] = skb;
        mapping = (dma_addr_t *)skb->cb;
        *mapping = pci_map_single(priv->pdev, skb->tail,//skb_tail_pointer(skb),
                priv->rxbuffersize, PCI_DMA_FROMDEVICE);

        entry->BufferAddress = cpu_to_le32(*mapping);

        entry->Length = priv->rxbuffersize;
        entry->OWN = 1;
    }

    entry->EOR = 1;
    return 0;
}

static int rtl8192_alloc_tx_desc_ring(struct net_device *dev,
        unsigned int prio, unsigned int entries)
{
    struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
    tx_desc *ring;
    dma_addr_t dma;
    int i;

    ring = pci_alloc_consistent(priv->pdev, sizeof(*ring) * entries, &dma);
    if (!ring || (unsigned long)ring & 0xFF) {
        RT_TRACE(COMP_ERR, "Cannot allocate TX ring (prio = %d)\n", prio);
        return -ENOMEM;
    }

    memset(ring, 0, sizeof(*ring)*entries);
    priv->tx_ring[prio].desc = ring;
    priv->tx_ring[prio].dma = dma;
    priv->tx_ring[prio].idx = 0;
    priv->tx_ring[prio].entries = entries;
    skb_queue_head_init(&priv->tx_ring[prio].queue);

    for (i = 0; i < entries; i++)
        ring[i].NextDescAddress =
            cpu_to_le32((u32)dma + ((i + 1) % entries) * sizeof(*ring));

    return 0;
}

                    
short rtl8192_pci_initdescring(struct net_device *dev)
{
    u32 ret;
    int i;
    struct r8192_priv *priv = ieee80211_priv(dev);

    ret = rtl8192_alloc_rx_desc_ring(dev);
    if (ret) {
        return ret;
    }
    

    /* general process for other queue */
    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++) {
        if ((ret = rtl8192_alloc_tx_desc_ring(dev, i, priv->txringcount)))
            goto err_free_rings;
    }

#if 0
    /* specific process for hardware beacon process */
    if ((ret = rtl8192_alloc_tx_desc_ring(dev, MAX_TX_QUEUE_COUNT - 1, 2)))
        goto err_free_rings;
#endif    

    return 0;

err_free_rings:
    rtl8192_free_rx_ring(dev);
    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++)
        if (priv->tx_ring[i].desc)
            rtl8192_free_tx_ring(dev, i);
    return 1;
}

void rtl8192_pci_resetdescring(struct net_device *dev)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
    int i;

    /* force the rx_idx to the first one */
    if(priv->rx_ring) {
        rx_desc *entry = NULL;
        for (i = 0; i < priv->rxringcount; i++) {
            entry = &priv->rx_ring[i];
            entry->OWN = 1;
        }
        priv->rx_idx = 0;
    }

    /* after reset, release previous pending packet, and force the 
     * tx idx to the first one */
    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++) {
        if (priv->tx_ring[i].desc) {
            struct rtl8192_tx_ring *ring = &priv->tx_ring[i];

            while (skb_queue_len(&ring->queue)) {
                tx_desc *entry = &ring->desc[ring->idx];
                struct sk_buff *skb = __skb_dequeue(&ring->queue);

                pci_unmap_single(priv->pdev, le32_to_cpu(entry->TxBuffAddr),
                        skb->len, PCI_DMA_TODEVICE);
                kfree_skb(skb);
                ring->idx = (ring->idx + 1) % ring->entries;
            }
            ring->idx = 0;
        }
    }
}

u8 HwRateToMRate90(bool bIsHT, u8 rate)
{
	u8  ret_rate = 0x02;

	if(!bIsHT) {
		switch(rate) {
			case DESC90_RATE1M:   ret_rate = MGN_1M;         break;
			case DESC90_RATE2M:   ret_rate = MGN_2M;         break;
			case DESC90_RATE5_5M: ret_rate = MGN_5_5M;       break;
			case DESC90_RATE11M:  ret_rate = MGN_11M;        break;
			case DESC90_RATE6M:   ret_rate = MGN_6M;         break;
			case DESC90_RATE9M:   ret_rate = MGN_9M;         break;
			case DESC90_RATE12M:  ret_rate = MGN_12M;        break;
			case DESC90_RATE18M:  ret_rate = MGN_18M;        break;
			case DESC90_RATE24M:  ret_rate = MGN_24M;        break;
			case DESC90_RATE36M:  ret_rate = MGN_36M;        break;
			case DESC90_RATE48M:  ret_rate = MGN_48M;        break;
			case DESC90_RATE54M:  ret_rate = MGN_54M;        break;

			default:
					      RT_TRACE(COMP_RECV, "HwRateToMRate90(): Non supported Rate [%x], bIsHT = %d!!!\n", rate, bIsHT);
					      break;
		}

	} else {
		switch(rate) {
			case DESC90_RATEMCS0:   ret_rate = MGN_MCS0;    break;
			case DESC90_RATEMCS1:   ret_rate = MGN_MCS1;    break;
			case DESC90_RATEMCS2:   ret_rate = MGN_MCS2;    break;
			case DESC90_RATEMCS3:   ret_rate = MGN_MCS3;    break;
			case DESC90_RATEMCS4:   ret_rate = MGN_MCS4;    break;
			case DESC90_RATEMCS5:   ret_rate = MGN_MCS5;    break;
			case DESC90_RATEMCS6:   ret_rate = MGN_MCS6;    break;
			case DESC90_RATEMCS7:   ret_rate = MGN_MCS7;    break;
			case DESC90_RATEMCS8:   ret_rate = MGN_MCS8;    break;
			case DESC90_RATEMCS9:   ret_rate = MGN_MCS9;    break;
			case DESC90_RATEMCS10:  ret_rate = MGN_MCS10;   break;
			case DESC90_RATEMCS11:  ret_rate = MGN_MCS11;   break;
			case DESC90_RATEMCS12:  ret_rate = MGN_MCS12;   break;
			case DESC90_RATEMCS13:  ret_rate = MGN_MCS13;   break;
			case DESC90_RATEMCS14:  ret_rate = MGN_MCS14;   break;
			case DESC90_RATEMCS15:  ret_rate = MGN_MCS15;   break;
			case DESC90_RATEMCS32:  ret_rate = (0x80|0x20); break;

			default:
						RT_TRACE(COMP_RECV, "HwRateToMRate90(): Non supported Rate [%x], bIsHT = %d!!!\n",rate, bIsHT);
						break;
		}
	}

	return ret_rate;
}

/**
 * Function:     UpdateRxPktTimeStamp
 * Overview:     Recored down the TSF time stamp when receiving a packet
 * 
 * Input:                
 *       PADAPTER        dev
 *       PRT_RFD         pRfd,
 *                       
 * Output:               
 *       PRT_RFD         pRfd
 *                               (pRfd->Status.TimeStampHigh is updated)
 *                               (pRfd->Status.TimeStampLow is updated)
 * Return:       
 *               None
 */
void UpdateRxPktTimeStamp8190 (struct net_device *dev, struct ieee80211_rx_stats *stats)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	if(stats->bIsAMPDU && !stats->bFirstMPDU) {
		stats->mac_time[0] = priv->LastRxDescTSFLow;
		stats->mac_time[1] = priv->LastRxDescTSFHigh;
	} else {
		priv->LastRxDescTSFLow = stats->mac_time[0];
		priv->LastRxDescTSFHigh = stats->mac_time[1];
	}
}

long rtl819x_translate_todbm(u8 signal_strength_index	)// 0-100 index.
{
	long	signal_power; // in dBm.

	// Translate to dBm (x=0.5y-95).
	signal_power = (long)((signal_strength_index + 1) >> 1); 
	signal_power -= 95; 

	return signal_power;
}

//
//	Description:
//		Update Rx signal related information in the packet reeived 
//		to RxStats. User application can query RxStats to realize 
//		current Rx signal status. 
//	
//	Assumption:
//		In normal operation, user only care about the information of the BSS 
//		and we shall invoke this function if the packet received is from the BSS.
//
void
rtl819x_update_rxsignalstatistics8190pci(
	struct r8192_priv * priv,
	struct ieee80211_rx_stats * pprevious_stats
	)
{
	int weighting = 0;

	//2 <ToDo> Update Rx Statistics (such as signal strength and signal quality).

	// Initila state
	if(priv->stats.recv_signal_power == 0)
		priv->stats.recv_signal_power = pprevious_stats->RecvSignalPower;

	// To avoid the past result restricting the statistics sensitivity, weight the current power (5/6) to speed up the 
	// reaction of smoothed Signal Power.
	if(pprevious_stats->RecvSignalPower > priv->stats.recv_signal_power)
		weighting = 5;
	else if(pprevious_stats->RecvSignalPower < priv->stats.recv_signal_power)
		weighting = (-5);
	//
	// We need more correct power of received packets and the  "SignalStrength" of RxStats have been beautified or translated,
	// so we record the correct power in Dbm here. By Bruce, 2008-03-07. 
	//
	priv->stats.recv_signal_power = (priv->stats.recv_signal_power * 5 + pprevious_stats->RecvSignalPower + weighting) / 6;
}

void
rtl8190_process_cck_rxpathsel(
	struct r8192_priv * priv,
	struct ieee80211_rx_stats * pprevious_stats
	)
{
#ifdef RTL8190P	//Only 90P 2T4R need to check
	char				last_cck_adc_pwdb[4]={0,0,0,0};
	u8				i;
//cosa add for Rx path selection
		if(priv->rf_type == RF_2T4R && DM_RxPathSelTable.Enable)
		{
			if(pprevious_stats->bIsCCK && 
				(pprevious_stats->bPacketToSelf ||pprevious_stats->bPacketBeacon))
			{
				/* record the cck adc_pwdb to the sliding window. */
				if(priv->stats.cck_adc_pwdb.TotalNum++ >= PHY_RSSI_SLID_WIN_MAX)
				{
					priv->stats.cck_adc_pwdb.TotalNum = PHY_RSSI_SLID_WIN_MAX;
					for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
					{
						last_cck_adc_pwdb[i] = priv->stats.cck_adc_pwdb.elements[i][priv->stats.cck_adc_pwdb.index];
						priv->stats.cck_adc_pwdb.TotalVal[i] -= last_cck_adc_pwdb[i];
					}
				}
				for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
				{
					priv->stats.cck_adc_pwdb.TotalVal[i] += pprevious_stats->cck_adc_pwdb[i];
					priv->stats.cck_adc_pwdb.elements[i][priv->stats.cck_adc_pwdb.index] = pprevious_stats->cck_adc_pwdb[i];
				}
				priv->stats.cck_adc_pwdb.index++;
				if(priv->stats.cck_adc_pwdb.index >= PHY_RSSI_SLID_WIN_MAX)
					priv->stats.cck_adc_pwdb.index = 0;

				for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
				{
					DM_RxPathSelTable.cck_pwdb_sta[i] = priv->stats.cck_adc_pwdb.TotalVal[i]/priv->stats.cck_adc_pwdb.TotalNum;
				}

				for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
				{
					if(pprevious_stats->cck_adc_pwdb[i]  > (char)priv->undecorated_smoothed_cck_adc_pwdb[i])
					{
						priv->undecorated_smoothed_cck_adc_pwdb[i] = 
							( (priv->undecorated_smoothed_cck_adc_pwdb[i]*(Rx_Smooth_Factor-1)) + 
							(pprevious_stats->cck_adc_pwdb[i])) /(Rx_Smooth_Factor);
						priv->undecorated_smoothed_cck_adc_pwdb[i] = priv->undecorated_smoothed_cck_adc_pwdb[i] + 1;
					}
					else
					{
						priv->undecorated_smoothed_cck_adc_pwdb[i] = 
							( (priv->undecorated_smoothed_cck_adc_pwdb[i]*(Rx_Smooth_Factor-1)) + 
							(pprevious_stats->cck_adc_pwdb[i])) /(Rx_Smooth_Factor);
					}
				}
			}
		}
#endif
}


/* 2008/01/22 MH We can not delcare RSSI/EVM total value of sliding window to
	be a local static. Otherwise, it may increase when we return from S3/S4. The
	value will be kept in memory or disk. We must delcare the value in adapter
	and it will be reinitialized when return from S3/S4. */
void rtl8192_process_phyinfo(struct r8192_priv * priv, u8* buffer,struct ieee80211_rx_stats * pprevious_stats, struct ieee80211_rx_stats * pcurrent_stats)
{
	bool bcheck = false;
	u8	rfpath;
	u32 nspatial_stream, tmp_val;
	//u8	i;
	static u32 slide_rssi_index=0, slide_rssi_statistics=0; 
	static u32 slide_evm_index=0, slide_evm_statistics=0;
	static u32 last_rssi=0, last_evm=0;
	//cosa add for rx path selection 
//	static long slide_cck_adc_pwdb_index=0, slide_cck_adc_pwdb_statistics=0;
//	static char last_cck_adc_pwdb[4]={0,0,0,0};
	//cosa add for beacon rssi smoothing
	static u32 slide_beacon_adc_pwdb_index=0, slide_beacon_adc_pwdb_statistics=0;
	static u32 last_beacon_adc_pwdb=0;

	struct ieee80211_hdr_3addr *hdr;
	u16 sc ;
	unsigned int frag,seq;
	hdr = (struct ieee80211_hdr_3addr *)buffer;
	sc = le16_to_cpu(hdr->seq_ctl);
	frag = WLAN_GET_SEQ_FRAG(sc);
	seq = WLAN_GET_SEQ_SEQ(sc);
	//cosa add 04292008 to record the sequence number
	pcurrent_stats->Seq_Num = seq;
	//
	// Check whether we should take the previous packet into accounting
	//
	if(!pprevious_stats->bIsAMPDU)
	{
		// if previous packet is not aggregated packet
		bcheck = true;
	}else 
	{
//remve for that we don't use AMPDU to calculate PWDB,because the reported PWDB of some AP is fault.
#if 0
		// if previous packet is aggregated packet, and current packet
		//	(1) is not AMPDU
		//	(2) is the first packet of one AMPDU
		// that means the previous packet is the last one aggregated packet
		if( !pcurrent_stats->bIsAMPDU || pcurrent_stats->bFirstMPDU)
			bcheck = true;
#endif
	}
		
	if(slide_rssi_statistics++ >= PHY_RSSI_SLID_WIN_MAX)
	{
		slide_rssi_statistics = PHY_RSSI_SLID_WIN_MAX;
		last_rssi = priv->stats.slide_signal_strength[slide_rssi_index];
		priv->stats.slide_rssi_total -= last_rssi;
	}
	priv->stats.slide_rssi_total += pprevious_stats->SignalStrength;
	
	priv->stats.slide_signal_strength[slide_rssi_index++] = pprevious_stats->SignalStrength;
	if(slide_rssi_index >= PHY_RSSI_SLID_WIN_MAX)
		slide_rssi_index = 0;
	
	// <1> Showed on UI for user, in dbm
	tmp_val = priv->stats.slide_rssi_total/slide_rssi_statistics;
	priv->stats.signal_strength = rtl819x_translate_todbm((u8)tmp_val);
	pcurrent_stats->rssi = priv->stats.signal_strength;
	//
	// If the previous packet does not match the criteria, neglect it
	//
	if(!pprevious_stats->bPacketMatchBSSID)
	{
		if(!pprevious_stats->bToSelfBA)
			return;
	}
	
	if(!bcheck)
		return;

	rtl8190_process_cck_rxpathsel(priv,pprevious_stats);

	//
	// Check RSSI
	//
	priv->stats.num_process_phyinfo++;
#if 0
	/* record the general signal strength to the sliding window. */
	if(slide_rssi_statistics++ >= PHY_RSSI_SLID_WIN_MAX)
	{
		slide_rssi_statistics = PHY_RSSI_SLID_WIN_MAX;
		last_rssi = priv->stats.slide_signal_strength[slide_rssi_index];
		priv->stats.slide_rssi_total -= last_rssi;
	}
	priv->stats.slide_rssi_total += pprevious_stats->SignalStrength;
	
	priv->stats.slide_signal_strength[slide_rssi_index++] = pprevious_stats->SignalStrength;
	if(slide_rssi_index >= PHY_RSSI_SLID_WIN_MAX)
		slide_rssi_index = 0;
	
	// <1> Showed on UI for user, in dbm
	tmp_val = priv->stats.slide_rssi_total/slide_rssi_statistics;
	priv->stats.signal_strength = rtl819x_translate_todbm((u8)tmp_val);

#endif
	// <2> Showed on UI for engineering
	// hardware does not provide rssi information for each rf path in CCK
	if(!pprevious_stats->bIsCCK && pprevious_stats->bPacketToSelf)
	{
		for (rfpath = RF90_PATH_A; rfpath < RF90_PATH_C; rfpath++)
		{
			if (!rtl8192_phy_CheckIsLegalRFPath(priv->ieee80211->dev, rfpath))
				continue;
			RT_TRACE(COMP_DBG,"Jacken -> pPreviousstats->RxMIMOSignalStrength[rfpath]  = %d \n" ,pprevious_stats->RxMIMOSignalStrength[rfpath] );		 
			//Fixed by Jacken 2008-03-20
			if(priv->stats.rx_rssi_percentage[rfpath] == 0)
			{
				priv->stats.rx_rssi_percentage[rfpath] = pprevious_stats->RxMIMOSignalStrength[rfpath];
				//DbgPrint("MIMO RSSI initialize \n");
			}	
			if(pprevious_stats->RxMIMOSignalStrength[rfpath]  > priv->stats.rx_rssi_percentage[rfpath])
			{
				priv->stats.rx_rssi_percentage[rfpath] = 
					( (priv->stats.rx_rssi_percentage[rfpath]*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxMIMOSignalStrength[rfpath])) /(Rx_Smooth_Factor);
				priv->stats.rx_rssi_percentage[rfpath] = priv->stats.rx_rssi_percentage[rfpath]  + 1;
			}
			else
			{
				priv->stats.rx_rssi_percentage[rfpath] = 
					( (priv->stats.rx_rssi_percentage[rfpath]*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxMIMOSignalStrength[rfpath])) /(Rx_Smooth_Factor);
			}	
			RT_TRACE(COMP_DBG,"Jacken -> priv->RxStats.RxRSSIPercentage[rfPath]  = %d \n" ,priv->stats.rx_rssi_percentage[rfpath] );
		}		
	}
	
	
	//
	// Check PWDB.
	//
	//cosa add for beacon rssi smoothing by average.
	if(pprevious_stats->bPacketBeacon)
	{
		/* record the beacon pwdb to the sliding window. */
		if(slide_beacon_adc_pwdb_statistics++ >= PHY_Beacon_RSSI_SLID_WIN_MAX)
		{
			slide_beacon_adc_pwdb_statistics = PHY_Beacon_RSSI_SLID_WIN_MAX;	
			last_beacon_adc_pwdb = priv->stats.Slide_Beacon_pwdb[slide_beacon_adc_pwdb_index];
			priv->stats.Slide_Beacon_Total -= last_beacon_adc_pwdb;
			//DbgPrint("slide_beacon_adc_pwdb_index = %d, last_beacon_adc_pwdb = %d, dev->RxStats.Slide_Beacon_Total = %d\n", 
			//	slide_beacon_adc_pwdb_index, last_beacon_adc_pwdb, dev->RxStats.Slide_Beacon_Total);
		}
		priv->stats.Slide_Beacon_Total += pprevious_stats->RxPWDBAll;
		priv->stats.Slide_Beacon_pwdb[slide_beacon_adc_pwdb_index] = pprevious_stats->RxPWDBAll;
		//DbgPrint("slide_beacon_adc_pwdb_index = %d, pPreviousRfd->Status.RxPWDBAll = %d\n", slide_beacon_adc_pwdb_index, pPreviousRfd->Status.RxPWDBAll);
		slide_beacon_adc_pwdb_index++;
		if(slide_beacon_adc_pwdb_index >= PHY_Beacon_RSSI_SLID_WIN_MAX)
			slide_beacon_adc_pwdb_index = 0;
		pprevious_stats->RxPWDBAll = priv->stats.Slide_Beacon_Total/slide_beacon_adc_pwdb_statistics;
		if(pprevious_stats->RxPWDBAll >= 3)
			pprevious_stats->RxPWDBAll -= 3;
	}

	RT_TRACE(COMP_RXDESC, "Smooth %s PWDB = %d\n", 
				pprevious_stats->bIsCCK? "CCK": "OFDM",
				pprevious_stats->RxPWDBAll);

	if(pprevious_stats->bPacketToSelf || pprevious_stats->bPacketBeacon || pprevious_stats->bToSelfBA)
	{
		if(priv->undecorated_smoothed_pwdb < 0)	// initialize
		{
			priv->undecorated_smoothed_pwdb = pprevious_stats->RxPWDBAll;
			//DbgPrint("First pwdb initialize \n");
		}
#if 1
		if(pprevious_stats->RxPWDBAll > (u32)priv->undecorated_smoothed_pwdb)
		{
			priv->undecorated_smoothed_pwdb =	
					( ((priv->undecorated_smoothed_pwdb)*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxPWDBAll)) /(Rx_Smooth_Factor);
			priv->undecorated_smoothed_pwdb = priv->undecorated_smoothed_pwdb + 1;
		}
		else
		{
			priv->undecorated_smoothed_pwdb =	
					( ((priv->undecorated_smoothed_pwdb)*(Rx_Smooth_Factor-1)) + 
					(pprevious_stats->RxPWDBAll)) /(Rx_Smooth_Factor);
		}
#else
		//Fixed by Jacken 2008-03-20
		if(pPreviousRfd->Status.RxPWDBAll > (u32)pHalData->UndecoratedSmoothedPWDB)
		{
			pHalData->UndecoratedSmoothedPWDB = 	
					( ((pHalData->UndecoratedSmoothedPWDB)* 5) + (pPreviousRfd->Status.RxPWDBAll)) / 6;
			pHalData->UndecoratedSmoothedPWDB = pHalData->UndecoratedSmoothedPWDB + 1;
		}
		else
		{
			pHalData->UndecoratedSmoothedPWDB = 	
					( ((pHalData->UndecoratedSmoothedPWDB)* 5) + (pPreviousRfd->Status.RxPWDBAll)) / 6;
		}		
#endif
		rtl819x_update_rxsignalstatistics8190pci(priv,pprevious_stats);
	}

	//
	// Check EVM
	//	
	/* record the general EVM to the sliding window. */
	if(pprevious_stats->SignalQuality == 0)
	{
	}
	else
	{
		if(pprevious_stats->bPacketToSelf || pprevious_stats->bPacketBeacon || pprevious_stats->bToSelfBA){
			if(slide_evm_statistics++ >= PHY_RSSI_SLID_WIN_MAX){
				slide_evm_statistics = PHY_RSSI_SLID_WIN_MAX;
				last_evm = priv->stats.slide_evm[slide_evm_index];
				priv->stats.slide_evm_total -= last_evm;
			}
	
			priv->stats.slide_evm_total += pprevious_stats->SignalQuality;
	
			priv->stats.slide_evm[slide_evm_index++] = pprevious_stats->SignalQuality;
			if(slide_evm_index >= PHY_RSSI_SLID_WIN_MAX)
				slide_evm_index = 0;
	
			// <1> Showed on UI for user, in percentage.
			tmp_val = priv->stats.slide_evm_total/slide_evm_statistics;
			priv->stats.signal_quality = tmp_val;
			//cosa add 10/11/2007, Showed on UI for user in Windows Vista, for Link quality.
			priv->stats.last_signal_strength_inpercent = tmp_val;
		}

		// <2> Showed on UI for engineering
		if(pprevious_stats->bPacketToSelf || pprevious_stats->bPacketBeacon || pprevious_stats->bToSelfBA)
		{
			for(nspatial_stream = 0; nspatial_stream<2 ; nspatial_stream++) // 2 spatial stream
			{
				if(pprevious_stats->RxMIMOSignalQuality[nspatial_stream] != -1)
				{
					if(priv->stats.rx_evm_percentage[nspatial_stream] == 0)	// initialize
					{
						priv->stats.rx_evm_percentage[nspatial_stream] = pprevious_stats->RxMIMOSignalQuality[nspatial_stream];
					}
					priv->stats.rx_evm_percentage[nspatial_stream] = 
						( (priv->stats.rx_evm_percentage[nspatial_stream]* (Rx_Smooth_Factor-1)) + 
						(pprevious_stats->RxMIMOSignalQuality[nspatial_stream]* 1)) / (Rx_Smooth_Factor);
				}
			}
		}
	}
	
}

/*-----------------------------------------------------------------------------
 * Function:	rtl819x_query_rxpwrpercentage()
 *
 * Overview:	
 *
 * Input:		char		antpower
 *
 * Output:		NONE
 *
 * Return:		0-100 percentage
 *
 * Revised History:
 *	When		Who 	Remark
 *	05/26/2008	amy 	Create Version 0 porting from windows code.  
 *
 *---------------------------------------------------------------------------*/
//#ifndef RTL8192SE
static u8 rtl819x_query_rxpwrpercentage(
	char		antpower
	)
{
	if ((antpower <= -100) || (antpower >= 20))
	{
		return	0;
	}
	else if (antpower >= 0)
	{
		return	100;
	}
	else
	{
		return	(100+antpower);
	}
	
}	/* QueryRxPwrPercentage */

static u8 
rtl819x_evm_dbtopercentage(
	char value
	)
{
	char ret_val;
	
	ret_val = value;
	
	if(ret_val >= 0)
		ret_val = 0;
	if(ret_val <= -33)
		ret_val = -33;
	ret_val = 0 - ret_val;
	ret_val*=3;
	if(ret_val == 99)
		ret_val = 100;
	return(ret_val);
}
//#endif
//
//	Description:
//	We want good-looking for signal strength/quality
//	2007/7/19 01:09, by cosa.
//
long
rtl819x_signal_scale_mapping(
	long currsig
	)
{
	long retsig;

	// Step 1. Scale mapping.
	if(currsig >= 61 && currsig <= 100)
	{
		retsig = 90 + ((currsig - 60) / 4);
	}
	else if(currsig >= 41 && currsig <= 60)
	{
		retsig = 78 + ((currsig - 40) / 2);
	}
	else if(currsig >= 31 && currsig <= 40)
	{
		retsig = 66 + (currsig - 30);
	}
	else if(currsig >= 21 && currsig <= 30)
	{
		retsig = 54 + (currsig - 20);
	}
	else if(currsig >= 5 && currsig <= 20)
	{
		retsig = 42 + (((currsig - 5) * 2) / 3);
	}
	else if(currsig == 4)
	{
		retsig = 36;
	}
	else if(currsig == 3)
	{
		retsig = 27;
	}
	else if(currsig == 2)
	{
		retsig = 18;
	}
	else if(currsig == 1)
	{
		retsig = 9;
	}
	else
	{
		retsig = currsig;
	}
	
	return retsig;
}
#ifdef RTL8192SE
#define 	rx_hal_is_cck_rate(_pdesc)\
			(_pdesc->RxMCS == DESC92S_RATE1M ||\
			 _pdesc->RxMCS == DESC92S_RATE2M ||\
			 _pdesc->RxMCS == DESC92S_RATE5_5M ||\
			 _pdesc->RxMCS == DESC92S_RATE11M)
static void rtl8192_query_rxphystatus(
	struct r8192_priv * priv,
	struct ieee80211_rx_stats * pstats,
	prx_desc  pdesc,	
	prx_fwinfo   pDrvInfo,
	struct ieee80211_rx_stats * precord_stats,
	bool bpacket_match_bssid,
	bool bpacket_toself,
	bool bPacketBeacon,
	bool bToSelfBA
	)
{
	bool is_cck_rate;
	phy_sts_cck_8192s_t* cck_buf;
	u8 rx_pwr_all=0, rx_pwr[4], rf_rx_num=0, EVM, PWDB_ALL;
	u8 i, max_spatial_stream;
	u32 rssi, total_rssi = 0;

	is_cck_rate = rx_hal_is_cck_rate(pdesc);

	memset(precord_stats, 0, sizeof(struct ieee80211_rx_stats));
	pstats->bPacketMatchBSSID = precord_stats->bPacketMatchBSSID = bpacket_match_bssid;
	pstats->bPacketToSelf = precord_stats->bPacketToSelf = bpacket_toself;
	pstats->bIsCCK = precord_stats->bIsCCK = is_cck_rate;//RX_HAL_IS_CCK_RATE(pDrvInfo);
	pstats->bPacketBeacon = precord_stats->bPacketBeacon = bPacketBeacon;
	pstats->bToSelfBA = precord_stats->bToSelfBA = bToSelfBA;
	pstats->bIsCCK = precord_stats->bIsCCK = is_cck_rate;

	pstats->RxMIMOSignalQuality[0] = -1;
	pstats->RxMIMOSignalQuality[1] = -1;
	precord_stats->RxMIMOSignalQuality[0] = -1;
	precord_stats->RxMIMOSignalQuality[1] = -1;

	if (is_cck_rate){
		u8 report, cck_highpwr;
		//s8 cck_adc_pwdb[4];
			
		cck_buf = (phy_sts_cck_8192s_t*)pDrvInfo;

		if(!priv->bInPowerSaveMode)
		cck_highpwr = (u8)rtl8192_QueryBBReg(priv->ieee80211->dev, rFPGA0_XA_HSSIParameter2, BIT9);
		else
			cck_highpwr = false;
		if (!cck_highpwr){
			report = cck_buf->cck_agc_rpt & 0xc0;
			report = report >> 6;
			switch(report){
				case 0x3:
					rx_pwr_all = -35 - (cck_buf->cck_agc_rpt&0x3e);
					break;
				case 0x2:
					rx_pwr_all = -23 - (cck_buf->cck_agc_rpt&0x3e);
					break;
				case 0x1:
					rx_pwr_all = -11 - (cck_buf->cck_agc_rpt&0x3e);
					break;
				case 0x0:
					rx_pwr_all = 8 - (cck_buf->cck_agc_rpt&0x3e);
					break;
			}
		}
		else{
			report = pDrvInfo->cfosho[0] & 0x60;
			report = report >> 5;
			switch(report){
				case 0x3:
					rx_pwr_all = -35 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x2:
					rx_pwr_all = -23 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x1:
					rx_pwr_all = -11 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x0:
					rx_pwr_all = -8 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
			}
		}

		PWDB_ALL= rtl819x_query_rxpwrpercentage(rx_pwr_all);
		pstats->RxPWDBAll = precord_stats->RxPWDBAll = PWDB_ALL;
		pstats->RecvSignalPower = rx_pwr_all;

		//get Signal Quality(EVM)
		if (bpacket_match_bssid){
			u8 sq;
			if (pstats->RxPWDBAll > 40)
				sq = 100;
			else{
				sq = cck_buf->sq_rpt;
				if (sq > 64)
					sq = 0;
				else if(sq < 20)
					sq = 100;
				else
					sq = ((64-sq)*100)/44;
			}
			pstats->SignalQuality = precord_stats->SignalQuality = sq;
			pstats->RxMIMOSignalQuality[0] = precord_stats->RxMIMOSignalQuality[0] = sq;
			pstats->RxMIMOSignalQuality[1] = precord_stats->RxMIMOSignalQuality[1] = -1;
		}
	}
	else{
		priv->brfpath_rxenable[0] = priv->brfpath_rxenable[1] = true;

		for (i=RF90_PATH_A; i<RF90_PATH_MAX; i++){
			if (priv->brfpath_rxenable[i])
				rf_rx_num ++;
			//else
			//	break;

			rx_pwr[i] = ((pDrvInfo->gain_trsw[i]&0x3f)*2) - 110;
			rssi = rtl819x_query_rxpwrpercentage(rx_pwr[i]);
			total_rssi += rssi;

			priv->stats.rxSNRdB[i] = (long)(pDrvInfo->rxsnr[i]/2);

			if (bpacket_match_bssid){
				pstats->RxMIMOSignalStrength[i] = (u8)rssi;
				precord_stats->RxMIMOSignalStrength [i] = (u8)rssi;
			}
		}

		rx_pwr_all = ((pDrvInfo->pwdb_all >> 1) & 0x7f) - 0x106;
		PWDB_ALL = rtl819x_query_rxpwrpercentage(rx_pwr_all);

		pstats->RxPWDBAll = precord_stats->RxPWDBAll = PWDB_ALL;
		pstats->RxPower = precord_stats->RxPower = rx_pwr_all;
		pstats->RecvSignalPower = precord_stats->RecvSignalPower = rx_pwr_all;

		//EVM of HT rate
		if (pdesc->RxHT && pdesc->RxMCS >= DESC90_RATEMCS8 && pdesc->RxMCS <= DESC90_RATEMCS15)
			max_spatial_stream = 2;
		else
			max_spatial_stream = 1;

		for(i=0; i<max_spatial_stream; i++){
			EVM = rtl819x_evm_dbtopercentage(pDrvInfo->rxevm[i]);

			if (bpacket_match_bssid)
			{
				if (i==0)
					pstats->SignalQuality = 
					precord_stats->SignalQuality = (u8)(EVM&0xff);
				pstats->RxMIMOSignalQuality[i] =
				precord_stats->RxMIMOSignalQuality[i] = (u8)(EVM&0xff);
			}
		}
#if 0
		if (pdesc->BW)
			priv->stats.received_bwtype[1+pDrvInfo->rxsc]++;
		else
			priv->stats.received_bwtype[0]++;
#endif
	}

	if (is_cck_rate)
		pstats->SignalStrength = 
		precord_stats->SignalStrength = (u8)(rtl819x_signal_scale_mapping(PWDB_ALL));
	else
		if (rf_rx_num != 0)
			pstats->SignalStrength = 
			precord_stats->SignalStrength = (u8)(rtl819x_signal_scale_mapping(total_rssi/=rf_rx_num));

}
#else //RTL8192SE





#define 	rx_hal_is_cck_rate(_pdrvinfo)\
			(_pdrvinfo->RxRate == DESC90_RATE1M ||\
			_pdrvinfo->RxRate == DESC90_RATE2M ||\
			_pdrvinfo->RxRate == DESC90_RATE5_5M ||\
			_pdrvinfo->RxRate == DESC90_RATE11M) &&\
			!_pdrvinfo->RxHT
static void rtl8192_query_rxphystatus(
	struct r8192_priv * priv,
	struct ieee80211_rx_stats * pstats,
	prx_desc  pdesc,	
	prx_fwinfo   pdrvinfo,
	struct ieee80211_rx_stats * precord_stats,
	bool bpacket_match_bssid,
	bool bpacket_toself,
	bool bPacketBeacon,
	bool bToSelfBA
	)
{	
	//PRT_RFD_STATUS		pRtRfdStatus = &(pRfd->Status); 
	phy_sts_ofdm_819xpci_t* pofdm_buf;
	phy_sts_cck_819xpci_t	*	pcck_buf;
	phy_ofdm_rx_status_rxsc_sgien_exintfflag* prxsc;
	u8				*prxpkt;
	u8				i,max_spatial_stream, tmp_rxsnr, tmp_rxevm, rxsc_sgien_exflg;
	char				rx_pwr[4], rx_pwr_all=0;
	//long				rx_avg_pwr = 0;
	char				rx_snrX, rx_evmX;
	u8				evm, pwdb_all;
	u32 			RSSI, total_rssi=0;//, total_evm=0;
//	long				signal_strength_index = 0;
	u8				is_cck_rate=0;
	u8				rf_rx_num = 0;

	/* 2007/07/04 MH For OFDM RSSI. For high power or not. */
	static	u8		check_reg824 = 0;
	static	u32		reg824_bit9 = 0;

	priv->stats.numqry_phystatus++;


	is_cck_rate = rx_hal_is_cck_rate(pdrvinfo);
	// Record it for next packet processing
	memset(precord_stats, 0, sizeof(struct ieee80211_rx_stats));
	pstats->bPacketMatchBSSID = precord_stats->bPacketMatchBSSID = bpacket_match_bssid;
	pstats->bPacketToSelf = precord_stats->bPacketToSelf = bpacket_toself;
	pstats->bIsCCK = precord_stats->bIsCCK = is_cck_rate;//RX_HAL_IS_CCK_RATE(pDrvInfo);
	pstats->bPacketBeacon = precord_stats->bPacketBeacon = bPacketBeacon;
	pstats->bToSelfBA = precord_stats->bToSelfBA = bToSelfBA;
	/*2007.08.30 requested by SD3 Jerry */
	if(check_reg824 == 0)
	{	
		reg824_bit9 = rtl8192_QueryBBReg(priv->ieee80211->dev, rFPGA0_XA_HSSIParameter2, 0x200);
		check_reg824 = 1;	
	}


	prxpkt = (u8*)pdrvinfo; 
	
	/* Move pointer to the 16th bytes. Phy status start address. */ 
	prxpkt += sizeof(rx_fwinfo);	
	
	/* Initial the cck and ofdm buffer pointer */
	pcck_buf = (phy_sts_cck_819xpci_t *)prxpkt;
	pofdm_buf = (phy_sts_ofdm_819xpci_t *)prxpkt;			
			
	pstats->RxMIMOSignalQuality[0] = -1;
	pstats->RxMIMOSignalQuality[1] = -1;
	precord_stats->RxMIMOSignalQuality[0] = -1;
	precord_stats->RxMIMOSignalQuality[1] = -1;
			
	if(is_cck_rate)
	{
		// 
		// (1)Hardware does not provide RSSI for CCK
		//

		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//
		u8 report;//, cck_agc_rpt;
#ifdef RTL8190P
		u8 tmp_pwdb; 
		char cck_adc_pwdb[4];
#endif
		priv->stats.numqry_phystatusCCK++;

#ifdef RTL8190P	//Only 90P 2T4R need to check
		if(priv->rf_type == RF_2T4R && DM_RxPathSelTable.Enable && bpacket_match_bssid)
		{
			for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
			{
				tmp_pwdb = pcck_buf->adc_pwdb_X[i];
				cck_adc_pwdb[i] = (char)tmp_pwdb;
				cck_adc_pwdb[i] /= 2;
				pstats->cck_adc_pwdb[i] = precord_stats->cck_adc_pwdb[i] = cck_adc_pwdb[i];
				//DbgPrint("RF-%d tmp_pwdb = 0x%x, cck_adc_pwdb = %d", i, tmp_pwdb, cck_adc_pwdb[i]);
			}
		}
#endif

		if(!reg824_bit9)
		{
			report = pcck_buf->cck_agc_rpt & 0xc0;
			report = report>>6;
			switch(report)
			{
				//Fixed by Jacken from Bryant 2008-03-20
				//Original value is -38 , -26 , -14 , -2
				//Fixed value is -35 , -23 , -11 , 6
				case 0x3:
					rx_pwr_all = -35 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x2:
					rx_pwr_all = -23 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x1:
					rx_pwr_all = -11 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x0:
					rx_pwr_all = 8 - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
			}
		}
		else
		{
			report = pcck_buf->cck_agc_rpt & 0x60;
			report = report>>5;
			switch(report)
			{
				case 0x3:
					rx_pwr_all = -35 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x2:
					rx_pwr_all = -23 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x1:
					rx_pwr_all = -11 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x0:
					rx_pwr_all = -8 - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
			}
		}

		pwdb_all = rtl819x_query_rxpwrpercentage(rx_pwr_all);
		pstats->RxPWDBAll = precord_stats->RxPWDBAll = pwdb_all;
		pstats->RecvSignalPower = rx_pwr_all;

		//
		// (3) Get Signal Quality (EVM)
		//
		if(bpacket_match_bssid)
		{
			u8	sq;

			if(pstats->RxPWDBAll > 40)
			{
				sq = 100;
			}else
			{
				sq = pcck_buf->sq_rpt;
				
				if(pcck_buf->sq_rpt > 64)
					sq = 0;
				else if (pcck_buf->sq_rpt < 20)
					sq = 100;
				else
					sq = ((64-sq) * 100) / 44;
			}
			pstats->SignalQuality = precord_stats->SignalQuality = sq;
			pstats->RxMIMOSignalQuality[0] = precord_stats->RxMIMOSignalQuality[0] = sq;
			pstats->RxMIMOSignalQuality[1] = precord_stats->RxMIMOSignalQuality[1] = -1;
		}
	}
	else
	{
		priv->stats.numqry_phystatusHT++;
		// 
		// (1)Get RSSI for HT rate
		//
		for(i=RF90_PATH_A; i<RF90_PATH_MAX; i++)
		{
			// 2008/01/30 MH we will judge RF RX path now.
			if (priv->brfpath_rxenable[i])
				rf_rx_num++;
			//else
				//continue;

			//Fixed by Jacken from Bryant 2008-03-20
			//Original value is 106
#ifdef RTL8190P	   //Modify by Jacken 2008/03/31
			rx_pwr[i] = ((pofdm_buf->trsw_gain_X[i]&0x3F)*2) - 106;
#else
			rx_pwr[i] = ((pofdm_buf->trsw_gain_X[i]&0x3F)*2) - 110;
#endif

			//Get Rx snr value in DB
			tmp_rxsnr = pofdm_buf->rxsnr_X[i];
			rx_snrX = (char)(tmp_rxsnr);
			rx_snrX /= 2;
			priv->stats.rxSNRdB[i] = (long)rx_snrX;
			
			/* Translate DBM to percentage. */
			RSSI = rtl819x_query_rxpwrpercentage(rx_pwr[i]);	
			if (priv->brfpath_rxenable[i])
				total_rssi += RSSI;

			/* Record Signal Strength for next packet */
			if(bpacket_match_bssid)
			{
				pstats->RxMIMOSignalStrength[i] =(u8) RSSI;
				precord_stats->RxMIMOSignalStrength[i] =(u8) RSSI;
			}
		}
		
		
		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//
		//Fixed by Jacken from Bryant 2008-03-20
		//Original value is 106
		rx_pwr_all = (((pofdm_buf->pwdb_all ) >> 1 )& 0x7f) -106;
		pwdb_all = rtl819x_query_rxpwrpercentage(rx_pwr_all);	

		pstats->RxPWDBAll = precord_stats->RxPWDBAll = pwdb_all;
		pstats->RxPower = precord_stats->RxPower =	rx_pwr_all;
		pstats->RecvSignalPower = rx_pwr_all;
		//
		// (3)EVM of HT rate
		//
		if(pdrvinfo->RxHT && pdrvinfo->RxRate>=DESC90_RATEMCS8 &&
			pdrvinfo->RxRate<=DESC90_RATEMCS15)
			max_spatial_stream = 2; //both spatial stream make sense
		else
			max_spatial_stream = 1; //only spatial stream 1 makes sense

		for(i=0; i<max_spatial_stream; i++)
		{
			tmp_rxevm = pofdm_buf->rxevm_X[i];
			rx_evmX = (char)(tmp_rxevm);
			
			// Do not use shift operation like "rx_evmX >>= 1" because the compilor of free build environment
			// fill most significant bit to "zero" when doing shifting operation which may change a negative 
			// value to positive one, then the dbm value (which is supposed to be negative)  is not correct anymore.
			rx_evmX /= 2;	//dbm

			evm = rtl819x_evm_dbtopercentage(rx_evmX);
#if 0			
			EVM = SignalScaleMapping(EVM);//make it good looking, from 0~100
#endif
			if(bpacket_match_bssid)
			{
				if(i==0) // Fill value in RFD, Get the first spatial stream only
					pstats->SignalQuality = precord_stats->SignalQuality = (u8)(evm & 0xff);
				pstats->RxMIMOSignalQuality[i] = precord_stats->RxMIMOSignalQuality[i] = (u8)(evm & 0xff);
			}
		}

		
		/* record rx statistics for debug */
		rxsc_sgien_exflg = pofdm_buf->rxsc_sgien_exflg;
		prxsc = (phy_ofdm_rx_status_rxsc_sgien_exintfflag *)&rxsc_sgien_exflg;
		if(pdrvinfo->BW)	//40M channel
			priv->stats.received_bwtype[1+prxsc->rxsc]++;
		else				//20M channel
			priv->stats.received_bwtype[0]++;
	}

	//UI BSS List signal strength(in percentage), make it good looking, from 0~100.
	//It is assigned to the BSS List in GetValueFromBeaconOrProbeRsp().
	if(is_cck_rate)
	{
		pstats->SignalStrength = precord_stats->SignalStrength = (u8)(rtl819x_signal_scale_mapping((long)pwdb_all));//PWDB_ALL;
		
	}
	else
	{
		//pRfd->Status.SignalStrength = pRecordRfd->Status.SignalStrength = (u8)(SignalScaleMapping(total_rssi/=RF90_PATH_MAX));//(u8)(total_rssi/=RF90_PATH_MAX);
		// We can judge RX path number now.
		if (rf_rx_num != 0)
			pstats->SignalStrength = precord_stats->SignalStrength = (u8)(rtl819x_signal_scale_mapping((long)(total_rssi/=rf_rx_num))); 	
	}
}	/* QueryRxPhyStatus8190Pci */

#endif //92SE

void
rtl8192_record_rxdesc_forlateruse(
	struct ieee80211_rx_stats * psrc_stats,
	struct ieee80211_rx_stats * ptarget_stats
)
{
	ptarget_stats->bIsAMPDU = psrc_stats->bIsAMPDU;
	ptarget_stats->bFirstMPDU = psrc_stats->bFirstMPDU;
	//ptarget_stats->Seq_Num = psrc_stats->Seq_Num;
}


void TranslateRxSignalStuff819xpci(struct net_device *dev, 
        struct sk_buff *skb,
        struct ieee80211_rx_stats * pstats,
        prx_desc pdesc,	
        prx_fwinfo pdrvinfo)
{
    // TODO: We must only check packet for current MAC address. Not finish
    struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
    bool bpacket_match_bssid, bpacket_toself;
    bool bPacketBeacon=false, bToSelfBA=false;
    static struct ieee80211_rx_stats  previous_stats;
    struct ieee80211_hdr_3addr *hdr;
    u16 fc,type;

    // Get Signal Quality for only RX data queue (but not command queue)

    u8* tmp_buf;
    u8	*praddr;

    /* Get MAC frame start address. */		
    tmp_buf = skb->data + pstats->RxDrvInfoSize + pstats->RxBufShift;

    hdr = (struct ieee80211_hdr_3addr *)tmp_buf;
    fc = le16_to_cpu(hdr->frame_ctl);
    type = WLAN_FC_GET_TYPE(fc);	
    praddr = hdr->addr1;

    /* Check if the received packet is acceptabe. */
    bpacket_match_bssid = ((IEEE80211_FTYPE_CTL != type) &&
            (eqMacAddr(priv->ieee80211->current_network.bssid,	(fc & IEEE80211_FCTL_TODS)? hdr->addr1 : (fc & IEEE80211_FCTL_FROMDS )? hdr->addr2 : hdr->addr3))
            && (!pstats->bHwError) && (!pstats->bCRC)&& (!pstats->bICV));
    bpacket_toself =  bpacket_match_bssid & (eqMacAddr(praddr, priv->ieee80211->dev->dev_addr));
#if 1//cosa
    if(WLAN_FC_GET_FRAMETYPE(fc)== IEEE80211_STYPE_BEACON)
    {
        bPacketBeacon = true;
        //DbgPrint("Beacon 2, MatchBSSID = %d, ToSelf = %d \n", bPacketMatchBSSID, bPacketToSelf);
    }
    if(WLAN_FC_GET_FRAMETYPE(fc) == IEEE80211_STYPE_BLOCKACK)
    {
        if((eqMacAddr(praddr,dev->dev_addr)))
            bToSelfBA = true;
        //DbgPrint("BlockAck, MatchBSSID = %d, ToSelf = %d \n", bPacketMatchBSSID, bPacketToSelf);
    }

#endif	
    if(bpacket_match_bssid)
    {
        priv->stats.numpacket_matchbssid++;
    }
    if(bpacket_toself){
        priv->stats.numpacket_toself++;
    }
    //
    // Process PHY information for previous packet (RSSI/PWDB/EVM)
    //
    // Because phy information is contained in the last packet of AMPDU only, so driver 
    // should process phy information of previous packet	
    rtl8192_process_phyinfo(priv, tmp_buf,&previous_stats, pstats);
    rtl8192_query_rxphystatus(priv, pstats, pdesc, pdrvinfo, &previous_stats, bpacket_match_bssid,
            bpacket_toself ,bPacketBeacon, bToSelfBA);
    rtl8192_record_rxdesc_forlateruse(pstats, &previous_stats);

}


//#endif


/**
* Function:	UpdateReceivedRateHistogramStatistics
* Overview:	Recored down the received data rate
* 
* Input:		
* 	PADAPTER	dev
*	PRT_RFD		pRfd,
* 			
* Output:		
*	PRT_TCB		dev
*				(dev->RxStats.ReceivedRateHistogram[] is updated)
* Return:     	
*		None
*/
void UpdateReceivedRateHistogramStatistics8190(
	struct net_device *dev,
	struct ieee80211_rx_stats* pstats
	)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u32 rcvType=1;   //0: Total, 1:OK, 2:CRC, 3:ICV
	u32 rateIndex;
	u32 preamble_guardinterval;  //1: short preamble/GI, 0: long preamble/GI
	    
	/* 2007/03/09 MH We will not update rate of packet from rx cmd queue. */
	#if 0
	if (pRfd->queue_id == CMPK_RX_QUEUE_ID)
		return;
	#endif
	if(pstats->bCRC)
		rcvType = 2;
	else if(pstats->bICV)
		rcvType = 3;
	    
	if(pstats->bShortPreamble)
		preamble_guardinterval = 1;// short
	else
		preamble_guardinterval = 0;// long

	switch(pstats->rate)
	{
		//
		// CCK rate
		//
		case MGN_1M:    rateIndex = 0;  break;
	    	case MGN_2M:    rateIndex = 1;  break;
	    	case MGN_5_5M:  rateIndex = 2;  break;
	    	case MGN_11M:   rateIndex = 3;  break;
		//
		// Legacy OFDM rate
		//
	    	case MGN_6M:    rateIndex = 4;  break;
	    	case MGN_9M:    rateIndex = 5;  break;
	    	case MGN_12M:   rateIndex = 6;  break;
	    	case MGN_18M:   rateIndex = 7;  break;
	    	case MGN_24M:   rateIndex = 8;  break;
	    	case MGN_36M:   rateIndex = 9;  break;
	    	case MGN_48M:   rateIndex = 10; break;
	    	case MGN_54M:   rateIndex = 11; break;
		//
		// 11n High throughput rate
		//
	    	case MGN_MCS0:  rateIndex = 12; break;
	    	case MGN_MCS1:  rateIndex = 13; break;
	    	case MGN_MCS2:  rateIndex = 14; break;
	    	case MGN_MCS3:  rateIndex = 15; break;
	    	case MGN_MCS4:  rateIndex = 16; break;
	    	case MGN_MCS5:  rateIndex = 17; break;
	    	case MGN_MCS6:  rateIndex = 18; break;
	    	case MGN_MCS7:  rateIndex = 19; break;
	    	case MGN_MCS8:  rateIndex = 20; break;
	    	case MGN_MCS9:  rateIndex = 21; break;
	    	case MGN_MCS10: rateIndex = 22; break;
	    	case MGN_MCS11: rateIndex = 23; break;
	    	case MGN_MCS12: rateIndex = 24; break;
	    	case MGN_MCS13: rateIndex = 25; break;
	    	case MGN_MCS14: rateIndex = 26; break;
	    	case MGN_MCS15: rateIndex = 27; break;
		default:        rateIndex = 28; break;
	}
	priv->stats.received_preamble_GI[preamble_guardinterval][rateIndex]++;
	priv->stats.received_rate_histogram[0][rateIndex]++; //total
	priv->stats.received_rate_histogram[rcvType][rateIndex]++;
}

void rtl8192_rx(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct ieee80211_hdr_1addr *ieee80211_hdr = NULL;
	bool unicast_packet = false;
	bool bLedBlinking=TRUE;
	u16 fc=0, type=0;
	u32 skb_len = 0;
	struct ieee80211_rx_stats stats = {
		.signal = 0,
		.noise = -98,
		.rate = 0,
		.freq = IEEE80211_24GHZ_BAND,
	};
	unsigned int count = priv->rxringcount;

	stats.nic_type = NIC_8192E;

	while (count--) {
		rx_desc *pdesc = &priv->rx_ring[priv->rx_idx];//rx descriptor
		struct sk_buff *skb = priv->rx_buf[priv->rx_idx];//rx pkt

		if (pdesc->OWN){
			/* wait data to be filled by hardware */
			return;
		} else {

			//RT_DEBUG_DATA(COMP_ERR, pdesc, sizeof(*pdesc));
			struct sk_buff *new_skb = NULL;
			if (!priv->ops->rx_query_status_descriptor(dev, &stats, pdesc, skb))
				goto done;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
			pci_dma_sync_single_for_cpu(priv->pdev,
						*((dma_addr_t *)skb->cb), 
						priv->rxbuffersize, 
						PCI_DMA_FROMDEVICE);
#else
			pci_unmap_single(priv->pdev,
					*((dma_addr_t *)skb->cb), 
					priv->rxbuffersize, 
					PCI_DMA_FROMDEVICE);
#endif

			skb_put(skb, pdesc->Length);
			skb_reserve(skb, stats.RxDrvInfoSize + stats.RxBufShift);
			skb_trim(skb, skb->len - 4/*sCrcLng*/);
			ieee80211_hdr = (struct ieee80211_hdr_1addr *)skb->data;
			if(is_broadcast_ether_addr(ieee80211_hdr->addr1)) {
				//TODO
			}else if(is_multicast_ether_addr(ieee80211_hdr->addr1)){
				//TODO
			}else {
				/* unicast packet */
				unicast_packet = true;
			}
			fc = le16_to_cpu(ieee80211_hdr->frame_ctl);
			type = WLAN_FC_GET_TYPE(fc);
			if(type == IEEE80211_FTYPE_MGMT)
			{
				bLedBlinking = false;
			}
			if(bLedBlinking)
				if(priv->ieee80211->LedControlHandler)
				priv->ieee80211->LedControlHandler(dev, LED_CTL_RX);
			skb_len = skb->len;
			if(!ieee80211_rx(priv->ieee80211, skb, &stats)){
				dev_kfree_skb_any(skb);
			} else {
				priv->stats.rxok++;
				if(unicast_packet) {
					priv->stats.rxbytesunicast += skb_len;
				}
			}
#if 1
			new_skb = dev_alloc_skb(priv->rxbuffersize);
			if (unlikely(!new_skb))
			{
				printk("==========>can't alloc skb for rx\n");
				goto done;
			}
			skb=new_skb;
                        skb->dev = dev;
#endif
			priv->rx_buf[priv->rx_idx] = skb;
			*((dma_addr_t *) skb->cb) = pci_map_single(priv->pdev, skb->tail, priv->rxbuffersize, PCI_DMA_FROMDEVICE);
			//                *((dma_addr_t *) skb->cb) = pci_map_single(priv->pdev, skb_tail_pointer(skb), priv->rxbuffersize, PCI_DMA_FROMDEVICE);

		}
done:
		pdesc->BufferAddress = cpu_to_le32(*((dma_addr_t *)skb->cb));
		pdesc->OWN = 1;
		pdesc->Length = priv->rxbuffersize;
		if (priv->rx_idx == priv->rxringcount-1)
			pdesc->EOR = 1;
		priv->rx_idx = (priv->rx_idx + 1) % priv->rxringcount;
	}

}



void rtl8192_tx_resume(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct ieee80211_device *ieee = priv->ieee80211;
	struct sk_buff *skb;
	int queue_index;

	for(queue_index = BK_QUEUE; queue_index < TXCMD_QUEUE;queue_index++) {
	//	printk("===>queue:%d, %d, %d\n", queue_index, skb_queue_empty(&ieee->skb_waitQ[queue_index]),  priv->ieee80211->check_nic_enough_desc(dev,queue_index));
		while((!skb_queue_empty(&ieee->skb_waitQ[queue_index]))&&
				(priv->ieee80211->check_nic_enough_desc(dev,queue_index) > 0)) {
			/* 1. dequeue the packet from the wait queue */	
			skb = skb_dequeue(&ieee->skb_waitQ[queue_index]);
			/* 2. tx the packet directly */	
			ieee->softmac_data_hard_start_xmit(skb,dev,0/* rate useless now*/);
			#if 0
			if(queue_index!=MGNT_QUEUE) {
				ieee->stats.tx_packets++;
				ieee->stats.tx_bytes += skb->len;
			}
			#endif
		}
	}
}

void rtl8192_irq_tx_tasklet(struct r8192_priv *priv)
{
       rtl8192_tx_resume(priv->ieee80211->dev);
}

void rtl8192_irq_rx_tasklet(struct r8192_priv *priv)
{
       rtl8192_rx(priv->ieee80211->dev);
	/* unmask RDU */
       write_nic_dword(priv->ieee80211->dev, INTA_MASK,read_nic_dword(priv->ieee80211->dev, INTA_MASK) | IMR_RDU); 
}

/****************************************************************************
 ---------------------------- NIC START/CLOSE STUFF---------------------------
*****************************************************************************/
/* detach all the work and timer structure declared or inititialized 
 * in r8192_init function. 
 * */
void rtl8192_cancel_deferred_work(struct r8192_priv* priv)
{
	/* call cancel_work_sync instead of cancel_delayed_work if and only if Linux_version_code 
         * is  or is newer than 2.6.20 and work structure is defined to be struct work_struct. 
         * Otherwise call cancel_delayed_work is enough. 
         * FIXME (2.6.20 shoud 2.6.22, work_struct shoud not cancel)	
         * */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	cancel_delayed_work(&priv->watch_dog_wq);
	cancel_delayed_work(&priv->update_beacon_wq);
#ifndef RTL8190P
//	cancel_delayed_work(&priv->ieee80211->hw_wakeup_wq);
	cancel_delayed_work(&priv->ieee80211->hw_sleep_wq);
	//cancel_delayed_work(&priv->gpio_change_rf_wq);//lzm 090323
#endif
#endif
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,22)
	cancel_work_sync(&priv->reset_wq);
	cancel_work_sync(&priv->qos_activate);
	//cancel_work_sync(&priv->SetBWModeWorkItem);
	//cancel_work_sync(&priv->SwChnlWorkItem);
#else
#if ((LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)))
	cancel_delayed_work(&priv->reset_wq);
	cancel_delayed_work(&priv->qos_activate);
	//cancel_delayed_work(&priv->SetBWModeWorkItem);
	//cancel_delayed_work(&priv->SwChnlWorkItem);
#endif
#endif

}

int _rtl8192_up(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	//int i;
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));
	bool init_status = true;
	priv->up=1;
	priv->ieee80211->ieee_up=1;	
	RT_TRACE(COMP_INIT, "Bringing up iface");
//	printk("======>%s()\n",__FUNCTION__);
	priv->bfirst_init = true;
	init_status = priv->ops->initialize_adapter(dev);
	if(init_status != true)
	{
		RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n",__FUNCTION__);
		return -1;
	}
	RT_TRACE(COMP_INIT, "start adapter finished\n");
	RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
	priv->bfirst_init = false;
	
#if 1	
#ifndef RTL8190P
	if(priv->ieee80211->eRFPowerState!=eRfOn)
		MgntActSet_RF_State(dev, eRfOn, priv->ieee80211->RfOffReason);	
#endif

#ifdef ENABLE_GPIO_RADIO_CTL

	if(priv->polling_timer_on == 0){//add for S3/S4
		check_rfctrl_gpio_timer((unsigned long)dev);
	}
	
#endif

	if(priv->ieee80211->state != IEEE80211_LINKED)
	    ieee80211_softmac_start_protocol(priv->ieee80211);
	    
	ieee80211_reset_queue(priv->ieee80211);
	watch_dog_timer_callback((unsigned long) dev);
	
	if(!netif_queue_stopped(dev))
		netif_start_queue(dev);
	else
		netif_wake_queue(dev);
#endif
	return 0;
}


int rtl8192_open(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);
	ret = rtl8192_up(dev);
	up(&priv->wx_sem);
	return ret;
	
}


int rtl8192_up(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	if (priv->up == 1) return -1;
	
	return _rtl8192_up(dev);
}


int rtl8192_close(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	int ret;
	
	down(&priv->wx_sem);

	ret = rtl8192_down(dev,true);
	
	up(&priv->wx_sem);
	
	return ret;

}

int rtl8192_down(struct net_device *dev, bool shutdownrf)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
//added by amy for ps 090319
	//PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));
//	int i;
#if 0
	u8	ucRegRead;	
	u32	ulRegRead;
#endif
	if (priv->up == 0) return -1;
	
	priv->up=0;
	priv->ieee80211->ieee_up = 0;
	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);
//	printk("==========>%s()\n", __FUNCTION__);
/* FIXME */
	if (!netif_queue_stopped(dev))
		netif_stop_queue(dev);

	//YJ,add for security,090323
	priv->ieee80211->wpa_ie_len = 0;
	if(priv->ieee80211->wpa_ie)
		kfree(priv->ieee80211->wpa_ie);
	priv->ieee80211->wpa_ie = NULL;
	CamResetAllEntry(dev);	
	
	rtl8192_irq_disable(dev);
#if 0	
	if(!priv->ieee80211->bSupportRemoteWakeUp) {
		MgntActSet_RF_State(dev, eRfOff, RF_CHANGE_BY_INIT);
		// 2006.11.30. System reset bit
		ulRegRead = read_nic_dword(dev, CPU_GEN);	
		ulRegRead|=CPU_GEN_SYSTEM_RESET;
		write_nic_dword(dev, CPU_GEN, ulRegRead);
	} else {
		//2008.06.03 for WOL
		write_nic_dword(dev, WFCRC0, 0xffffffff);
		write_nic_dword(dev, WFCRC1, 0xffffffff);
		write_nic_dword(dev, WFCRC2, 0xffffffff);
#ifdef RTL8190P
		//GPIO 0 = true
		ucRegRead = read_nic_byte(dev, GPO);
		ucRegRead |= BIT0;
		write_nic_byte(dev, GPO, ucRegRead);
#endif			
		//Write PMR register
		write_nic_byte(dev, PMR, 0x5);
		//Disable tx, enanble rx
		write_nic_byte(dev, MacBlkCtrl, 0xa);
	}
#endif
//	flush_scheduled_work();
	rtl8192_cancel_deferred_work(priv);
#ifndef RTL8190P
	cancel_delayed_work(&priv->ieee80211->hw_wakeup_wq);
#endif
	del_timer_sync(&priv->watch_dog_timer);	

	ieee80211_softmac_stop_protocol(priv->ieee80211,true);
#if 0
#ifdef ENABLE_IPS
	printk("==========>%s(): set rf off\n", __FUNCTION__);
	if(shutdownrf)
	MgntActSet_RF_State(dev, eRfOff, RF_CHANGE_BY_INIT);
#endif
#endif
	priv->ops->stop_adapter(dev, false);
//	RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
	udelay(100);
	memset(&priv->ieee80211->current_network, 0 , offsetof(struct ieee80211_network, list));
	//priv->isRFOff = false;
	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);

		return 0;
}


void rtl8192_commit(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	if (priv->up == 0) return ;


	ieee80211_softmac_stop_protocol(priv->ieee80211,true);
	
	rtl8192_irq_disable(dev);
	priv->ops->stop_adapter(dev, true);
	_rtl8192_up(dev);
}

/*
void rtl8192_restart(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
void rtl8192_restart(struct work_struct *work)
{
        struct r8192_priv *priv = container_of(work, struct r8192_priv, reset_wq);
        struct net_device *dev = priv->ieee80211->dev;
#else
void rtl8192_restart(struct net_device *dev)
{

        struct r8192_priv *priv = ieee80211_priv(dev);
#endif

	down(&priv->wx_sem);
	
	rtl8192_commit(dev);
	
	up(&priv->wx_sem);
}

static void r8192_set_multicast(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	short promisc;

	//down(&priv->wx_sem);
	
	/* FIXME FIXME */
	
	promisc = (dev->flags & IFF_PROMISC) ? 1:0;
	
	if (promisc != priv->promisc) {
		;
	//	rtl8192_commit(dev);
	}
	
	priv->promisc = promisc;
	
	//schedule_work(&priv->reset_wq);
	//up(&priv->wx_sem);
}


int r8192_set_mac_adr(struct net_device *dev, void *mac)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct sockaddr *addr = mac;
	
	down(&priv->wx_sem);
	
	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
		
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
	schedule_work(&priv->reset_wq);
#else
	schedule_task(&priv->reset_wq);
#endif	
	up(&priv->wx_sem);
	
	return 0;
}


/* based on ipw2200 driver */
int rtl8192_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	struct iwreq *wrq = (struct iwreq *)rq;
	int ret=-1;
	struct ieee80211_device *ieee = priv->ieee80211;
	u32 key[4];
	u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	u8 zero_addr[6] = {0};
	struct iw_point *p = &wrq->u.data;
	struct ieee_param *ipw = NULL;//(struct ieee_param *)wrq->u.data.pointer;

	down(&priv->wx_sem);


     if (p->length < sizeof(struct ieee_param) || !p->pointer){
             ret = -EINVAL;
             goto out;
     }

     ipw = (struct ieee_param *)kmalloc(p->length, GFP_KERNEL);
     if (ipw == NULL){
             ret = -ENOMEM;
             goto out;
     }
     if (copy_from_user(ipw, p->pointer, p->length)) {
            kfree(ipw);
            ret = -EFAULT;
            goto out;
     }

	switch (cmd) {
	    case RTL_IOCTL_WPA_SUPPLICANT:
		//parse here for HW security	
			if (ipw->cmd == IEEE_CMD_SET_ENCRYPTION)
			{
				if (ipw->u.crypt.set_tx)
				{
					if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
						ieee->pairwise_key_type = KEY_TYPE_CCMP;
					else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
						ieee->pairwise_key_type = KEY_TYPE_TKIP;
					else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
					{
						if (ipw->u.crypt.key_len == 13)
							ieee->pairwise_key_type = KEY_TYPE_WEP104;
						else if (ipw->u.crypt.key_len == 5)
							ieee->pairwise_key_type = KEY_TYPE_WEP40;
					}
					else
						ieee->pairwise_key_type = KEY_TYPE_NA;

					if (ieee->pairwise_key_type)
					{
  //      FIXME:these two lines below just to fix ipw interface bug, that is, it will never set mode down to driver. So treat it as ADHOC mode, if no association procedure. WB. 2009.02.04
                                                if (memcmp(ieee->ap_mac_addr, zero_addr, 6) == 0)
                                                        ieee->iw_mode = IW_MODE_ADHOC;

						memcpy((u8*)key, ipw->u.crypt.key, 16);
						EnableHWSecurityConfig8192(dev);
					//we fill both index entry and 4th entry for pairwise key as in IPW interface, adhoc will only get here, so we need index entry for its default key serching!
					//added by WB.
						setKey(dev, 4, ipw->u.crypt.idx, ieee->pairwise_key_type, (u8*)ieee->ap_mac_addr, 0, key);
						if (ieee->iw_mode == IW_MODE_ADHOC)  //LEAP WEP will never set this.
						setKey(dev, ipw->u.crypt.idx, ipw->u.crypt.idx, ieee->pairwise_key_type, (u8*)ieee->ap_mac_addr, 0, key);
					}
					#ifdef RTL8192E
					if ((ieee->pairwise_key_type == KEY_TYPE_CCMP) && ieee->pHTInfo->bCurrentHTSupport){
							write_nic_byte(dev, 0x173, 1); //fix aes bug
						}
					#endif
						
				}
				else //if (ipw->u.crypt.idx) //group key use idx > 0
				{
					memcpy((u8*)key, ipw->u.crypt.key, 16);
					if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
						ieee->group_key_type= KEY_TYPE_CCMP;
					else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
						ieee->group_key_type = KEY_TYPE_TKIP;
					else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
					{
						if (ipw->u.crypt.key_len == 13)
							ieee->group_key_type = KEY_TYPE_WEP104;
						else if (ipw->u.crypt.key_len == 5)
							ieee->group_key_type = KEY_TYPE_WEP40;
					}
					else
						ieee->group_key_type = KEY_TYPE_NA;

					if (ieee->group_key_type)
					{
							setKey(	dev, 
								ipw->u.crypt.idx,
								ipw->u.crypt.idx,		//KeyIndex 
						     		ieee->group_key_type,	//KeyType
						            	broadcast_addr,	//MacAddr
								0,		//DefaultKey
							      	key);		//KeyContent
					}
				}
			}
#ifdef JOHN_DEBUG
		//john's test 0711
	{
		int i;
		printk("@@ wrq->u pointer = ");
		for(i=0;i<wrq->u.data.length;i++){
			if(i%10==0) printk("\n");
			printk( "%8x|", ((u32*)wrq->u.data.pointer)[i] );
		}
		printk("\n");
	}
#endif /*JOHN_DEBUG*/
		ret = ieee80211_wpa_supplicant_ioctl(priv->ieee80211, &wrq->u.data);
		break; 

	    default:
		ret = -EOPNOTSUPP;
		break;
	}

	kfree(ipw);
out:
	up(&priv->wx_sem);
	
	return ret;
}
//warning message WB
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
void rtl8192_interrupt(int irq, void *netdev, struct pt_regs *regs)
#else
irqreturn_t rtl8192_interrupt(int irq, void *netdev, struct pt_regs *regs)
#endif
#else
irqreturn_t rtl8192_interrupt(int irq, void *netdev)
#endif
{
    struct net_device *dev = (struct net_device *) netdev;
    struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
    unsigned long flags;
    u32 inta, intb;
    /* We should return IRQ_NONE, but for now let me keep this */
    if(priv->irq_enabled == 0){
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
        return;
#else
        return IRQ_HANDLED;
#endif
    }

    spin_lock_irqsave(&priv->irq_th_lock,flags);

    //ISR: 4bytes

    inta = read_nic_dword(dev, ISR) & priv->irq_mask;
    write_nic_dword(dev,ISR,inta); // reset int situation
    intb = read_nic_dword(dev, ISR+4);
    write_nic_dword(dev, ISR+4, intb);
    priv->stats.shints++;
   // printk("Enter interrupt, ISR value = 0x%08x\n", inta);	
    if(!inta){
        spin_unlock_irqrestore(&priv->irq_th_lock,flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
        return;
#else
        return IRQ_HANDLED;  
#endif
        /* 
           most probably we can safely return IRQ_NONE,
           but for now is better to avoid problems
           */
    }

    if(inta == 0xffff){
        /* HW disappared */
        spin_unlock_irqrestore(&priv->irq_th_lock,flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
        return;
#else
        return IRQ_HANDLED;  
#endif
    }

    priv->stats.ints++;
#ifdef DEBUG_IRQ
    DMESG("NIC irq %x",inta);
#endif
    //priv->irqpending = inta;


    if(!netif_running(dev)) {
        spin_unlock_irqrestore(&priv->irq_th_lock,flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
        return;
#else
        return IRQ_HANDLED;
#endif
    }
#if 0
    if(inta & IMR_TIMEOUT0){
        //		write_nic_dword(dev, TimerInt, 0);
        //DMESG("=================>waking up");
        //		rtl8180_hw_wakeup(dev);
    }	

#endif
    if(intb & IMR_TBDOK){
        RT_TRACE(COMP_INTR, "beacon ok interrupt!\n");
        rtl8192_tx_isr(dev, BEACON_QUEUE);
        priv->stats.txbeaconokint++;
    }

    if(intb & IMR_TBDER){
        RT_TRACE(COMP_INTR, "beacon ok interrupt!\n");
        rtl8192_tx_isr(dev, BEACON_QUEUE);
        priv->stats.txbeaconerr++;
    }

    if(inta  & IMR_MGNTDOK ) {
        RT_TRACE(COMP_INTR, "Manage ok interrupt!\n");
   //     printk("Manage ok interrupt!\n");
        priv->stats.txmanageokint++;
        rtl8192_tx_isr(dev,MGNT_QUEUE);

    }

    if(inta & IMR_COMDOK)
    {
        priv->stats.txcmdpktokint++;
        rtl8192_tx_isr(dev,TXCMD_QUEUE);
    }

    if(inta & IMR_ROK){
#ifdef DEBUG_RX
        DMESG("Frame arrived !");
#endif
        priv->stats.rxint++;
        tasklet_schedule(&priv->irq_rx_tasklet);
    }

    if(inta & IMR_BcnInt) {
        RT_TRACE(COMP_INTR, "prepare beacon for interrupt!\n");
        tasklet_schedule(&priv->irq_prepare_beacon_tasklet);
    }

    if(inta & IMR_RDU){
        RT_TRACE(COMP_INTR, "rx descriptor unavailable!\n");
        priv->stats.rxrdu++;
        /* reset int situation */
        write_nic_dword(dev,INTA_MASK,read_nic_dword(dev, INTA_MASK) & ~IMR_RDU); 
        tasklet_schedule(&priv->irq_rx_tasklet);
    }

    if(inta & IMR_RXFOVW){
        RT_TRACE(COMP_INTR, "rx overflow !\n");
        priv->stats.rxoverflow++;
        tasklet_schedule(&priv->irq_rx_tasklet);
    }

    if(inta & IMR_TXFOVW) priv->stats.txoverflow++;

    if(inta & IMR_BKDOK){ 
        RT_TRACE(COMP_INTR, "BK Tx OK interrupt!\n");
        priv->stats.txbkokint++;
        priv->ieee80211->LinkDetectInfo.NumTxOkInPeriod++;
        rtl8192_tx_isr(dev,BK_QUEUE);
    //    rtl8192_try_wake_queue(dev, BK_QUEUE);
    }

    if(inta & IMR_BEDOK){ 
        RT_TRACE(COMP_INTR, "BE TX OK interrupt!\n");
        priv->stats.txbeokint++;
        priv->ieee80211->LinkDetectInfo.NumTxOkInPeriod++;
        rtl8192_tx_isr(dev,BE_QUEUE);
   //     rtl8192_try_wake_queue(dev, BE_QUEUE);
    }

    if(inta & IMR_VIDOK){ 
        RT_TRACE(COMP_INTR, "VI TX OK interrupt!\n");
        priv->stats.txviokint++;
        priv->ieee80211->LinkDetectInfo.NumTxOkInPeriod++;
        rtl8192_tx_isr(dev,VI_QUEUE);
    //    rtl8192_try_wake_queue(dev, VI_QUEUE);
    }

    if(inta & IMR_VODOK){ 
        priv->stats.txvookint++;
        RT_TRACE(COMP_INTR, "Vo TX OK interrupt!\n");
        priv->ieee80211->LinkDetectInfo.NumTxOkInPeriod++;
        rtl8192_tx_isr(dev,VO_QUEUE);
  //      rtl8192_try_wake_queue(dev, VO_QUEUE);
    }

    force_pci_posting(dev);
    spin_unlock_irqrestore(&priv->irq_th_lock,flags);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
    return;
#else
    return IRQ_HANDLED;
#endif
}

#if 0
void rtl8192_try_wake_queue(struct net_device *dev, int pri)
{
	
	unsigned long flags;
	short enough_desc;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	spin_lock_irqsave(&priv->tx_lock,flags);
	enough_desc = check_nic_enough_desc(dev,pri);
        spin_unlock_irqrestore(&priv->tx_lock,flags);	
	
	if(enough_desc)
		ieee80211_wake_queue(priv->ieee80211);

}
#endif	
/****************************************************************************
     ---------------------------- PCI_STUFF---------------------------
*****************************************************************************/

static int __devinit rtl8192_pci_probe(struct pci_dev *pdev,
			 const struct pci_device_id *id)
{
	unsigned long ioaddr = 0;
	struct net_device *dev = NULL;
	struct r8192_priv *priv= NULL;
	//u8 unit = 0;
	
#ifdef CONFIG_RTL8192_IO_MAP
	unsigned long pio_start, pio_len, pio_flags;
#else
	unsigned long pmem_start, pmem_len, pmem_flags;
#endif //end #ifdef RTL_IO_MAP

	RT_TRACE(COMP_INIT,"Configuring chip resources");
	
	if( pci_enable_device (pdev) ){
		RT_TRACE(COMP_ERR,"Failed to enable PCI device");
		return -EIO;
	}

	pci_set_master(pdev);
	//pci_set_wmi(pdev);
	pci_set_dma_mask(pdev, 0xffffff00ULL);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	pci_set_consistent_dma_mask(pdev,0xffffff00ULL);
#endif	
	dev = alloc_ieee80211(sizeof(struct r8192_priv));
	if (!dev)
		return -ENOMEM;
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	SET_MODULE_OWNER(dev);
#endif

	pci_set_drvdata(pdev, dev);	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	SET_NETDEV_DEV(dev, &pdev->dev);
#endif
	priv = ieee80211_priv(dev);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	priv->ieee80211 = netdev_priv(dev);
#else
	priv->ieee80211 = (struct ieee80211_device *)dev->priv;
#endif
	priv->pdev=pdev;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	if((pdev->subsystem_vendor == PCI_VENDOR_ID_DLINK)&&(pdev->subsystem_device == 0x3304)){
		priv->ieee80211->bSupportRemoteWakeUp = 1;
	} else 
#endif
	{
		priv->ieee80211->bSupportRemoteWakeUp = 0;
	}

#ifdef CONFIG_RTL8192_IO_MAP
	
	pio_start = (unsigned long)pci_resource_start (pdev, 0);
	pio_len = (unsigned long)pci_resource_len (pdev, 0);
	pio_flags = (unsigned long)pci_resource_flags (pdev, 0);
 	
      	if (!(pio_flags & IORESOURCE_IO)) {
		RT_TRACE(COMP_ERR,"region #0 not a PIO resource, aborting");
		goto fail;
	}
	
	//DMESG("IO space @ 0x%08lx", pio_start );
	if( ! request_region( pio_start, pio_len, RTL819xE_MODULE_NAME ) ){
		RT_TRACE(COMP_ERR,"request_region failed!");
		goto fail;
	}
	
	ioaddr = pio_start;
	dev->base_addr = ioaddr; // device I/O address
	
#else
	
	pmem_start = pci_resource_start(pdev, 1);
	pmem_len = pci_resource_len(pdev, 1);
	pmem_flags = pci_resource_flags (pdev, 1);
	
	if (!(pmem_flags & IORESOURCE_MEM)) {
		RT_TRACE(COMP_ERR,"region #1 not a MMIO resource, aborting");
		goto fail;
	}
	
	//DMESG("Memory mapped space @ 0x%08lx ", pmem_start);
	if( ! request_mem_region(pmem_start, pmem_len, RTL819xE_MODULE_NAME)) {
		RT_TRACE(COMP_ERR,"request_mem_region failed!");
		goto fail;
	}
	
	
	ioaddr = (unsigned long)ioremap_nocache( pmem_start, pmem_len);	
	if( ioaddr == (unsigned long)NULL ){
		RT_TRACE(COMP_ERR,"ioremap failed!");
	       // release_mem_region( pmem_start, pmem_len );
		goto fail1;
	}
	
	dev->mem_start = ioaddr; // shared mem start
	dev->mem_end = ioaddr + pci_resource_len(pdev, 0); // shared mem end
	
#endif //end #ifdef RTL_IO_MAP

#if 0 //need to check with HW
        /* We disable the RETRY_TIMEOUT register (0x41) to keep
         * PCI Tx retries from interfering with C3 CPU state */
         pci_write_config_byte(pdev, 0x41, 0x00);


	pci_read_config_byte(pdev, 0x05, &unit);
	pci_write_config_byte(pdev, 0x05, unit & (~0x04));
#endif

#ifdef RTL8190P
	rtl819xp_ops.nic_type = priv->card_8192 = NIC_8190P;
	priv->ops = &rtl819xp_ops;
#else
	{
		u8 tmp = 0;
		pci_read_config_byte(pdev, 0x8, 	&tmp);
#ifdef RTL8192E
		if (tmp == HAL_HW_PCI_REVISION_ID_8192PCIE){
			printk("===============>NIC 8192E\n");
			rtl819xp_ops.nic_type = priv->card_8192 = NIC_8192E;
			priv->ops = &rtl819xp_ops;
		}			
		else
#else //8192SE
		if (tmp == HAL_HW_PCI_REVISION_ID_8192SE){
			printk("===============>NIC 8192SE\n");
			priv->card_8192 = NIC_8192SE;
			priv->ops = &rtl8192se_ops;
		}
		else
#endif
		{
			RT_TRACE(COMP_ERR, "UNKNOWN nic type(%4x:%4x)\n", pdev->vendor, pdev->device);
			priv->card_8192 = NIC_UNKNOWN;
			goto fail1;
		}
		
	}
#endif
	dev->irq = pdev->irq;
	priv->irq = 0;
	
	dev->open = rtl8192_open;
	dev->stop = rtl8192_close;
	//dev->hard_start_xmit = rtl8192_8023_hard_start_xmit;
	dev->tx_timeout = tx_timeout;
	//dev->wireless_handlers = &r8192_wx_handlers_def;
	dev->do_ioctl = rtl8192_ioctl;
	dev->set_multicast_list = r8192_set_multicast;
	dev->set_mac_address = r8192_set_mac_adr;
	
         //DMESG("Oops: i'm coming\n");
#if WIRELESS_EXT >= 12
#if WIRELESS_EXT < 17
        dev->get_wireless_stats = r8192_get_wireless_stats;
#endif
        dev->wireless_handlers = (struct iw_handler_def *) &r8192_wx_handlers_def;
#endif
       //dev->get_wireless_stats = r8192_get_wireless_stats;
	dev->type=ARPHRD_ETHER;

	dev->watchdog_timeo = HZ*3;	//modified by john, 0805

	if (dev_alloc_name(dev, ifname) < 0){
                RT_TRACE(COMP_INIT, "Oops: devname already taken! Trying wlan%%d...\n");
		ifname = "wlan%d";
		dev_alloc_name(dev, ifname);
        }
	
	RT_TRACE(COMP_INIT, "Driver probe completed1\n");
	if(rtl8192_init(dev)!=0){ 
		RT_TRACE(COMP_ERR, "Initialization failed");
		goto fail1;
	}
	
	netif_carrier_off(dev);
	netif_stop_queue(dev);
	
	register_netdev(dev);
	RT_TRACE(COMP_INIT, "dev name=======> %s\n",dev->name);
	rtl8192_proc_init_one(dev);
	
#ifdef ENABLE_GPIO_RADIO_CTL
	if(priv->polling_timer_on == 0){//add for S3/S4
		check_rfctrl_gpio_timer((unsigned long)dev);
	}
#endif
	
	RT_TRACE(COMP_INIT, "Driver probe completed\n");
//#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
//	return dev;
//#else
	return 0;	
//#endif

fail1:
	
#ifdef CONFIG_RTL8180_IO_MAP
		
	if( dev->base_addr != 0 ){
			
		release_region(dev->base_addr, 
	       pci_resource_len(pdev, 0) );
	}
#else
	if( dev->mem_start != (unsigned long)NULL ){
		iounmap( (void *)dev->mem_start );
		release_mem_region( pci_resource_start(pdev, 1), 
				    pci_resource_len(pdev, 1) );
	}
#endif //end #ifdef RTL_IO_MAP
	
fail:
	if(dev){
		
		if (priv->irq) {
			free_irq(dev->irq, dev);
			dev->irq=0;
		}
		free_ieee80211(dev);
	}
	
	pci_disable_device(pdev);
	
	DMESG("wlan driver load failed\n");
	pci_set_drvdata(pdev, NULL);
	return -ENODEV;
	
}

static void __devexit rtl8192_pci_disconnect(struct pci_dev *pdev)
{
	struct net_device *dev = pci_get_drvdata(pdev);
	struct r8192_priv *priv ;

 	if(dev){
		
		unregister_netdev(dev);
		
		priv=ieee80211_priv(dev);
		
#ifdef ENABLE_GPIO_RADIO_CTL
		del_timer_sync(&priv->gpio_polling_timer);
		cancel_delayed_work(&priv->gpio_change_rf_wq);
		priv->polling_timer_on = 0;//add for S3/S4
#endif
		
		rtl8192_proc_remove_one(dev);
		
		rtl8192_down(dev,true);
		deinit_hal_dm(dev);
		//added by amy for LED 090313
#ifdef RTL8192SE
		DeInitSwLeds(dev);
#endif
		if (priv->pFirmware)
		{
			vfree(priv->pFirmware);
			priv->pFirmware = NULL;
		}
	//	priv->rf_close(dev);
	//	rtl8192_usb_deleteendpoints(dev);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
		destroy_workqueue(priv->priv_wq);
#endif
                /* redundant with rtl8192_down */
               // rtl8192_irq_disable(dev);
               // rtl8192_reset(dev);
               // mdelay(10);
                {
                    u32 i;
                    /* free tx/rx rings */
                    rtl8192_free_rx_ring(dev);
                    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++) {
                        rtl8192_free_tx_ring(dev, i);
                    }
                }
		if(priv->irq){
			
			printk("Freeing irq %d\n",dev->irq);
			free_irq(dev->irq, dev);
			priv->irq=0;
			
		}
                


	//	free_beacon_desc_ring(dev,priv->txbeaconcount);
		
#ifdef CONFIG_RTL8180_IO_MAP
		
		if( dev->base_addr != 0 ){
			
			release_region(dev->base_addr, 
				       pci_resource_len(pdev, 0) );
		}
#else
		if( dev->mem_start != (unsigned long)NULL ){
			iounmap( (void *)dev->mem_start );
			release_mem_region( pci_resource_start(pdev, 1), 
					    pci_resource_len(pdev, 1) );
		}
#endif /*end #ifdef RTL_IO_MAP*/
		free_ieee80211(dev);

	}

	pci_disable_device(pdev);
	RT_TRACE(COMP_DOWN, "wlan driver removed\n");
}


static int __init rtl8192_pci_module_init(void)
{
	printk(KERN_INFO "\nLinux kernel driver for RTL8192 based WLAN cards\n");
	printk(KERN_INFO "Copyright (c) 2007-2008, Realsil Wlan\n");
	RT_TRACE(COMP_INIT, "Initializing module");
	RT_TRACE(COMP_INIT, "Wireless extensions version %d", WIRELESS_EXT);
	rtl8192_proc_module_init();
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
      if(0!=pci_module_init(&rtl8192_pci_driver))
#else
      if(0!=pci_register_driver(&rtl8192_pci_driver))
#endif
	{
		DMESG("No device found");
		/*pci_unregister_driver (&rtl8192_pci_driver);*/
		return -ENODEV;
	}
	return 0;
}


static void __exit rtl8192_pci_module_exit(void)
{
	pci_unregister_driver(&rtl8192_pci_driver);

	RT_TRACE(COMP_DOWN, "Exiting");
	rtl8192_proc_module_remove();
}


/***************************************************************************
     ------------------- HW security related ----------------
****************************************************************************/

void CamResetAllEntry(struct net_device *dev)
{
	//u8 ucIndex;
	u32 ulcommand = 0;

#if 1
	ulcommand |= BIT31|BIT30;
	write_nic_dword(dev, RWCAM, ulcommand); 
#else
        for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
                CAM_mark_invalid(dev, ucIndex);
        for(ucIndex=0;ucIndex<TOTAL_CAM_ENTRY;ucIndex++)
                CAM_empty_entry(dev, ucIndex);
#endif
}


void write_cam(struct net_device *dev, u8 addr, u32 data)
{
        write_nic_dword(dev, WCAMI, data);
        write_nic_dword(dev, RWCAM, BIT31|BIT16|(addr&0xff) );
}
u32 read_cam(struct net_device *dev, u8 addr)
{
        write_nic_dword(dev, RWCAM, 0x80000000|(addr&0xff) );
        return read_nic_dword(dev, 0xa8);
}

void EnableHWSecurityConfig8192(struct net_device *dev)
{
        u8 SECR_value = 0x0;
	// struct ieee80211_device* ieee1 = container_of(&dev, struct ieee80211_device, dev);
	 //printk("==>ieee1:%p, dev:%p\n", ieee1, dev); 
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	 struct ieee80211_device* ieee = priv->ieee80211; 
	 //printk("==>ieee:%p, dev:%p\n", ieee, dev); 
	SECR_value = SCR_TxEncEnable | SCR_RxDecEnable;
#if 1
	if (((KEY_TYPE_WEP40 == ieee->pairwise_key_type) || (KEY_TYPE_WEP104 == ieee->pairwise_key_type)) && (priv->ieee80211->auth_mode != 2))
	{
		SECR_value |= SCR_RxUseDK;
		SECR_value |= SCR_TxUseDK;
	}
	else if ((ieee->iw_mode == IW_MODE_ADHOC) && (ieee->pairwise_key_type & (KEY_TYPE_CCMP | KEY_TYPE_TKIP)))
	{
		SECR_value |= SCR_RxUseDK;
		SECR_value |= SCR_TxUseDK;
	}

#endif

        //add HWSec active enable here.
//default using hwsec. when peer AP is in N mode only and pairwise_key_type is none_aes(which HT_IOT_ACT_PURE_N_MODE indicates it), use software security. when peer AP is in b,g,n mode mixed and pairwise_key_type is none_aes, use g mode hw security. WB on 2008.7.4
	ieee->hwsec_active = 1;
	
	if ((ieee->pHTInfo->IOTAction&HT_IOT_ACT_PURE_N_MODE) || !hwwep)//!ieee->hwsec_support) //add hwsec_support flag to totol control hw_sec on/off
	{
		ieee->hwsec_active = 0;
		SECR_value &= ~SCR_RxDecEnable;
	}
	
	RT_TRACE(COMP_SEC,"%s:, hwsec:%d, pairwise_key:%d, SECR_value:%x\n", __FUNCTION__, \
			ieee->hwsec_active, ieee->pairwise_key_type, SECR_value);	
	{
                write_nic_byte(dev, SECR,  SECR_value);//SECR_value |  SCR_UseDK ); 
        }

}
void setKey(	struct net_device *dev, 
		u8 EntryNo,
		u8 KeyIndex, 
		u16 KeyType, 
		u8 *MacAddr, 
		u8 DefaultKey, 
		u32 *KeyContent )
{
	u32 TargetCommand = 0;
	u32 TargetContent = 0;
	u16 usConfig = 0;
	u8 i;
#ifdef ENABLE_IPS
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	RT_RF_POWER_STATE	rtState;
	rtState = priv->ieee80211->eRFPowerState;
	if(priv->ieee80211->PowerSaveControl.bInactivePs){ 
		if(rtState == eRfOff){
			if(priv->ieee80211->RfOffReason > RF_CHANGE_BY_IPS)
			{
				RT_TRACE(COMP_ERR, "%s(): RF is OFF.\n",__FUNCTION__);
				up(&priv->wx_sem);
				return ;
			}
			else{
				down(&priv->ieee80211->ips_sem);
				IPSLeave(dev);
				up(&priv->ieee80211->ips_sem);			}
		}
	}
	priv->ieee80211->is_set_key = true;
#endif
	if (EntryNo >= TOTAL_CAM_ENTRY)
		RT_TRACE(COMP_ERR, "cam entry exceeds in setKey()\n");

	RT_TRACE(COMP_SEC, "====>to setKey(), dev:%p, EntryNo:%d, KeyIndex:%d, KeyType:%d, MacAddr"MAC_FMT"\n", dev,EntryNo, KeyIndex, KeyType, MAC_ARG(MacAddr));
	
	if (DefaultKey)
		usConfig |= BIT15 | (KeyType<<2);
	else
		usConfig |= BIT15 | (KeyType<<2) | KeyIndex;
//	usConfig |= BIT15 | (KeyType<<2) | (DefaultKey<<5) | KeyIndex;


	for(i=0 ; i<CAM_CONTENT_COUNT; i++){
		TargetCommand  = i+CAM_CONTENT_COUNT*EntryNo;
		TargetCommand |= BIT31|BIT16;

		if(i==0){//MAC|Config
			TargetContent = (u32)(*(MacAddr+0)) << 16|
					(u32)(*(MacAddr+1)) << 24|
					(u32)usConfig;

			write_nic_dword(dev, WCAMI, TargetContent); 
			write_nic_dword(dev, RWCAM, TargetCommand);
	//		printk("setkey cam =%8x\n", read_cam(dev, i+6*EntryNo));
		}
		else if(i==1){//MAC
                        TargetContent = (u32)(*(MacAddr+2)) 	 |
                                        (u32)(*(MacAddr+3)) <<  8|
                                        (u32)(*(MacAddr+4)) << 16|
                                        (u32)(*(MacAddr+5)) << 24;
			write_nic_dword(dev, WCAMI, TargetContent); 
			write_nic_dword(dev, RWCAM, TargetCommand);
		}
		else {	//Key Material
			if(KeyContent != NULL)
			{
			write_nic_dword(dev, WCAMI, (u32)(*(KeyContent+i-2)) ); 
			write_nic_dword(dev, RWCAM, TargetCommand);
		}
	}
	}
	RT_TRACE(COMP_SEC,"=========>after set key, usconfig:%x\n", usConfig);
//	CAM_read_entry(dev, 0);
}
// This function seems not ready! WB 
#if 0
void CamPrintDbgReg(struct net_device* dev)
{
	unsigned long rvalue;
	unsigned char ucValue;
	write_nic_dword(dev, DCAM, 0x80000000);
	msleep(40);
	rvalue = read_nic_dword(dev, DCAM);	//delay_ms(40);
	RT_TRACE(COMP_SEC, " TX CAM=%8lX ",rvalue);
	if((rvalue & 0x40000000) != 0x4000000) 
		RT_TRACE(COMP_SEC, "-->TX Key Not Found      ");
	msleep(20);
	write_nic_dword(dev, DCAM, 0x00000000);	//delay_ms(40);
	rvalue = read_nic_dword(dev, DCAM);	//delay_ms(40);
	RT_TRACE(COMP_SEC, "RX CAM=%8lX ",rvalue);
	if((rvalue & 0x40000000) != 0x4000000) 
		RT_TRACE(COMP_SEC, "-->CAM Key Not Found   ");
	ucValue = read_nic_byte(dev, SECR);
	RT_TRACE(COMP_SEC, "WPA_Config=%x \n",ucValue);
}
#endif
void CAM_read_entry(
	struct net_device *dev,
	u32	 		iIndex 
)
{
 	u32 target_command=0;
	 u32 target_content=0;
	 u8 entry_i=0;
	 u32 ulStatus;
	s32 i=100;
//	printk("=======>start read CAM\n"); 
 	for(entry_i=0;entry_i<CAM_CONTENT_COUNT;entry_i++)
 	{
   	// polling bit, and No Write enable, and address
		target_command= entry_i+CAM_CONTENT_COUNT*iIndex;
		target_command= target_command | BIT31;

	//Check polling bit is clear
//	mdelay(1);
#if 1
		while((i--)>=0)
		{
			ulStatus = read_nic_dword(dev, RWCAM);
			if(ulStatus & BIT31){
				continue;
			}
			else{
				break;
			}
		}
#endif
  		write_nic_dword(dev, RWCAM, target_command);
   	 	RT_TRACE(COMP_SEC,"CAM_read_entry(): WRITE A0: %x \n",target_command);
   	 //	printk("CAM_read_entry(): WRITE A0: %lx \n",target_command);
  	 	target_content = read_nic_dword(dev, RCAMO);
  	 	RT_TRACE(COMP_SEC, "CAM_read_entry(): WRITE A8: %x \n",target_content);
  	 //	printk("CAM_read_entry(): WRITE A8: %lx \n",target_content);
 	}
	printk("\n");
}

void CamRestoreAllEntry(	struct net_device *dev)
{
	u8 EntryId = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8*	MacAddr = priv->ieee80211->current_network.bssid;
	
	static u8	CAM_CONST_ADDR[4][6] = {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x03}};
	static u8	CAM_CONST_BROAD[] = 
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
	RT_TRACE(COMP_SEC, "CamRestoreAllEntry: \n");

		
	if ((priv->ieee80211->pairwise_key_type == KEY_TYPE_WEP40)||
	    (priv->ieee80211->pairwise_key_type == KEY_TYPE_WEP104))
	{
		
		for(EntryId=0; EntryId<4; EntryId++)
		{
			{
				MacAddr = CAM_CONST_ADDR[EntryId];
				setKey(dev, 
						EntryId ,
						EntryId,
						priv->ieee80211->pairwise_key_type,
						MacAddr, 
						0,
						NULL);			
			}	
		}	
		
	}
	else if(priv->ieee80211->pairwise_key_type == KEY_TYPE_TKIP)
	{
		
		{
			if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type, 
						(u8*)dev->dev_addr, 
						0,
						NULL);		
			else
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type, 
						MacAddr, 
						0,
						NULL);			
		}
	}
	else if(priv->ieee80211->pairwise_key_type == KEY_TYPE_CCMP)
	{

		{
			if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type,
						(u8*)dev->dev_addr, 
						0,
						NULL);		
			else
				setKey(dev, 
						4,
						0,
						priv->ieee80211->pairwise_key_type,
						MacAddr, 
						0,
						NULL);				
		}
	}



	if(priv->ieee80211->group_key_type == KEY_TYPE_TKIP)
	{
		MacAddr = CAM_CONST_BROAD;
		for(EntryId=1 ; EntryId<4 ; EntryId++)
		{
			{
				setKey(dev, 
						EntryId, 
						EntryId,
						priv->ieee80211->group_key_type,
						MacAddr, 
						0,
						NULL);			
			}	
		}
		if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						0,
						0,
						priv->ieee80211->group_key_type,
						CAM_CONST_ADDR[0], 
						0,
						NULL);		
	}
	else if(priv->ieee80211->group_key_type == KEY_TYPE_CCMP)
	{
		MacAddr = CAM_CONST_BROAD;
		for(EntryId=1; EntryId<4 ; EntryId++)
		{
			{	
				setKey(dev, 
						EntryId , 
						EntryId,
						priv->ieee80211->group_key_type,
						MacAddr, 
						0,
						NULL);			
			}	
		}

		if(priv->ieee80211->iw_mode == IW_MODE_ADHOC)
				setKey(dev, 
						0 , 
						0,
						priv->ieee80211->group_key_type,
						CAM_CONST_ADDR[0], 
						0,
						NULL);
	}
}
/***************************************************************************
     ------------------- HW RELATED STUFF----------------
****************************************************************************/
#ifdef RTL8192SE
//#define  rtl8192se_init_priv_variable rtl8192_init_priv_variable 

//Config hw with initial value. 
static void rtl8192se_config_hw_for_load_fail(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u16			i;//,usValue;		
	u8 			sMacAddr[6] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
	u8			rf_path, index;

	RT_TRACE(COMP_INIT, "====> rtl8192se_config_hw_for_load_fail\n");

	write_nic_byte(dev, SYS_ISO_CTRL+1, 0xE8); // Isolation signals from Loader
	mdelay(10);
	write_nic_byte(dev, PMC_FSM, 0x02); // Enable Loader Data Keep
	
	// Initialize IC Version && Channel Plan
	priv->eeprom_vid= 0;
	priv->eeprom_did= 0;		
	priv->eeprom_ChannelPlan= 0;
	priv->eeprom_CustomerID= 0;	

       //
	//<Roger_Notes> In this case, we random assigh MAC address here. 2008.10.15.
	//
	//Initialize Permanent MAC address
	//if(!dev->bInHctTest)
       get_random_bytes(&sMacAddr[5], 1);
	for(i = 0; i < 6; i++)
		dev->dev_addr[i] = sMacAddr[i];			


	// 2007/11/15 MH For RTL8192USB we assign as 1T2R now.
	priv->rf_type= RTL819X_DEFAULT_RF_TYPE;	// default : 1T2R
	priv->rf_chip = RF_6052;

	//
	// For TX power index 
	//
#if (EEPROM_OLD_FORMAT_SUPPORT == 1)
	for(i=0; i<14; i++)
	{
		priv->EEPROMTxPowerLevelCCK[i] = (u8)(EEPROM_Default_TxPower & 0xff);
		RT_TRACE(COMP_INIT, "CCK 2.4G Tx Pwr Index %d = 0x%02x\n", 
		i, priv->EEPROMTxPowerLevelCCK[i]);
	}			

	for(i=0; i<14; i++)
	{
		priv->EEPROMTxPowerLevelOFDM24G[i] = (u8)(EEPROM_Default_TxPower & 0xff);
		RT_TRACE(COMP_INIT, "OFDM 2.4G Tx Pwr Index %d = 0x%02x\n", 
		i, priv->EEPROMTxPowerLevelOFDM24G[i]);				
	}	

	//
	// Update HAL variables.
	//
	for(i=0; i<14; i++)
	{			
		priv->TxPowerLevelOFDM24G[i] = priv->EEPROMTxPowerLevelOFDM24G[i];
		priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelCCK[i];
	}	
	 // Default setting for Empty EEPROM				
	for(i=0; i<6; i++)
	{
		priv->EEPROMHT2T_TxPwr[i] = EEPROM_Default_HT2T_TxPwr;
	}
	
#else
	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			// Read CCK RF A & B Tx power 
			priv->RfCckChnlAreaTxPwr[rf_path][i] = 
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][i] = 
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][i] = 
			EEPROM_Default_TxPowerLevel;
		}
	}	
				
	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for(i=0; i<14; i++)	// channel 1~3 use the same Tx Power Level.
		{
			if (i < 3)			// Cjanel 1-3
				index = 0;
			else if (i < 9)		// Channel 4-9
				index = 1;
			else				// Channel 10-14
				index = 2;

			// Record A & B CCK /OFDM - 1T/2T Channel area tx power
			priv->RfTxPwrLevelCck[rf_path][i]  = 
			priv->RfCckChnlAreaTxPwr[rf_path][index] = 
			priv->RfTxPwrLevelOfdm1T[rf_path][i]  = 
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][index] = 
			priv->RfTxPwrLevelOfdm2T[rf_path][i]  = 
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][index] = 
			(u8)(EEPROM_Default_TxPower & 0xff);				

			if (rf_path == 0)
			{
				priv->TxPowerLevelOFDM24G[i] = (u8)(EEPROM_Default_TxPower & 0xff);
				priv->TxPowerLevelCCK[i] = (u8)(EEPROM_Default_TxPower & 0xff);
			}
		}
		
	}
	
	RT_TRACE(COMP_INIT, "All TxPwr = 0x%x\n", EEPROM_Default_TxPower);

	// 2009/01/21 MH Support new EEPROM format from SD3 requirement for verification.
	for(i=0; i<14; i++)	
	{			
		// Set HT 20 <->40MHZ tx power diff	
		priv->TxPwrHt20Diff[RF90_PATH_A][i] = DEFAULT_HT20_TXPWR_DIFF;
		priv->TxPwrHt20Diff[RF90_PATH_B][i] = DEFAULT_HT20_TXPWR_DIFF;

		priv->TxPwrLegacyHtDiff[0][i] = EEPROM_Default_LegacyHTTxPowerDiff;
		priv->TxPwrLegacyHtDiff[1][i] = EEPROM_Default_LegacyHTTxPowerDiff;
	}
	
	priv->TxPwrSafetyFlag = 0;
#endif
	
	priv->EEPROMTxPowerDiff = EEPROM_Default_LegacyHTTxPowerDiff;
	priv->LegacyHTTxPowerDiff = priv->EEPROMTxPowerDiff;
	RT_TRACE(COMP_INIT, "TxPowerDiff = %#x\n", priv->EEPROMTxPowerDiff);

#if 0
	// Antenna A legacy and HT TX power difference
	priv->AntennaTxPwDiff[0] = (priv->EEPROMTxPowerDiff & 0xf);
	// Antenna A legacy and HT TX power difference
	priv->AntennaTxPwDiff[1] = ((priv->EEPROMTxPowerDiff & 0xf0)>>4);

	// CrystalCap, bit12~15
	priv->CrystalCap = priv->EEPROMCrystalCap;
	// ThermalMeter, bit0~3 for RFIC1, bit4~7 for RFIC2
	// 92U does not enable TX power tracking.
	priv->ThermalMeter[0] = priv->EEPROMThermalMeter;
#endif
	//
	//
	//
	priv->EEPROMTSSI_A = EEPROM_Default_TSSI;
	priv->EEPROMTSSI_B = EEPROM_Default_TSSI;			

	for(i=0; i<6; i++)
	{
		priv->EEPROMHT2T_TxPwr[i] = EEPROM_Default_HT2T_TxPwr;
	}

	
	// Initialize Difference of gain index between legacy and HT OFDM from EEPROM.
	// Initialize ThermalMeter from EEPROM
	priv->EEPROMThermalMeter = EEPROM_Default_ThermalMeter;
	// ThermalMeter, bit0~3 for RFIC1, bit4~7 for RFIC2
	priv->ThermalMeter[0] = (priv->EEPROMThermalMeter&0x1f);	//[4:0]
	priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;

	priv->BluetoothCoexist = EEPROM_Default_BlueToothCoexist;
	
	// Initialize CrystalCap from EEPROM		
	priv->EEPROMCrystalCap = EEPROM_Default_CrystalCap;
	// CrystalCap, bit12~15
	priv->CrystalCap = priv->EEPROMCrystalCap;

	//
	// Read EEPROM Version && Channel Plan
	//
	// Default channel plan  =0
	priv->eeprom_ChannelPlan = 0;
	priv->eeprom_version = 1;		// Default version is 1
	priv->bTXPowerDataReadFromEEPORM = false;

	// 2007/11/15 MH For RTL8192S we assign as 1T2R now.
	priv->rf_type = RTL819X_DEFAULT_RF_TYPE;	// default : 1T2R		
	priv->rf_chip = RF_6052;
	priv->eeprom_CustomerID = 0;
	RT_TRACE(COMP_INIT, "EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);

			
	priv->EEPROMBoardType = EEPROM_Default_BoardType;	
	RT_TRACE(COMP_INIT, "BoardType = %#x\n", priv->EEPROMBoardType);

	priv->LedStrategy = SW_LED_MODE0;

//	InitRateAdaptive(dev);
	
	RT_TRACE(COMP_INIT,"<==== rtl8192se_config_hw_for_load_fail\n");
}

static	void rtl8192se_read_eeprom_info(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u16			i,usValue;
	u16			EEPROMId;
#if (EEPROM_OLD_FORMAT_SUPPORT == 1)
	u8			tmpBuffer[30];	
#endif
	u8			tempval;
	u8			hwinfo[HWSET_MAX_SIZE_92S];
	u8			rf_path, index;	// For EEPROM/EFUSE After V0.6_1117

	RT_TRACE(COMP_INIT, "====> rtl8192se_read_eeprom_info\n");

	if (priv->epromtype== EEPROM_93C46)
	{	// Read frin EEPROM
		write_nic_byte(dev, SYS_ISO_CTRL+1, 0xE8); // Isolation signals from Loader
		mdelay(10);
		write_nic_byte(dev, PMC_FSM, 0x02); // Enable Loader Data Keep

		RT_TRACE(COMP_INIT, "EEPROM\n");
		// Read all Content from EEPROM or EFUSE!!!
		for(i = 0; i < HWSET_MAX_SIZE_92S; i += 2)
		{
			usValue = eprom_read(dev, (u16) (i>>1));
			*((u16*)(&hwinfo[i])) = usValue;					
		}	
	}
	else if (priv->epromtype == EEPROM_BOOT_EFUSE)
	{	// Read from EFUSE	
		RT_TRACE(COMP_INIT, "EFUSE\n");

		// Read EFUSE real map to shadow!!
		EFUSE_ShadowMapUpdate(dev);

		memcpy( hwinfo, &priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE_92S);		
	}

	// Print current HW setting map!!
	//RT_DEBUG_DATA(COMP_INIT, hwinfo, HWSET_MAX_SIZE_92S);			

	// Checl 0x8129 again for making sure autoload status!!
	EEPROMId = *((u16 *)&hwinfo[0]);
	if( EEPROMId != RTL8190_EEPROM_ID )
	{
		RT_TRACE(COMP_ERR, "EEPROM ID(%#x) is invalid!!\n", EEPROMId); 
		priv->AutoloadFailFlag=true;
	}
	else
	{
		RT_TRACE(COMP_EPROM, "Autoload OK\n"); 
		priv->AutoloadFailFlag=false;
	}	

  	//
	if (priv->AutoloadFailFlag == TRUE)
	{
		rtl8192se_config_hw_for_load_fail(dev);
		return;
	}
	
	// Read IC Version && Channel Plan
	// VID, DID	 SE	0xA-D
	priv->eeprom_vid		= *(u16 *)&hwinfo[EEPROM_VID];
	priv->eeprom_did         = *(u16 *)&hwinfo[EEPROM_DID];
	priv->eeprom_svid		= *(u16 *)&hwinfo[EEPROM_SVID];
	priv->eeprom_smid		= *(u16 *)&hwinfo[EEPROM_SMID];

	RT_TRACE(COMP_EPROM, "EEPROMId = 0x%4x\n", EEPROMId);
	RT_TRACE(COMP_EPROM, "EEPROM VID = 0x%4x\n", priv->eeprom_vid);
	RT_TRACE(COMP_EPROM, "EEPROM DID = 0x%4x\n", priv->eeprom_did);
	RT_TRACE(COMP_EPROM, "EEPROM SVID = 0x%4x\n", priv->eeprom_svid);
	RT_TRACE(COMP_EPROM, "EEPROM SMID = 0x%4x\n", priv->eeprom_smid);

      //Read Permanent MAC address
	for(i = 0; i < 6; i += 2)
	{
		usValue = *(u16 *)&hwinfo[EEPROM_MAC_ADDR+i];
		*((u16*)(&dev->dev_addr[i])) = usValue;
	}
	for (i=0;i<6;i++)
		write_nic_byte(dev, MACIDR0+i, dev->dev_addr[i]); 	

	RT_TRACE(COMP_EPROM, "ReadAdapterInfo8192S(), Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
	dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2], dev->dev_addr[3], 
	dev->dev_addr[4], dev->dev_addr[5]); 	

#if (EEPROM_OLD_FORMAT_SUPPORT == 1)
	//
	// Buffer TxPwIdx(i.e., from offset 0x50~0x6D, total 28Bytes)
	// Update CCK, OFDM Tx Power Index from above buffer.
	//
	for(i = 0; i < EEPROM_TX_PWR_INDEX_RANGE; i += 2)
	{
		usValue = *(u16 *)&hwinfo[EEPROM_TxPowerBase+i];
		*((u16 *)(&tmpBuffer[i])) = usValue;					
	}
	for(i=0; i<14; i++)
	{
		priv->EEPROMTxPowerLevelCCK[i] = (u8)tmpBuffer[i];
		priv->EEPROMTxPowerLevelOFDM24G[i] = (u8)tmpBuffer[i+14];
	}
		

	for(i=0; i<14; i++)
	{
		priv->TxPowerLevelOFDM24G[i] = priv->EEPROMTxPowerLevelOFDM24G[i];
		priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelCCK[i];
		
		RT_TRACE(COMP_EPROM, "CH%d CCK Tx PWR IDX = 0x%02x\n", i, priv->EEPROMTxPowerLevelCCK[i]);
		RT_TRACE(COMP_EPROM, "CH%d OFDM Tx PWR IDX = 0x%02x\n", i, priv->EEPROMTxPowerLevelOFDM24G[i]);
	}

#if 0
	// Read RF-indication and Tx Power gain index diff of legacy to HT OFDM rate.
	tempval = (*(u8 *)&hwinfo[EEPROM_RFInd_PowerDiff])&0xff;
	priv->EEPROMTxPowerDiff = tempval;	

	//pHalData->LegacyHTTxPowerDiff = pHalData->EEPROMLegacyHTTxPowerDiff;
	priv->LegacyHTTxPowerDiff = priv->EEPROMTxPowerDiff;
	// Antenna B gain offset to antenna A, bit0~3
	priv->AntennaTxPwDiff[0] = (priv->EEPROMTxPowerDiff & 0xf);
	// Antenna C gain offset to antenna A, bit4~7
	priv->AntennaTxPwDiff[1] = ((priv->EEPROMTxPowerDiff & 0xf0)>>4);
	priv->TxPowerDiff = priv->EEPROMTxPowerDiff;
	RT_TRACE(COMP_EPROM, "TxPowerDiff = %#x\n", priv->EEPROMTxPowerDiff);
#endif
	//
	// Get HT 2T Path A and B Power Index.
	//
	for(i = 0; i < 6; i += 2)
	{
		usValue = *(u16 *)&hwinfo[EEPROM_HT2T_CH1_A+i];
		*((u16*)(&priv->EEPROMHT2T_TxPwr[i])) = usValue;
	}		
	for(i=0; i<6; i++)
	{
		RT_TRACE(COMP_EPROM, "EEPROMHT2T_TxPwr, Index %d = 0x%02x\n", i, priv->EEPROMHT2T_TxPwr[i]);					
	}
	
#else	// For 93C56 like EEPROM or EFUSe TX power index definition
	//
	// Buffer TxPwIdx(i.e., from offset 0x50~0x61, total 18Bytes)
	// Update CCK, OFDM (1T/2T)Tx Power Index from above buffer.
	//

	//
	// Get Tx Power Level by Channel
	//
	// Read Tx power of Channel 1 ~ 14 from EEPROM.
	// 92S suupport RF A & B
	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			// Read CCK RF A & B Tx power 
			priv->RfCckChnlAreaTxPwr[rf_path][i] = 
			hwinfo[EEPROM_TxPowerBase+rf_path*3+i];

			// Read OFDM RF A & B Tx power for 1T
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][i] = 
			hwinfo[EEPROM_TxPowerBase+6+rf_path*3+i];

			// Read OFDM RF A & B Tx power for 2T
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][i] = 
			hwinfo[EEPROM_TxPowerBase+12+rf_path*3+i];
		}
	}

	for (rf_path = 0; rf_path < 2; rf_path++)
	{
		for (i = 0; i < 3; i++)
		{
			RT_TRACE(COMP_EPROM,"CCK RF-%d CHan_Area-%d = 0x%x\n",  rf_path, i,
			priv->RfCckChnlAreaTxPwr[rf_path][i]);
			RT_TRACE(COMP_EPROM, "OFDM-1T RF-%d CHan_Area-%d = 0x%x\n",  rf_path, i,
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][i]);
			RT_TRACE(COMP_EPROM,"OFDM-2T RF-%d CHan_Area-%d = 0x%x\n",  rf_path, i,
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][i]);			
		}

		// Assign dedicated channel tx power
		for(i=0; i<14; i++)	// channel 1~3 use the same Tx Power Level.
		{
			if (i < 3)			// Chanel 1-3
				index = 0;
			else if (i < 9)		// Channel 4-9
				index = 1;
			else				// Channel 10-14
				index = 2;

			// Record A & B CCK /OFDM - 1T/2T Channel area tx power
			priv->RfTxPwrLevelCck[rf_path][i]  = 
			priv->RfCckChnlAreaTxPwr[rf_path][index];
			priv->RfTxPwrLevelOfdm1T[rf_path][i]  = 
			priv->RfOfdmChnlAreaTxPwr1T[rf_path][index];
			priv->RfTxPwrLevelOfdm2T[rf_path][i]  = 
			priv->RfOfdmChnlAreaTxPwr2T[rf_path][index];

			if (rf_path == 0)
			{
				priv->TxPowerLevelOFDM24G[i] = priv->RfTxPwrLevelOfdm1T[rf_path][i] ;
				priv->TxPowerLevelCCK[i] = priv->RfTxPwrLevelCck[rf_path][i];					
                    }
		}

		for(i=0; i<14; i++)
		{
			RT_TRACE(COMP_EPROM, "Rf-%d TxPwr CH-%d CCK OFDM_1T OFDM_2T= 0x%x/0x%x/0x%x\n", 
			rf_path, i, priv->RfTxPwrLevelCck[rf_path][i], 
			priv->RfTxPwrLevelOfdm1T[rf_path][i] ,
			priv->RfTxPwrLevelOfdm2T[rf_path][i] );
		}
	}	
// 2009/01/20 MH add for new EEPROM format
	//	
	for(i=0; i<14; i++)	// channel 1~3 use the same Tx Power Level.
	{
		// Read tx power difference between HT OFDM 20/40 MHZ
		if (i < 3)			// Cjanel 1-3
			index = 0;
		else if (i < 9)		// Channel 4-9
			index = 1;
		else				// Channel 10-14
			index = 2;
		
		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_HT20_DIFF+index])&0xff;
		priv->TxPwrHt20Diff[RF90_PATH_A][i] = (tempval&0xF);
		priv->TxPwrHt20Diff[RF90_PATH_B][i] = ((tempval>>4)&0xF);

		// Read OFDM<->HT tx power diff
		if (i < 3)			// Cjanel 1-3
			index = 0;
		else if (i < 9)		// Channel 4-9
			index = 0x11;
		else				// Channel 10-14
			index = 1;

		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_OFDM_DIFF+index])&0xff;
		priv->TxPwrLegacyHtDiff[RF90_PATH_A][i] = (tempval&0xF);
		priv->TxPwrLegacyHtDiff[RF90_PATH_B][i] = ((tempval>>4)&0xF);
#if 0
		//
		// Read Band Edge tx power offset and check if user enable the ability
		//
		// HT 40 band edge channel
		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_BAND_EDGE])&0xff;
		priv->TxPwrbandEdgeHt40[RF90_PATH_A][0] = (tempval&0xF); 		// Band edge low channel
		priv->TxPwrbandEdgeHt40[RF90_PATH_A][1] =  ((tempval>>4)&0xF);	// Band edge high channel
		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_BAND_EDGE+1])&0xff;
		priv->TxPwrbandEdgeHt40[RF90_PATH_B][0] = (tempval&0xF); 		// Band edge low channel
		priv->TxPwrbandEdgeHt40[RF90_PATH_B][1] =  ((tempval>>4)&0xF);	// Band edge high channel
		// HT 20 band edge channel
		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_BAND_EDGE+2])&0xff;
		priv->TxPwrbandEdgeHt20[RF90_PATH_A][0] = (tempval&0xF); 		// Band edge low channel
		priv->TxPwrbandEdgeHt20[RF90_PATH_A][1] =  ((tempval>>4)&0xF);	// Band edge high channel
		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_BAND_EDGE+3])&0xff;
		priv->TxPwrbandEdgeHt20[RF90_PATH_B][0] = (tempval&0xF); 		// Band edge low channel
		priv->TxPwrbandEdgeHt20[RF90_PATH_B][1] =  ((tempval>>4)&0xF);	// Band edge high channel
		// OFDM band edge channel
		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_BAND_EDGE+4])&0xff;
		priv->TxPwrbandEdgeLegacyOfdm[RF90_PATH_A][0] = (tempval&0xF); 		// Band edge low channel
		priv->TxPwrbandEdgeLegacyOfdm[RF90_PATH_A][1] =  ((tempval>>4)&0xF);	// Band edge high channel
		tempval = (*(u8 *)&hwinfo[EEPROM_TX_PWR_BAND_EDGE+5])&0xff;
		priv->TxPwrbandEdgeLegacyOfdm[RF90_PATH_B][0] = (tempval&0xF); 		// Band edge low channel
		priv->TxPwrbandEdgeLegacyOfdm[RF90_PATH_B][1] =  ((tempval>>4)&0xF);	// Band edge high channel
#endif
		tempval = (*(u8 *)&hwinfo[TX_PWR_SAFETY_CHK]);
		priv->TxPwrSafetyFlag = (tempval&0x01);		
	}
#if 0
	// Read RF-indication and Tx Power gain index diff of legacy to HT OFDM rate.
	tempval = (*(u8 *)&hwinfo[EEPROM_RFInd_PowerDiff])&0xff;
	priv->EEPROMTxPowerDiff = tempval;	
	priv->LegacyHTTxPowerDiff = priv->EEPROMTxPowerDiff;
	priv->TxPowerDiff = priv->EEPROMTxPowerDiff;
	// Antenna A legacy and HT TX power difference
	priv->AntennaTxPwDiff[0] = (priv->EEPROMTxPowerDiff & 0xf);
	// Antenna B legacy and HT TX power difference
	priv->AntennaTxPwDiff[1] = ((priv->EEPROMTxPowerDiff & 0xf0)>>4);	
#endif
	for(i=0; i<14; i++)
		RT_TRACE(COMP_EPROM, "RF-A Ht20 to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrHt20Diff[RF90_PATH_A][i]);
	for(i=0; i<14; i++)
		RT_TRACE(COMP_EPROM, "RF-A Legacy to Ht40 Diff[%d] = 0x%x\n", i, priv->TxPwrLegacyHtDiff[RF90_PATH_A][i]);
	for(i=0; i<14; i++)
		RT_TRACE(COMP_EPROM, "RF-B Ht20 to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrHt20Diff[RF90_PATH_B][i]);
	for(i=0; i<14; i++)
		RT_TRACE(COMP_EPROM, "RF-B Legacy to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrLegacyHtDiff[RF90_PATH_B][i]);
#endif	// #if (EEPROM_OLD_FORMAT_SUPPORT == 1)
	RT_TRACE(COMP_EPROM, "TxPwrSafetyFlag = %d\n", priv->TxPwrSafetyFlag);

	//
	// 
	//
	// Read RF-indication and Tx Power gain index diff of legacy to HT OFDM rate.	
	tempval = (*(u8 *)&hwinfo[EEPROM_RFInd_PowerDiff])&0xff;
	priv->EEPROMTxPowerDiff = tempval;	
	priv->LegacyHTTxPowerDiff = priv->EEPROMTxPowerDiff;

	RT_TRACE(COMP_EPROM, "TxPowerDiff = %#x\n", priv->EEPROMTxPowerDiff);

	//
	// Get TSSI value for each path.
	//
	usValue = *(u16 *)&hwinfo[EEPROM_TSSI_A];
	priv->EEPROMTSSI_A = (u8)((usValue&0xff00)>>8);
	usValue = *(u8 *)&hwinfo[EEPROM_TSSI_B];
	priv->EEPROMTSSI_B = (u8)(usValue&0xff);
	RT_TRACE(COMP_EPROM, "TSSI_A = %#x, TSSI_B = %#x\n", 
			priv->EEPROMTSSI_A, priv->EEPROMTSSI_B);
		
	//
	// Read antenna tx power offset of B/C/D to A  from EEPROM
	// and read ThermalMeter from EEPROM
	//
	tempval = *(u8 *)&hwinfo[EEPROM_ThermalMeter];
	priv->EEPROMThermalMeter = tempval;			
	RT_TRACE(COMP_EPROM, "ThermalMeter = %#x\n", priv->EEPROMThermalMeter);
	// ThermalMeter, bit0~3 for RFIC1, bit4~7 for RFIC2
	priv->ThermalMeter[0] =(priv->EEPROMThermalMeter&0x1f);
	//pHalData->ThermalMeter[1] = ((pHalData->EEPROMThermalMeter & 0xf0)>>4);
	priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;
	
	// Bluetooth coexistance
	tempval = *(u8 *)&hwinfo[EEPROM_BLUETOOTH_COEXIST];
	priv->EEPROMBluetoothCoexist = ((tempval&0x2)>>1);	// 92se, 0x78[1]
	priv->BluetoothCoexist = priv->EEPROMBluetoothCoexist;
	RT_TRACE(COMP_EPROM, "BlueTooth Coexistance = %#x\n", priv->BluetoothCoexist);

	//
	// Read CrystalCap from EEPROM
	//
	tempval = (*(u8 *)&hwinfo[EEPROM_CrystalCap])>>4;
	priv->EEPROMCrystalCap =tempval;		
	RT_TRACE(COMP_EPROM, "CrystalCap = %#x\n", priv->EEPROMCrystalCap);
	// CrystalCap, bit12~15
	priv->CrystalCap = priv->EEPROMCrystalCap;	
	
	//
	// Read IC Version && Channel Plan
	//
	// Version ID, Channel plan
	priv->eeprom_ChannelPlan = *(u8 *)&hwinfo[EEPROM_ChannelPlan];
	priv->eeprom_version = *(u16 *)&hwinfo[EEPROM_Version];
	priv->bTXPowerDataReadFromEEPORM = true;
	RT_TRACE(COMP_EPROM, "EEPROM ChannelPlan = 0x%4x\n", priv->eeprom_ChannelPlan);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("EEPROM Version ID: 0x%2x\n", pHalData->VersionID));
	
	//
	// Read Customer ID or Board Type!!!
	//
	tempval = *(u8*)&hwinfo[EEPROM_BoardType];  
	// 2008/11/14 MH RTL8192SE_PCIE_EEPROM_SPEC_V0.6_20081111.doc
	// 2008/12/09 MH RTL8192SE_PCIE_EEPROM_SPEC_V0.7_20081201.doc
	// Change RF type definition
	if (tempval == 0)	// 2x2           RTL8192SE (QFN68)
		priv->rf_type= RF_2T2R;
	else if (tempval == 1)	 // 1x2           RTL8191RE (QFN68)
		priv->rf_type = RF_1T2R;
	else if (tempval == 2)	 //  1x2		RTL8191SE (QFN64) Crab
		priv->rf_type = RF_1T2R;
	else if (tempval == 3)	 //  1x1 		RTL8191SE (QFN64) Unicorn
		priv->rf_type = RF_1T1R;

	priv->rf_chip = RF_6052;

	priv->eeprom_CustomerID = *(u8 *)&hwinfo[EEPROM_CustomID];

	RT_TRACE(COMP_EPROM, "EEPROM Customer ID: 0x%2x, rf_chip:%x\n", priv->eeprom_CustomerID, priv->rf_chip);
	
	priv->LedStrategy = SW_LED_MODE0;

	// 2008/01/16 MH We can only know RF type in the function. So we have to init
	// DIG RATR table again.
	//FIXME:YJ,090317
	//InitRateAdaptive(dev);
	
	RT_TRACE(COMP_INIT, "<==== rtl8192se_read_eeprom_info\n");
}

void rtl8192se_get_eeprom_size(struct net_device* dev)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	u8 curCR = 0;
	curCR = read_nic_byte(dev, EPROM_CMD);
	// For debug test now!!!!!
	PHY_RFShadowRefresh(dev);
	if (curCR & BIT4){
		RT_TRACE(COMP_EPROM, "Boot from EEPROM\n");
		priv->epromtype = EEPROM_93C46;
	}
	else{
		RT_TRACE(COMP_EPROM, "Boot from EFUSE\n");
		priv->epromtype = EEPROM_BOOT_EFUSE;
	}
	if (curCR & BIT5){
		RT_TRACE(COMP_EPROM,"Autoload OK\n"); 
		priv->AutoloadFailFlag=false;		
		rtl8192se_read_eeprom_info(dev);
	}
	else{// Auto load fail.		
		RT_TRACE(COMP_INIT, "AutoLoad Fail reported from CR9346!!\n"); 
		priv->AutoloadFailFlag=true;		
		rtl8192se_config_hw_for_load_fail(dev);		

		if (priv->epromtype == EEPROM_BOOT_EFUSE)
		{
#if (RTL92SE_FPGA_VERIFY == 0)
			EFUSE_ShadowMapUpdate(dev);
#endif
		}
	}			
#ifdef TO_DO_LIST
	if(Adapter->bInHctTest)
	{
		pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
		pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
		pMgntInfo->PowerSaveControl.bLeisurePs = FALSE;
		pMgntInfo->keepAliveLevel = 0;
	}
	
	switch(pHalData->eeprom_CustomerID)
	{
		case EEPROM_CID_DEFAULT:
			pMgntInfo->CustomerID = RT_CID_DEFAULT;
			break;
		case EEPROM_CID_TOSHIBA:       
			pMgntInfo->CustomerID = RT_CID_TOSHIBA;
			break;
		default:
			// value from RegCustomerID
			break;
	}
	
	if(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0xE020)
		pMgntInfo->CustomerID = RT_CID_819x_Lenovo;
	
	if((pMgntInfo->RegChannelPlan >= RT_CHANNEL_DOMAIN_MAX) || (pHalData->EEPROMChannelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK))
	{
		pMgntInfo->ChannelPlan = HalMapChannelPlan8192S(Adapter, (pHalData->EEPROMChannelPlan & (~(EEPROM_CHANNEL_PLAN_BY_HW_MASK))));
		pMgntInfo->bChnlPlanFromHW = (pHalData->EEPROMChannelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK) ? TRUE : FALSE; // User cannot change  channel plan.
	}
	else
	{
		pMgntInfo->ChannelPlan = (RT_CHANNEL_DOMAIN)pMgntInfo->RegChannelPlan;
	}

	switch(pMgntInfo->ChannelPlan)
	{
		case RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN:
		{
			PRT_DOT11D_INFO	pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);
	
			pDot11dInfo->bEnabled = TRUE;
		}
		RT_TRACE(COMP_INIT, DBG_LOUD, ("ReadAdapterInfo8187(): Enable dot11d when RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN!\n"));
		break;
	}
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("RegChannelPlan(%d) EEPROMChannelPlan(%d)", pMgntInfo->RegChannelPlan, pHalData->EEPROMChannelPlan));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("ChannelPlan = %d\n" , pMgntInfo->ChannelPlan));

	HalCustomizedBehavior8192S(Adapter);
#endif
	priv->LedStrategy = SW_LED_MODE7;
}

//
// Description:
//	Set the MAC offset [0x09] and prevent all I/O for a while (about 20us~200us, suggested from SD4 Scott).
//	If the protection is not performed well or the value is not set complete, the next I/O will cause the system hang.
// Note:
//	This procudure is designed specifically for 8192S and references the platform based variables
//	which violates the stucture of multi-platform.
//	Thus, we shall not extend this procedure to common handler.
// By Bruce, 2009-01-08.
//
u8
HalSetSysClk8192SE(
	struct net_device *dev,
	u8 Data
	)
{
//#if PLATFORM != PLATFORM_WINDOWS
#if 0
	write_nic_byte(dev, (SYS_CLKR + 1), Data);
	udelay(200);;
	return 1;
#else
	{
		//PMP_ADAPTER			pDevice = &(Adapter->NdisAdapter);
		//struct r8192_priv* priv = ieee80211_priv(dev);
		u8				WaitCount = 100;
		bool bResult = false;

#ifdef TO_DO_LIST
		RT_DISABLE_FUNC(Adapter, DF_IO_BIT);

		do
		{
			// Wait until all I/Os are complete.
			if(pDevice->IOCount == 0)
				break;
			delay_us(10);
		}while(WaitCount -- > 0);

		if(WaitCount == 0)
		{ // Wait too long, give up this procedure.
			RT_ENABLE_FUNC(Adapter, DF_IO_BIT);
			RT_TRACE(COMP_POWER, DBG_WARNING, ("HalSetSysClk8192SE(): Wait too long! Skip ....\n"));
			return FALSE;
		}
		#endif
		write_nic_byte(dev,SYS_CLKR + 1,Data);

		// Wait the MAC synchronized.
		udelay(400);


		// Check if it is set ready.
		{
			u8 TmpValue;
			TmpValue=read_nic_byte(dev,SYS_CLKR + 1);
			bResult = ((TmpValue&BIT7)== (Data & BIT7));
			//DbgPrint("write 38 and return value %x\n",TmpValue);
			if((Data &(BIT6|BIT7)) == false)
			{			
				WaitCount = 100;
				TmpValue = 0;
				while(1) 
				{
					WaitCount--;
					TmpValue=read_nic_byte(dev, SYS_CLKR + 1); 
					if((TmpValue &BIT6))
						break;
					printk("wait for BIT6 return value %x\n",TmpValue);	
					if(WaitCount==0)
						break;
					udelay(10);
				}
				if(WaitCount == 0)
					bResult = false;
				else
					bResult = true;
			}
		}
#ifdef TO_DO_LIST
		// should be wait for BIT6 to 1 Neo Test // BIT7 0 BIT6 1
		RT_ENABLE_FUNC(Adapter, DF_IO_BIT);
#endif
		RT_TRACE(COMP_PS,"HalSetSysClk8192SE():Value = %02X, return: %d\n", Data, bResult);
		return bResult;
	}
#endif
}

#if (RTL92SE_FPGA_VERIFY == 1)
/*-----------------------------------------------------------------------------
 * Function:	MacConfigBeforeFwDownload()
 *
 * Overview:	Copy from WMAc for init setting. MAC config before FW download
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	04/29/2008	MHC		The same as 92SE MAC initialization.  
 *	07/11/2008	MHC		MAC config before FW download
 *
 *---------------------------------------------------------------------------*/
static void MacConfigBeforeFwDownload(struct net_device* dev)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	u8				i;
	u8				tmpU1b;
	u16				tmpU2b;
	u32				addr;
	
	// 2008/05/14 MH For 92SE rest before init MAC data sheet not defined now. 
	// HW provide the method to prevent press reset bbutton every time.
	RT_TRACE(COMP_INIT, "Set some register before enable NIC\r\n");

	tmpU1b = read_nic_byte(dev, 0x562);
	tmpU1b |= 0x08;
	write_nic_byte(dev, 0x562, tmpU1b);
	tmpU1b &= ~(BIT3);
	write_nic_byte(dev, 0x562, tmpU1b);
	
	tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);
	tmpU1b &= 0x73;
	write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b);

	tmpU1b = read_nic_byte(dev, SYS_CLKR);	
	tmpU1b &= 0xfa;
	write_nic_byte(dev, SYS_CLKR, tmpU1b);

	RT_TRACE(COMP_INIT, "Delay 1000ms before reset NIC. I dont know how long should we delay!!!!!\r\n");	
	ssleep(1);

	// Switch to 80M clock
	write_nic_byte(dev, SYS_CLKR, SYS_CLKSEL_80M);
	
	 // Enable VSPS12 LDO Macro block
	tmpU1b = read_nic_byte(dev, SPS1_CTRL);	
	write_nic_byte(dev, SPS1_CTRL, (tmpU1b|SPS1_LDEN));
	
	//Enable AFE Macro Block's Bandgap
	tmpU1b = read_nic_byte(dev, AFE_MISC);	
	write_nic_byte(dev, AFE_MISC, (tmpU1b|AFE_BGEN));

	//Enable LDOA15 block
	tmpU1b = read_nic_byte(dev, LDOA15_CTRL);	
	write_nic_byte(dev, LDOA15_CTRL, (tmpU1b|LDA15_EN));

	 //Enable VSPS12_SW Macro Block
	tmpU1b = read_nic_byte(dev, SPS1_CTRL);	
	write_nic_byte(dev, SPS1_CTRL, (tmpU1b|SPS1_SWEN));

	//Enable AFE Macro Block's Mbias
	tmpU1b = read_nic_byte(dev, AFE_MISC);	
	write_nic_byte(dev, AFE_MISC, (tmpU1b|AFE_MBEN));

	// Set Digital Vdd to Retention isolation Path.
	tmpU2b = read_nic_word(dev, SYS_ISO_CTRL);	
	write_nic_word(dev, SYS_ISO_CTRL, (tmpU2b|ISO_PWC_DV2RP));

	// Attatch AFE PLL to MACTOP/BB/PCIe Digital
	tmpU2b = read_nic_word(dev, SYS_ISO_CTRL);	
	write_nic_word(dev, SYS_ISO_CTRL, (tmpU2b &(~ISO_PWC_RV2RP)));

	//Enable AFE clock
	tmpU2b = read_nic_word(dev, AFE_XTAL_CTRL);	
	write_nic_word(dev, AFE_XTAL_CTRL, (tmpU2b &(~XTAL_GATE_AFE)));

	//Enable AFE PLL Macro Block
	tmpU1b = read_nic_byte(dev, AFE_PLL_CTRL);	
	write_nic_byte(dev, AFE_PLL_CTRL, (tmpU1b|APLL_EN));

	// Release isolation AFE PLL & MD
	write_nic_byte(dev, SYS_ISO_CTRL, 0xEE);

	//Enable MAC clock
	tmpU2b = read_nic_word(dev, SYS_CLKR);	
	write_nic_word(dev, SYS_CLKR, (tmpU2b|SYS_MAC_CLK_EN));

	//Enable Core digital and enable IOREG R/W
	tmpU2b = read_nic_word(dev, SYS_FUNC_EN);	
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b|FEN_DCORE|FEN_MREGEN));

	 //Switch the control path.
	tmpU2b = read_nic_word(dev, SYS_CLKR);	
	write_nic_word(dev, SYS_CLKR, ((tmpU2b|SYS_FWHW_SEL)&(~SYS_SWHW_SEL)));

	// Reset RF temporarily!!!!
	write_nic_byte(dev, RF_CTRL, 0);
	write_nic_byte(dev, RF_CTRL, 7);

	write_nic_word(dev, CMDR, 0x37FC);	
	
#if 1
	// Load MAC register from WMAc temporarily We simulate macreg.txt HW will provide 
	// MAC txt later 
	write_nic_byte(dev, 0x6, 0x30);
	//write_nic_byte(dev, 0x48, 0x2f);
	write_nic_byte(dev, 0x49, 0xf0);

	write_nic_byte(dev, 0x4b, 0x81);

	write_nic_byte(dev, 0xb5, 0x21);

	write_nic_byte(dev, 0xdc, 0xff);
	write_nic_byte(dev, 0xdd, 0xff);	
	write_nic_byte(dev, 0xde, 0xff);
	write_nic_byte(dev, 0xdf, 0xff);

	write_nic_byte(dev, 0x11a, 0x00);
	write_nic_byte(dev, 0x11b, 0x00);

	for (i = 0; i < 32; i++)
		write_nic_byte(dev, INIMCS_SEL+i, 0x1b);	

	write_nic_byte(dev, 0x236, 0xff);
	
	write_nic_byte(dev, 0x503, 0x22);
	//write_nic_byte(dev, 0x560, 0x09);
	write_nic_byte(dev, 0x560, 0x79);

	write_nic_byte(dev, DBG_PORT, 0x91);	
#endif

	//Set RX Desc Address
#if TODO_LIST //rx cmd packet? 	
	write_nic_dword(dev, RCDA, 
	pHalData->RxDescMemory[RX_CMD_QUEUE].PhysicalAddressLow);
#endif
	write_nic_dword(dev, RDQDA, priv->rx_ring_dma);
	rtl8192_tx_enable(dev);
	
	RT_TRACE(COMP_INIT, "<---MacConfig8192SE()\n");

}	/* MacConfigBeforeFwDownload */
#else
/*-----------------------------------------------------------------------------
 * Function:	gen_RefreshLedState()
 *
 * Overview:	When we call the function, media status is no link. It must be in SW/HW
 *			radio off. Or IPS state. If IPS no link we will turn on LED, otherwise, we must turn off.
 *			After MAC IO reset, we must write LED control 0x2f2 again.
 *
 * Input:		IN	PADAPTER			Adapter)
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	03/27/2009	MHC		Create for LED judge only~!!   
 *
 *---------------------------------------------------------------------------*/
void gen_RefreshLedState(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);	

	if(priv->bfirst_init)
	{
		printk("gen_RefreshLedState first init\n");
		return;
	}

	//printk("priv->ieee80211->RfOffReason=%x\n", priv->ieee80211->RfOffReason);
	if(priv->ieee80211->RfOffReason == RF_CHANGE_BY_IPS )
	{
		//PlatformEFIOWrite1Byte(Adapter, 0x2f2, 0x00);
		SwLedOn(dev, pLed0);
//		Adapter->HalFunc.LedControlHandler(Adapter,LED_CTL_NO_LINK); 
	}
	else		// SW/HW radio off
	{
		// Turn off LED if RF is not ON.
		SwLedOff(dev, pLed0);		
//		Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_POWER_OFF); 
	}

}	// gen_RefreshLedState
/*-----------------------------------------------------------------------------
 * Function:	MacConfigBeforeFwDownload()
 *
 * Overview:	Copy from WMAc for init setting. MAC config before FW download
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	04/29/2008	MHC		The same as 92SE MAC initialization.  
 *	07/11/2008	MHC		MAC config before FW download
 *	09/02/2008	MHC		The initialize sequence can preven NIC reload fail
 *						NIC will not disappear when power domain init twice.
 *	11/04/2008	MHC		Modify power on seq, we must reset MACIO/CPU/Digital Core
 *						Pull low and then high.
 *
 *---------------------------------------------------------------------------*/
static void MacConfigBeforeFwDownload(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);	
	u8				i;
	u8				tmpU1b;
	u16				tmpU2b;
	//u32				addr;
	u8				PollingCnt = 20;

	// 2008/05/14 MH For 92SE rest before init MAC data sheet not defined now. 
	RT_TRACE(COMP_INIT, "--->MacConfigBeforeFwDownload()\n");
//	printk("--->MacConfigBeforeFwDownload()\n");
	//added by amy 090408 from windows
	if(priv->bfirst_init)
	{
		// 2009/03/24 MH Reset PCIE Digital
		tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);
		tmpU1b &= 0xFE;
		write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b);
		udelay(1);
		write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b|BIT0);
	}
	//added by amy 090408 from windows end
	// Switch to SW IO control
	tmpU1b = read_nic_byte(dev, (SYS_CLKR + 1));
	if(tmpU1b & BIT7)
	{
		tmpU1b &= ~(BIT6 | BIT7);
		if(!HalSetSysClk8192SE(dev, tmpU1b))
			return; // Set failed, return to prevent hang.
	}
#if 0
	// Switch to SW IO control
	tmpU2b = read_nic_word(dev, SYS_CLKR);
	if (tmpU2b & BIT15)	// If not FW control, we may fail to return from HW SW to HW
	{
		tmpU2b &= ~(BIT14|BIT15);
		RT_TRACE(COMP_INIT, "Return to HW CTRL\n");
		write_nic_word(dev, SYS_CLKR, tmpU2b);
	}
	// Prevent Hang on some platform!!!!
	udelay(200);
#endif
	// Clear FW RPWM for FW control LPS. by tynli. 2009.02.23
	write_nic_byte(dev, RPWM, 0x0);

	// Reset MAC-IO and CPU and Core Digital BIT10/11/15
	tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);
	tmpU1b &= 0x73;
	write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b);
	// wait for BIT 10/11/15 to pull high automatically!!
	mdelay(1);
	
	write_nic_byte(dev, CMDR, 0);
	write_nic_byte(dev, TCR, 0);

#if (DEMO_BOARD_SUPPORT == 0)
	tmpU1b = read_nic_byte(dev, SPS1_CTRL);
	tmpU1b &= 0xfc;
	write_nic_byte(dev, SPS1_CTRL, tmpU1b);
#endif

	// Data sheet not define 0x562!!! Copy from WMAC!!!!!
	tmpU1b = read_nic_byte(dev, 0x562);
	tmpU1b |= 0x08;
	write_nic_byte(dev, 0x562, tmpU1b);
	tmpU1b &= ~(BIT3);
	write_nic_byte(dev, 0x562, tmpU1b);
	
	//tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);
	//tmpU1b &= 0x73;
	//write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b);

	//tmpU1b = read_nic_byte(dev, SYS_CLKR);	
	//tmpU1b &= 0xfa;
	//write_nic_byte(dev, SYS_CLKR, tmpU1b);

	// Switch to 80M clock
	//write_nic_byte(dev, SYS_CLKR, SYS_CLKSEL_80M);
	
	 // Enable VSPS12 LDO Macro block
	//tmpU1b = read_nic_byte(dev, SPS1_CTRL);	
	//write_nic_byte(dev, SPS1_CTRL, (tmpU1b|SPS1_LDEN));

	//Enable AFE clock source
	RT_TRACE(COMP_INIT, "Enable AFE clock source\r\n");	
	tmpU1b = read_nic_byte(dev, AFE_XTAL_CTRL);	
	write_nic_byte(dev, AFE_XTAL_CTRL, (tmpU1b|0x01));
	// Delay 1.5ms
	udelay(1500);	
	tmpU1b = read_nic_byte(dev, AFE_XTAL_CTRL+1);	
	write_nic_byte(dev, AFE_XTAL_CTRL+1, (tmpU1b&0xfb));


	//Enable AFE Macro Block's Bandgap
	RT_TRACE(COMP_INIT, "Enable AFE Macro Block's Bandgap\r\n");	
	tmpU1b = read_nic_byte(dev, AFE_MISC);	
	write_nic_byte(dev, AFE_MISC, (tmpU1b|BIT0));
	mdelay(1);

	//Enable AFE Mbias
	RT_TRACE(COMP_INIT, "Enable AFE Mbias\r\n");	
	tmpU1b = read_nic_byte(dev, AFE_MISC);	
	write_nic_byte(dev, AFE_MISC, (tmpU1b|0x02));
	mdelay(1);
	
	//Enable LDOA15 block	
	tmpU1b = read_nic_byte(dev, LDOA15_CTRL);	
	write_nic_byte(dev, LDOA15_CTRL, (tmpU1b|BIT0));

	//Enable VSPS12_SW Macro Block
	//tmpU1b = read_nic_byte(dev, SPS1_CTRL);	
	//write_nic_byte(dev, SPS1_CTRL, (tmpU1b|SPS1_SWEN));

	//Enable AFE Macro Block's Mbias
	//tmpU1b = read_nic_byte(dev, AFE_MISC);	
	//write_nic_byte(dev, AFE_MISC, (tmpU1b|AFE_MBEN));

	// Set Digital Vdd to Retention isolation Path.
	tmpU2b = read_nic_word(dev, SYS_ISO_CTRL);	
	write_nic_word(dev, SYS_ISO_CTRL, (tmpU2b|BIT11));

	// Attatch AFE PLL to MACTOP/BB/PCIe Digital
	//tmpU2b = read_nic_word(dev, SYS_ISO_CTRL);	
	//write_nic_word(dev, SYS_ISO_CTRL, (tmpU2b &(~BIT12)));

	// 2008/09/25 MH From SD1 Jong, For warm reboot NIC disappera bug.
	tmpU2b = read_nic_word(dev, SYS_FUNC_EN);
	write_nic_word(dev, SYS_FUNC_EN, tmpU2b |= BIT13);

	//Enable AFE clock
	//tmpU2b = read_nic_word(dev, AFE_XTAL_CTRL);	
	//write_nic_word(dev, AFE_XTAL_CTRL, (tmpU2b &(~XTAL_GATE_AFE)));

	write_nic_byte(dev, SYS_ISO_CTRL+1, 0x68);

	//Enable AFE PLL Macro Block
	tmpU1b = read_nic_byte(dev, AFE_PLL_CTRL);	
	write_nic_byte(dev, AFE_PLL_CTRL, (tmpU1b|BIT0|BIT4));
	// Enable MAC 80MHZ clock  //added by amy 090408 
	tmpU1b = read_nic_byte(dev, AFE_PLL_CTRL+1);	
	write_nic_byte(dev, AFE_PLL_CTRL+1, (tmpU1b|BIT0));
	mdelay(1);

	// Release isolation AFE PLL & MD
	write_nic_byte(dev, SYS_ISO_CTRL, 0xA6);

	//Enable MAC clock
	tmpU2b = read_nic_word(dev, SYS_CLKR);	
	//write_nic_word(dev, SYS_CLKR, (tmpU2b|SYS_MAC_CLK_EN));
	write_nic_word(dev, SYS_CLKR, (tmpU2b|BIT12|BIT11));

	//Enable Core digital and enable IOREG R/W
	tmpU2b = read_nic_word(dev, SYS_FUNC_EN);	
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b|BIT11));

	//added by amy 090408
	tmpU1b = read_nic_byte(dev, SYS_FUNC_EN+1);
	write_nic_byte(dev, SYS_FUNC_EN+1, tmpU1b&~(BIT7));
	//added by amy 090408 end
	
	//enable REG_EN
	write_nic_word(dev, SYS_FUNC_EN, (tmpU2b|BIT11|BIT15));

	 //Switch the control path.
	 tmpU2b = read_nic_word(dev, SYS_CLKR);	
	write_nic_word(dev, SYS_CLKR, (tmpU2b&(~BIT2)));

	tmpU1b = read_nic_byte(dev, (SYS_CLKR + 1));
	tmpU1b = ((tmpU1b | BIT7) & (~BIT6));
	if(!HalSetSysClk8192SE(dev, tmpU1b))
		return; // Set failed, return to prevent hang.

#if 0	
	tmpU2b = read_nic_word(dev, SYS_CLKR);	
	write_nic_word(dev, SYS_CLKR, ((tmpU2b|BIT15)&(~BIT14)));
#endif

	//write_nic_word(dev, CMDR, 0x37FC);
	write_nic_word(dev, CMDR, 0x07FC);//added by amy 090408
	
#if 1
	// 2008/09/02 MH We must enable the section of code to prevent load IMEM fail.
	// Load MAC register from WMAc temporarily We simulate macreg.txt HW will provide 
	// MAC txt later 
	write_nic_byte(dev, 0x6, 0x30);
	//write_nic_byte(dev, 0x48, 0x2f);
	write_nic_byte(dev, 0x49, 0xf0);

	write_nic_byte(dev, 0x4b, 0x81);

	write_nic_byte(dev, 0xb5, 0x21);

	write_nic_byte(dev, 0xdc, 0xff);
	write_nic_byte(dev, 0xdd, 0xff);	
	write_nic_byte(dev, 0xde, 0xff);
	write_nic_byte(dev, 0xdf, 0xff);

	write_nic_byte(dev, 0x11a, 0x00);
	write_nic_byte(dev, 0x11b, 0x00);

	for (i = 0; i < 32; i++)
		write_nic_byte(dev, INIMCS_SEL+i, 0x1b);	

	write_nic_byte(dev, 0x236, 0xff);
	
	write_nic_byte(dev, 0x503, 0x22);
	//write_nic_byte(dev, 0x560, 0x09);
	// 2008/11/26 MH From SD1 Jong's suggestion for clock request bug.
	//write_nic_byte(dev, 0x560, 0x79);
	// 2008/12/26 MH From SD1 Victor's suggestion for clock request bug.
	write_nic_byte(dev, 0x560, 0x40);

	write_nic_byte(dev, DBG_PORT, 0x91);	
#endif

	//Set RX Desc Address
	write_nic_dword(dev, RDQDA, priv->rx_ring_dma);
#if 0
	write_nic_dword(dev, RCDA, 
	pHalData->RxDescMemory[RX_CMD_QUEUE].PhysicalAddressLow);
#endif	
	//Set TX Desc Address
	rtl8192_tx_enable(dev);

	write_nic_word(dev, CMDR, 0x37FC);
	//
	// <Roger_EXP> To make sure that TxDMA can ready to download FW.
	// We should reset TxDMA if IMEM RPT was not ready.
	// Suggested by SD1 Alex. 2008.10.23.
	//
	do
	{
		tmpU1b = read_nic_byte(dev, TCR);
		if((tmpU1b & TXDMA_INIT_VALUE) == TXDMA_INIT_VALUE)
			break;	
			
		udelay(5);
	}while(PollingCnt--);	// Delay 1ms
	
	if(PollingCnt <= 0 )
	{
		RT_TRACE(COMP_ERR, "MacConfigBeforeFwDownloadASIC(): Polling TXDMA_INIT_VALUE timeout!! Current TCR(%#x)\n", tmpU1b);
		tmpU1b = read_nic_byte(dev, CMDR);			
		write_nic_byte(dev, CMDR, tmpU1b&(~TXDMA_EN));
		udelay(2);
		write_nic_byte(dev, CMDR, tmpU1b|TXDMA_EN);// Reset TxDMA
	}

	//added by amy 090408
	// After MACIO reset,we must refresh LED state.
	gen_RefreshLedState(dev);
	//added by amy 090408 end

	RT_TRACE(COMP_INIT, "<---MacConfigBeforeFwDownload()\n");

}	/* MacConfigBeforeFwDownload */
#endif

static void MacConfigAfterFwDownload(struct net_device* dev)
{
	u8				i;
	//u8				tmpU1b;
	u16				tmpU2b;
	struct r8192_priv* priv = ieee80211_priv(dev);

	//
	// 1. System Configure Register (Offset: 0x0000 - 0x003F)
	//
	
	//
	// 2. Command Control Register (Offset: 0x0040 - 0x004F)
	//
	// Turn on 0x40 Command register
	write_nic_word(dev, CMDR, 
	BBRSTn|BB_GLB_RSTn|SCHEDULE_EN|MACRXEN|MACTXEN|DDMA_EN|FW2HW_EN|
	RXDMA_EN|TXDMA_EN|HCI_RXDMA_EN|HCI_TXDMA_EN);

	// Set TCR TX DMA pre 2 FULL enable bit	
	write_nic_dword(dev, TCR, 
	read_nic_dword(dev, TCR)|TXDMAPRE2FULL);
	// Set RCR	
	write_nic_dword(dev, RCR, priv->ReceiveConfig);

	//
	// 3. MACID Setting Register (Offset: 0x0050 - 0x007F)
	//
//move to start adapter
#if 0
	for (i=0;i<6;i++)
	write_nic_byte(dev, MACIDR0+i, dev->dev_addr[i]); 	
#endif
	//
	// 4. Timing Control Register  (Offset: 0x0080 - 0x009F)
	//
	// Set CCK/OFDM SIFS
	write_nic_word(dev, SIFS_CCK, 0x0a0a); // CCK SIFS shall always be 10us.
	write_nic_word(dev, SIFS_OFDM, 0x1010);
	//3
	//Set AckTimeout
	write_nic_byte(dev, ACK_TIMEOUT, 0x40);
	
	//Beacon related
	write_nic_word(dev, BCN_INTERVAL, 100);
	write_nic_word(dev, ATIMWND, 2);	
	//write_nic_word(dev, BCN_DRV_EARLY_INT, 0x0022);
	//write_nic_word(dev, BCN_DMATIME, 0xC8);
	//write_nic_word(dev, BCN_ERR_THRESH, 0xFF);
	//write_nic_byte(dev, MLT, 8);

	//
	// 5. FIFO Control Register (Offset: 0x00A0 - 0x015F)
	//
	// 5.1 Initialize Number of Reserved Pages in Firmware Queue
	// Firmware allocate now, associate with FW internal setting.!!!
#if 0
	write_nic_dword(dev, RQPN1,  
	NUM_OF_PAGE_IN_FW_QUEUE_BK<<0 | NUM_OF_PAGE_IN_FW_QUEUE_BE<<8 |\
	NUM_OF_PAGE_IN_FW_QUEUE_VI<<16 | NUM_OF_PAGE_IN_FW_QUEUE_VO<<24);												
	write_nic_dword(dev, RQPN2, 
	NUM_OF_PAGE_IN_FW_QUEUE_HCCA << 0 | NUM_OF_PAGE_IN_FW_QUEUE_CMD << 8|\
	NUM_OF_PAGE_IN_FW_QUEUE_MGNT << 16 |NUM_OF_PAGE_IN_FW_QUEUE_HIGH << 24);
	write_nic_dword(dev, RQPN3, 
	NUM_OF_PAGE_IN_FW_QUEUE_BCN<<0 | NUM_OF_PAGE_IN_FW_QUEUE_PUB<<8);
	// Active FW/MAC to load the new RQPN setting
	write_nic_byte(dev, LD_RQPN, BIT7);
#endif

	// 5.2 Setting TX/RX page size 0/1/2/3/4=64/128/256/512/1024
	//write_nic_byte(dev, PBP, 0x22);


	// 5.3 Set driver info, we only accept PHY status now.
	//write_nic_byte(dev, RXDRVINFO_SZ, 4);  

	// 5.4 Set RXDMA arbitration to control RXDMA/MAC/FW R/W for RXFIFO 
	write_nic_byte(dev, RXDMA, 
							read_nic_byte(dev, RXDMA)|BIT6);

	//
	// 6. Adaptive Control Register  (Offset: 0x0160 - 0x01CF)
	//
	// Set RRSR to all legacy rate and HT rate
	// CCK rate is supported by default.
	// CCK rate will be filtered out only when associated AP does not support it.
	// Only enable ACK rate to OFDM 24M
	// 2008/09/24 MH Disable RRSR for CCK rate in A-Cut	
//#if (DISABLE_CCK == 1)
	if (priv->card_8192_version== VERSION_8192S_ACUT)
		write_nic_byte(dev, RRSR, 0xf0);
	else if (priv->card_8192_version == VERSION_8192S_BCUT)
		write_nic_byte(dev, RRSR, 0xff);
	write_nic_byte(dev, RRSR+1, 0x01);
	write_nic_byte(dev, RRSR+2, 0x00);

	// 2008/09/22 MH A-Cut IC do not support CCK rate. We forbid ARFR to 
	// fallback to CCK rate
	for (i = 0; i < 8; i++)
	{
	// 2008/09/24 MH Disable RRSR for CCK rate in A-Cut

//#if (DISABLE_CCK == 1)
		if (priv->card_8192_version == VERSION_8192S_ACUT)
			write_nic_dword(dev, ARFR0+i*4, 0x1f0ff0f0);
		else if (priv->card_8192_version == VERSION_8192S_BCUT)
			write_nic_dword(dev, ARFR0+i*4, 0x1f0ff0f0);
	}

	// Different rate use different AMPDU size
	// MCS32/ MCS15_SG use max AMPDU size 15*2=30K
	write_nic_byte(dev, AGGLEN_LMT_H, 0x0f);
	// MCS0/1/2/3 use max AMPDU size 4*2=8K
	write_nic_word(dev, AGGLEN_LMT_L, 0x7442);
	// MCS4/5 use max AMPDU size 8*2=16K 6/7 use 10*2=20K
	write_nic_word(dev, AGGLEN_LMT_L+2, 0xddd7);
	// MCS8/9 use max AMPDU size 8*2=16K 10/11 use 10*2=20K
	write_nic_word(dev, AGGLEN_LMT_L+4, 0xd772);
	// MCS12/13/14/15 use max AMPDU size 15*2=30K
	write_nic_word(dev, AGGLEN_LMT_L+6, 0xfffd);
	//write_nic_word(dev, AGGLEN_LMT_L+6, 0xFFFF);

	// Set Data / Response auto rate fallack retry count
	write_nic_dword(dev, DARFRC, 0x04010000);
	write_nic_dword(dev, DARFRC+4, 0x09070605);
	write_nic_dword(dev, RARFRC, 0x04010000);
	write_nic_dword(dev, RARFRC+4, 0x09070605);

	// MCS/CCK TX Auto Gain Control Register 
	//write_nic_dword(dev, MCS_TXAGC, 0x0D0C0C0C);
	//write_nic_dword(dev, MCS_TXAGC+4, 0x0F0E0E0D);
	//write_nic_word(dev, CCK_TXAGC, 0x0000);
	
	
	//
	// 7. EDCA Setting Register (Offset: 0x01D0 - 0x01FF)
	//
	// BCNTCFG
	//write_nic_word(dev, BCNTCFG, 0x0000);
	// Set all rate to support SG
	write_nic_word(dev, SG_RATE, 0xFFFF);


	//
	// 8. WMAC, BA, and CCX related Register (Offset: 0x0200 - 0x023F)
	//
	// Set NAV protection length
	write_nic_word(dev, NAV_PROT_LEN, 0x0080);	
	// CF-END Threshold
	write_nic_byte(dev, CFEND_TH, 0xFF);	
	// Set AMPDU minimum space
	write_nic_byte(dev, AMPDU_MIN_SPACE, 0x07);
	// Set TXOP stall control for several queue/HI/BCN/MGT/
	write_nic_byte(dev, TXOP_STALL_CTRL, 0x00);
	
	// 9. Security Control Register (Offset: 0x0240 - 0x025F)
	// 10. Power Save Control Register (Offset: 0x0260 - 0x02DF)
	// 11. General Purpose Register (Offset: 0x02E0 - 0x02FF)
	// 12. Host Interrupt Status Register (Offset: 0x0300 - 0x030F)
	// 13. Test Mode and Debug Control Register (Offset: 0x0310 - 0x034F)

	//
	// 13. HOST/MAC PCIE Interface Setting
	//	
	// Set Tx dma burst 
	/*write_nic_byte(dev, PCIF, ((MXDMA2_NoLimit<<MXDMA2_RX_SHIFT) | \
											(MXDMA2_NoLimit<<MXDMA2_TX_SHIFT) | \
											(1<<MULRW_SHIFT)));*/

	//
	// 14. Set driver info, we only accept PHY status now.
	//
	write_nic_byte(dev, RXDRVINFO_SZ, 4);  

	//
	// 15. For EEPROM R/W Workaround
	// 16. For EFUSE to share SYS_FUNC_EN with EEPROM!!!
	//
	tmpU2b= read_nic_byte(dev, SYS_FUNC_EN);
	write_nic_byte(dev, SYS_FUNC_EN, tmpU2b | BIT13);  
	tmpU2b= read_nic_byte(dev, SYS_ISO_CTRL);
	write_nic_byte(dev, SYS_ISO_CTRL, tmpU2b & (~BIT8));  

	//
	// 16. For EFUSE
	//
	if (priv->epromtype == EEPROM_BOOT_EFUSE)	// We may R/W EFUSE in EEPROM mode
	{	
		u8	tempval;		
		
		tempval = read_nic_byte(dev, SYS_ISO_CTRL+1); 
		tempval &= 0xFE;
		write_nic_byte(dev, SYS_ISO_CTRL+1, tempval); 

		// Enable LDO 2.5V for write action
		//tempval = read_nic_byte(dev, EFUSE_TEST+3);
		//write_nic_byte(dev, EFUSE_TEST+3, (tempval | 0x80));

		// Change Efuse Clock for write action
		//write_nic_byte(dev, EFUSE_CLK, 0x03);
		
		// Change Program timing
		write_nic_byte(dev, EFUSE_CTRL+3, 0x72); 
		RT_TRACE(COMP_INIT, "EFUSE CONFIG OK\n");
	}
	RT_TRACE(COMP_INIT, "MacConfigAfterFwDownload OK\n");

}	/* MacConfigAfterFwDownload */
//added by amy 090330
void SetHwReg8192SE(struct net_device *dev,u8 variable,u8* val)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	switch(variable)
	{
		case HW_VAR_AMPDU_MIN_SPACE:
		{
			u8	MinSpacingToSet;
			u8	SecMinSpace;

			MinSpacingToSet = *((u8*)val);
			if(MinSpacingToSet <= 7)
			{
				if((priv->ieee80211->current_network.capability & WLAN_CAPABILITY_PRIVACY) == 0)  //NO_Encryption
					SecMinSpace = 0;
				else if(priv->ieee80211->is_ap_in_wep_tkip(dev))
					SecMinSpace = 7;
				else
					SecMinSpace = 1;

				if(MinSpacingToSet < SecMinSpace)
					MinSpacingToSet = SecMinSpace;
				priv->ieee80211->MinSpaceCfg = ((priv->ieee80211->MinSpaceCfg&0xf8) |MinSpacingToSet);
				RT_TRACE(COMP_MLME, "Set HW_VAR_AMPDU_MIN_SPACE: %#x\n", priv->ieee80211->MinSpaceCfg);
				write_nic_byte(dev, AMPDU_MIN_SPACE, priv->ieee80211->MinSpaceCfg);	
			}
		}
		break;	
		case HW_VAR_SHORTGI_DENSITY:
		{
			u8	DensityToSet;
		
			DensityToSet = *((u8*)val);		
			//DensityToSet &= 0x1f;
			priv->ieee80211->MinSpaceCfg|= (DensityToSet<<3);		
			RT_TRACE(COMP_MLME, "Set HW_VAR_SHORTGI_DENSITY: %#x\n", priv->ieee80211->MinSpaceCfg);
			write_nic_byte(dev, AMPDU_MIN_SPACE, priv->ieee80211->MinSpaceCfg);
			break;		
		}
		case HW_VAR_AMPDU_FACTOR:
		{
			u8	FactorToSet;
			u8	RegToSet;
			u8	FactorLevel[18] = {2, 4, 4, 7, 7, 13, 13, 13, 2, 7, 7, 13, 13, 15, 15, 15, 15, 0};
			u8	index = 0;
		
			FactorToSet = *((u8*)val);
			if(FactorToSet <= 3)
			{
				FactorToSet = (1<<(FactorToSet + 2));
				if(FactorToSet>0xf)
					FactorToSet = 0xf;

				for(index=0; index<17; index++)
				{
					if(FactorLevel[index] > FactorToSet)
						FactorLevel[index] = FactorToSet;
				}

				for(index=0; index<8; index++)
				{
					RegToSet = ((FactorLevel[index*2]) | (FactorLevel[index*2+1]<<4));
					write_nic_byte(dev, AGGLEN_LMT_L+index, RegToSet);
				}
				RegToSet = ((FactorLevel[16]) | (FactorLevel[17]<<4));
				write_nic_byte(dev, AGGLEN_LMT_H, RegToSet);

				RT_TRACE(COMP_MLME, "Set HW_VAR_AMPDU_FACTOR: %#x\n", FactorToSet);
			}
		}
		break;
		default:
			break;
	}
}
//added by amy 090330 end
void HwConfigureRTL8192SE(struct net_device *dev)
{

	struct r8192_priv* priv = ieee80211_priv(dev);
	
	u8	regBwOpMode = 0;
	u32	regRATR = 0, regRRSR = 0;
	u8	regTmp = 0;


	//1 This part need to modified according to the rate set we filtered!!
	//
	// Set RRSR, RATR, and BW_OPMODE registers
	//
	switch(priv->ieee80211->mode)
	{
	case WIRELESS_MODE_B:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK;
		regRRSR = RATE_ALL_CCK;
		break;
	case WIRELESS_MODE_A:
		regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_G:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_AUTO:
	case WIRELESS_MODE_N_24G:
		// It support CCK rate by default.
		// CCK rate will be filtered out only when associated AP does not support it.
			regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_N_5G:
		regBwOpMode = BW_OPMODE_5G;
		regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	}

	regTmp = read_nic_byte(dev, INIRTSMCS_SEL);
#if (RTL8192SU_DISABLE_CCK_RATE == 1)
	regRRSR = ((regRRSR & 0x000ffff0)<<8) | regTmp;
#else
	regRRSR = ((regRRSR & 0x000fffff)<<8) | regTmp;
#endif
	write_nic_dword(dev, INIRTSMCS_SEL, regRRSR);

	// 2007/02/07 Mark by Emily becasue we have not verify whether this register works	
	write_nic_byte(dev, BW_OPMODE, regBwOpMode);
#if 0	// For 8192SE disable
	dev->HalFunc.SetHwRegHandler(dev, HW_VAR_RATR_0, (pu1Byte)(&regRATR));
	
	regTmp = read_nic_byte(dev, 0x313);
	regRRSR = ((regTmp) << 24) | (regRRSR & 0x00ffffff);
	PlatformEFIOWrite4Byte(dev, RRSR, regRRSR);
#endif
	//
	// Set Retry Limit here
	//
//FIXME:YJ,090316
#if 0
	if(Adapter->MgntInfo.bPSPXlinkMode)	
	{
		pHalData->ShortRetryLimit = 3;
		pHalData->LongRetryLimit = 3;		
	}		
#endif
	write_nic_word(dev, 	RETRY_LIMIT, 
							priv->ShortRetryLimit<< RETRY_LIMIT_SHORT_SHIFT | \
							priv->ShortRetryLimit << RETRY_LIMIT_LONG_SHIFT);
//	dev->HalFunc.SetHwRegHandler(dev, HW_VAR_RETRY_LIMIT, (pu1Byte)(&pHalData->ShortRetryLimit));

	write_nic_byte(dev, MLT, 0x8f);
	// Set Contention Window here		

	// Set Tx AGC

	// Set Tx Antenna including Feedback control
		
	// Set Auto Rate fallback control
	//
	// For Min Spacing configuration.
	//
#if 0	
	switch(priv->rf_type)
	{
		case RF_1T2R:
		case RF_1T1R:
			RT_TRACE(COMP_INIT, "Initializeadapter: RF_Type%s\n", priv->rf_type==RF_1T1R? "(1T1R)":"(1T2R)");
			regTmp = (MAX_MSS_DENSITY_1T<<3);						
			break;
		case RF_2T2R:
		case RF_2T2R_GREEN:
			RT_TRACE(COMP_INIT, "Initializeadapter:RF_Type(2T2R)\n");
			regTmp = (MAX_MSS_DENSITY_2T<<3);			
			break;
	}
	write_nic_byte(dev, AMPDU_MIN_SPACE, regTmp);
#endif
	priv->ieee80211->MinSpaceCfg = 0x90;	//cosa, asked by scott, for MCS15 short GI, padding patch, 0x237[7:3] = 0x12.
	SetHwReg8192SE(dev, HW_VAR_AMPDU_MIN_SPACE, (u8*)(&priv->ieee80211->MinSpaceCfg));
}

void 
RF_RECOVERY(struct net_device*dev)
{
	u8 offset = 0x25;
	for(;offset<0x29;offset++)
	{
		PHY_RFShadowCompareFlagSet(dev, (RF90_RADIO_PATH_E)0, offset,TRUE);
		if(PHY_RFShadowCompare(dev, (RF90_RADIO_PATH_E)0, offset))
		{
			PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)0, offset, TRUE);
			PHY_RFShadowRecorver( dev, (RF90_RADIO_PATH_E)0, offset);
		}
	}
}


bool rtl8192se_adapter_start(struct net_device* dev)
{ 	
	struct r8192_priv *priv = ieee80211_priv(dev);	
	//struct ieee80211_device* ieee = priv->ieee80211;
	//u32				ulRegRead;
	bool rtStatus 	= true;	
	u8				tmpU1b;
	//u16				tmpU2b;	
	u8				eRFPath;
	u8				fw_download_times = 1;
	RT_TRACE(COMP_INIT, "rtl8192se_adapter_start()\n");
	priv->being_init_adapter = true;	
start:
	rtl8192_pci_resetdescring(dev);
	//
	// 1. MAC Initialize
	//
	// Before FW download, we have to set some MAC register
	MacConfigBeforeFwDownload(dev);
    
	// Read Version ID from Reg4 bit 16-19. EEPROM may not PGed, we will asign as ACUT
	priv->card_8192_version 
	= (VERSION_8192S)((read_nic_dword(dev, PMC_FSM)>>16)&0xF);
    
    //kassert();  // pass
            
	// Read macreg.txt before firmware download !!???
	// Firmware download 
	rtStatus = FirmwareDownload92S((struct net_device*)dev); //WB		
	
	if(rtStatus != true)
	{
		if(fw_download_times <= 10){
			RT_TRACE(COMP_INIT, "rtl8192se_adapter_start(): Download Firmware failed %d times, Download again!!\n",fw_download_times);
			fw_download_times = fw_download_times + 1;
			goto start;
		}else{
			RT_TRACE(COMP_INIT, "rtl8192se_adapter_start(): Download Firmware failed 10, end!!\n");
			goto end;
		}
	}
	
    //kassert();  // ????
    
	// After FW download, we have to reset MAC register
	MacConfigAfterFwDownload(dev);

	// Read Efuse after MAC init OK, we can switch efuse clock from 40M to 500K.
	//ReadAdapterInfo8192S(dev);
	
#if (RTL8192S_DISABLE_FW_DM == 1)
	write_nic_dword(dev, WFM5, FW_DM_DISABLE); 
#endif

    //kassert();  // 5  - pass
    
	//
	// 2. Initialize MAC/PHY Config by MACPHY_reg.txt
	//
#if (HAL_MAC_ENABLE == 1)
	RT_TRACE(COMP_INIT, "MAC Config Start!\n");
	if (PHY_MACConfig8192S(dev) != true)
	{
		RT_TRACE(COMP_ERR, "MAC Config failed\n");
		return rtStatus;
	}
	RT_TRACE(COMP_INIT, "MAC Config Finished!\n");
#endif	// #if (HAL_MAC_ENABLE == 1)	
	
	//
	// 2008/12/19 MH Make sure BB/RF write OK. We should prevent enter IPS. radio off.
	// We must set flag avoid BB/RF config period later!!
	//
	write_nic_dword(dev, CMDR, 0x37FC);     
    
	//
	// 3. Initialize BB After MAC Config PHY_reg.txt, AGC_Tab.txt
	//
#if (HAL_BB_ENABLE == 1)
    //kassert();  // 5.5  - pass
	RT_TRACE(COMP_INIT, "BB Config Start!\n");
	
	if (PHY_BBConfig8192S(dev) != true)
	{
		RT_TRACE(COMP_INIT, "BB Config failed\n");
		return rtStatus;
	}
	
	RT_TRACE(COMP_INIT, "BB Config Finished!\n");
#endif	// #if (HAL_BB_ENABLE == 1)

    //kassert();  // 4 - failed

	//
	// 4. Initiailze RF RAIO_A.txt RF RAIO_B.txt
	//
	// 2007/11/02 MH Before initalizing RF. We can not use FW to do RF-R/W.
	priv->Rf_Mode = RF_OP_By_SW_3wire;
#if (HAL_RF_ENABLE == 1)		
	RT_TRACE(COMP_INIT, "RF Config started!\n");

#if (RTL92SE_FPGA_VERIFY == 0)
	// Before RF-R/W we must execute the IO from Scott's suggestion.
	write_nic_byte(dev, AFE_XTAL_CTRL+1, 0xDB);
	if(priv->card_8192_version== VERSION_8192S_ACUT)
		write_nic_byte(dev, SPS1_CTRL+3, 0x07);
	else
		write_nic_byte(dev, RF_CTRL, 0x07);
#endif	
	if(PHY_RFConfig8192S(dev) != true)
	{
		RT_TRACE(COMP_ERR, "RF Config failed\n");
		return rtStatus;
	}
	RT_TRACE(COMP_INIT, "RF Config Finished!\n");	
#endif	// #if (HAL_RF_ENABLE == 1)	

	// After read predefined TXT, we must set BB/MAC/RF register as our requirement
    

	/*---- Set CCK and OFDM Block "ON"----*/
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bCCKEn, 0x1);
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, 0x1);
	
	//3 Set Hardware(Do nothing now)
	HwConfigureRTL8192SE(dev);		

	//3 Set Wireless Mode
	// TODO: Emily 2006.07.13. Wireless mode should be set according to registry setting and RF type
	//Default wireless mode is set to "WIRELESS_MODE_N_24G|WIRELESS_MODE_G", 
	//and the RRSR is set to Legacy OFDM rate sets. We do not include the bit mask 
	//of WIRELESS_MODE_B currently. Emily, 2006.11.13
	//For wireless mode setting from mass. 
	if(priv->ResetProgress == RESET_TYPE_NORESET)
		rtl8192_SetWirelessMode(dev, priv->ieee80211->mode);
	//-----------------------------------------------------------------------------
	// Set up security related. 070106, by rcnjko:
	// 1. Clear all H/W keys.
	// 2. Enable H/W encryption/decryption.
	//-----------------------------------------------------------------------------
	CamResetAllEntry(dev);
	{
		u8 SECR_value = 0x0;
		SECR_value |= SCR_TxEncEnable;
		SECR_value |= SCR_RxDecEnable;
		SECR_value |= SCR_NoSKMC;
		write_nic_byte(dev, SECR, SECR_value);
	}

	{
		int i;
		for (i=0; i<4; i++)
		 write_nic_dword(dev,WDCAPARA_ADD[i], 0x5e4322); 
	}
	//
	// Read EEPROM TX power index and PHY_REG_PG.txt to capture correct
	// TX power index for different rate set.
	//
//	if(priv->card_8192_version>= VERSION_8192S_ACUT) //always true
	{
		/* Get original hw reg values */
		PHY_GetHWRegOriginalValue(dev);
		/* Write correct tx power index */
		rtl8192_phy_setTxPower(dev, priv->chan);
	}

    //kassert();  // 3 - failed
    
	//2=======================================================
	// RF Power Save
	//2=======================================================
#if 1
	if(priv->RegRfOff == true)
	{ // User disable RF via registry.
		RT_TRACE((COMP_INIT|COMP_RF), "InitializeAdapter8190(): Turn off RF for RegRfOff ----------\n");
//		printk("InitializeAdapter8190(): Turn off RF for RegRfOff ----------\n");
		MgntActSet_RF_State(dev, eRfOff, RF_CHANGE_BY_SW);

		// Those action will be discard in MgntActSet_RF_State because off the same state
		for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
	
	}
	else if(priv->ieee80211->RfOffReason > RF_CHANGE_BY_PS)
	{ // H/W or S/W RF OFF before sleep.
		RT_TRACE((COMP_INIT|COMP_RF), "InitializeAdapter8190(): Turn off RF for RfOffReason(%d) ----------\n", priv->ieee80211->RfOffReason);
//		printk("InitializeAdapter8190(): Turn off RF for RfOffReason(%d) ----------\n", priv->ieee80211->RfOffReason);
		MgntActSet_RF_State(dev, eRfOff, priv->ieee80211->RfOffReason);
	}
	else
	{
            //lzm gpio radio on/off is out of adapter start
            if(priv->bHwRadioOff == false){
		priv->ieee80211->eRFPowerState = eRfOn;
		priv->ieee80211->RfOffReason = 0; 
		// LED control
		if(priv->ieee80211->LedControlHandler)
			priv->ieee80211->LedControlHandler(dev, LED_CTL_POWER_ON);
	}		
	}		
#endif

    //kassert();  // 2 - fail

	//
	// Execute TX power tracking later
	//

	//  For test.
	//DM_ShadowInit(dev);
	
	// We must set MAC address after firmware download. HW do not support MAC addr
	// autoload now.
	{
		int i;
		for (i=0;i<6;i++)
			write_nic_byte(dev, MACIDR0+i, dev->dev_addr[i]); 	
	}
#if 0
	if (pHalData->VersionID == VERSION_8192S_BCUT)
	{	
		PHY_IQCalibrateBcut(dev);
	}
	else if (pHalData->VersionID == VERSION_8192S_ACUT)
	{
		PHY_IQCalibrate(dev);
	}
#endif
	// EEPROM R/W workaround
	tmpU1b = read_nic_byte(dev, MAC_PINMUX_CFG);
	write_nic_byte(dev, MAC_PINMUX_CFG, tmpU1b&(~BIT3));
	
	//kassert();  // 1 - fail
	
	//
	// <Roger_Notes> We enable high power mechanism after NIC initialized. 
	// 2008.11.27.
	//
	write_nic_dword(dev, WFM5, FW_RA_RESET); 
	ChkFwCmdIoDone(dev);
	write_nic_dword(dev, WFM5, FW_RA_ACTIVE); 
	ChkFwCmdIoDone(dev);
	write_nic_dword(dev, WFM5, FW_RA_REFRESH); 
	
	// Execute RF check with last rounf resume!!!!
	
	//
	// 2008/12/22 MH Wroaround to rePG section1 data.
	//
	EFUSE_RePgSection1(dev);

	//
	// 2008/12/26 MH Add to prevent ASPM bug.
	// 2008/12/27 MH Always enable hst and NIC clock request.
	//
	//if(!priv->isRFOff)//FIXME: once IPS enter,the isRFOff is set to true,but never set false, is it a bug?? amy 090331
		PHY_SwitchEphyParameter(dev);
	//PHY_EnableHostClkReq(Adapter);
	RF_RECOVERY(dev);

#ifdef TO_DO_LIST
	//
	// Restore all of the initialization or silent reset related count
	//
	pHalData->SilentResetRxSoltNum = 4;	// if the value is 4, driver detect rx stuck after 8 seconds without receiving packets
	pHalData->SilentResetRxSlotIndex = 0;
	for( i=0; i < MAX_SILENT_RESET_RX_SLOT_NUM; i++ )
	{
		pHalData->SilentResetRxStuckEvent[i] = 0;
	}

	Adapter->bIndicateCurrPhyStateAtInitialize = TRUE;
#endif

	{
		u8 tmpvalue;
		EFUSE_ShadowRead(dev, 1, 0x78, (u32 *)&tmpvalue);

		if(tmpvalue & BIT0)
		{
			priv->pwrdown = TRUE;}
		else
		{
			priv->pwrdown = FALSE;
		}
	}
	
	if(priv->BluetoothCoexist)
	{
		printk("Write reg 0x%x = 1 for Bluetooth Co-existance\n", SYSF_CFG);
		write_nic_byte(dev, SYSF_CFG, 0x1);
	}		
	
//	printk("==================>original RCR:%x, need to rewrite to fix aes bug:%x\n", read_nic_dword(dev, RCR), priv->ReceiveConfig);
//FIXME:YJ,090316
#if 1
	//write_nic_dword(dev, RCR, priv->ReceiveConfig);	//YJ,del,090320. priv->ReceiveConfig is set zero in rtl8192se_link_change after rtl8192_down, so when up again, RCR will be set to zero.
	// Execute RF check with last rounf resume!!!!
	rtl8192_irq_enable(dev);
#endif
end:
	priv->being_init_adapter = false;
	return rtStatus;

	
}

void rtl8192se_update_ratr_table(struct net_device* dev)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	u8* pMcsRate = ieee->dot11HTOperationalRateSet;
	//struct ieee80211_network *net = &ieee->current_network;
	u32 ratr_value = 0;
	u8 ratr_index = 0;
	u8 bNMode = 0;
	u16 shortGI_rate = 0;
	u32 tmp_ratr_value = 0;

	rtl8192_config_rate(dev, (u16*)(&ratr_value));
	ratr_value |= (*(u16*)(pMcsRate)) << 12;
	switch (ieee->mode)
	{
		case IEEE_A:
			ratr_value &= 0x00000FF0;
			break;
		case IEEE_B:
			ratr_value &= 0x0000000D;
			break;
		case IEEE_G:
			ratr_value &= 0x00000FF5;
			break;
		case IEEE_N_24G:
		case IEEE_N_5G:
			bNMode = 1;
			if (ieee->pHTInfo->PeerMimoPs == 0) //MIMO_PS_STATIC
				ratr_value &= 0x0007F005;
			else{
				if (priv->rf_type == RF_1T2R ||priv->rf_type  == RF_1T1R){
					if (ieee->pHTInfo->bCurTxBW40MHz)
						ratr_value &= 0x000FF015;
					else
						ratr_value &= 0x000ff005;
				}else{
					if (ieee->pHTInfo->bCurTxBW40MHz)
						ratr_value &= 0x0f0ff015;
					else
						ratr_value &= 0x0f0ff005;
					}
			}
			break;
		default:
			printk("====>%s(), mode is not correct:%x\n", __FUNCTION__, ieee->mode);
			break; 
	}
	if (priv->card_8192_version>= VERSION_8192S_BCUT)
		ratr_value &= 0x0FFFFFFF;
	else if (priv->card_8192_version == VERSION_8192S_ACUT)
		ratr_value &= 0x0FFFFFF0;	// Normal ACUT must disable CCK rate
	
	// Get MAX MCS available.
	if (bNMode && ((ieee->pHTInfo->bCurBW40MHz && ieee->pHTInfo->bCurShortGI40MHz) ||
	    (!ieee->pHTInfo->bCurBW40MHz && ieee->pHTInfo->bCurShortGI20MHz)))
	{
		ratr_value |= 0x10000000;  //Short GI
		tmp_ratr_value = (ratr_value>>12);
		for(shortGI_rate=15; shortGI_rate>0; shortGI_rate--)
		{
			if((1<<shortGI_rate) & tmp_ratr_value)
				break;
		}	
		shortGI_rate = (shortGI_rate<<12)|(shortGI_rate<<8)|\
			       (shortGI_rate<<4)|(shortGI_rate);
		write_nic_byte(dev, SG_RATE, shortGI_rate);
	}
	write_nic_dword(dev, ARFR0+ratr_index*4, ratr_value);
	printk("===========%s: %x\n", __FUNCTION__, read_nic_dword(dev, ARFR0));
	//2 UFWP
	if (ratr_value & 0xfffff000){
		printk("===>set to N mode\n");
		rtl8192se_set_fw_cmd(dev, FW_CMD_RA_REFRESH_N);
	}
	else{
		printk("===>set to B/G mode\n");
		rtl8192se_set_fw_cmd(dev, FW_CMD_RA_REFRESH_BG);
	}
}

void rtl8192se_net_update(struct net_device *dev)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
//	struct ieee80211_device* ieee = priv->ieee80211;
	struct ieee80211_network *net = &priv->ieee80211->current_network;
	//u16 BcnTimeCfg = 0, BcnCW = 6, BcnIFS = 0xf;
	u16 rate_config = 0;
	u32 regTmp = 0;
	u8 rateIndex = 0;
	//u8	retrylimit = 0x30;
	u16 cap = net->capability;

	priv->short_preamble = cap & WLAN_CAPABILITY_SHORT_PREAMBLE;
	
	//update Basic rate: RR, BRSR
	rtl8192_config_rate(dev, &rate_config);	
	if (priv->card_8192_version== VERSION_8192S_ACUT)
		priv->basic_rate = rate_config  = rate_config & 0x150;
	else if (priv->card_8192_version == VERSION_8192S_BCUT)
		priv->basic_rate= rate_config = rate_config & 0x15f;

#if 0	
	if (priv->short_preamble)
		regTmp |= BRSR_AckShortPmb;
#if RTL8192S_PREPARE_FOR_NORMAL_RELEASE
	regTmp |= rate_config;
	while(rate_config > 0x1)
	{
		rate_config = (rate_config >> 1);
		rateIndex++;
	}
	regTmp = ((regTmp<<8)|rateIndex);

	RT_TRACE(COMP_INIT, "HW_VAR_BASIC_RATE: Config 0x180(%#x)\n", regTmp);
	write_nic_dword(dev, INIRTSMCS_SEL, regTmp);
#endif
#else
			// Set RRSR rate table.
			write_nic_byte(dev, RRSR, rate_config&0xff);
			write_nic_byte(dev, RRSR+1, (rate_config>>8)&0xff);

			// Set RTS initial rate
			while(rate_config > 0x1)
			{
				rate_config = (rate_config>> 1);
				rateIndex++;
			}
			write_nic_byte(dev, INIRTSMCS_SEL, rateIndex);

			//set ack preample
			regTmp = (priv->nCur40MhzPrimeSC) << 5;
			if (priv->short_preamble)
				regTmp |= 0x80;
			write_nic_byte(dev, RRSR+2, regTmp);

#endif
	write_nic_dword(dev,BSSIDR,((u32*)net->bssid)[0]);
	write_nic_word(dev,BSSIDR+4,((u16*)net->bssid)[2]);

	write_nic_word(dev, BCN_INTERVAL, net->beacon_interval);
		//2008.10.24 added by tynli for beacon changed.
	PHY_SetBeaconHwReg( dev, net->beacon_interval);
	rtl8192_update_cap(dev, cap);

#if 0	
	if (ieee->iw_mode == IW_MODE_ADHOC){
		retrylimit = 7;
		//we should enable ibss interrupt here, but disable it temporarily
		if (0){
			priv->irq_mask |= (IMR_BcnInt | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);
			rtl8192_irq_disable(dev);
			rtl8192_irq_enable(dev);
		}
	}
	else{		
		if (0){
			priv->irq_mask &= ~(IMR_BcnInt | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);
			rtl8192_irq_disable(dev);
			rtl8192_irq_enable(dev);
		}
	}	
		
	priv->ShortRetryLimit = 
		priv->LongRetryLimit = retrylimit;
			
	write_nic_word(dev, 	RETRY_LIMIT, 
				retrylimit << RETRY_LIMIT_SHORT_SHIFT | \
				retrylimit << RETRY_LIMIT_LONG_SHIFT);	
#endif	
}

void rtl8192se_link_change(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	u32 reg = 0;
	reg = read_nic_dword(dev, RCR);
	if (ieee->state == IEEE80211_LINKED)
	{
		rtl8192se_net_update(dev);
		rtl8192se_update_ratr_table(dev);
		priv->ReceiveConfig = reg |= RCR_CBSSID;
	}
	else{
		priv->ReceiveConfig = reg &= ~RCR_CBSSID;
	}
	write_nic_dword(dev, RCR, reg);
	rtl8192_update_msr(dev);
	{		//// For 92SE test we must reset TST after setting MSR
		u32	temp = read_nic_dword(dev, TCR);
		write_nic_dword(dev, TCR, temp&(~BIT8));
		write_nic_dword(dev, TCR, temp|BIT8);
	}
	
}
void  rtl8192se_tx_fill_cmd_desc(struct net_device* dev, tx_desc_cmd * entry, cb_desc * cb_desc, struct sk_buff* skb)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
    dma_addr_t mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
    	// Clear all status	
	memset((void*)entry, 0, 32); 	
	
	// For firmware downlaod we only need to set LINIP
	entry->LINIP = cb_desc->bLastIniPkt;
	// From Scott --> 92SE must set as 1 for firmware download HW DMA error
	entry->FirstSeg = 1;//bFirstSeg;
	entry->LastSeg = 1;//bLastSeg;
	//pDesc->Offset	= 0x20;
	
	// 92SE need not to set TX packet size when firmware download
	entry->TxBufferSize= (u16)(skb->len);
	entry->TxBufferAddr = cpu_to_le32(mapping);
	entry->PktSize = (u16)(skb->len);
	
	//if (bSetOwnBit)
	{
		entry->OWN = 1;
	}
//	RT_DEBUG_DATA(COMP_ERR, entry, sizeof(*entry));
}

u8 QueryIsShort(u8 TxHT, u8 TxRate, cb_desc *tcb_desc)
{
	u8   tmp_Short;

	tmp_Short = (TxHT==1)?((tcb_desc->bUseShortGI)?1:0):((tcb_desc->bUseShortPreamble)?1:0);

	if(TxHT==1 && TxRate != DESC92S_RATEMCS15)
		tmp_Short = 0;

	return tmp_Short;
}

u8 MRateToHwRate8192SE(struct net_device*dev, u8 rate)
{
	u8	ret = DESC92S_RATE1M;
	u16	max_sg_rate;
	static	u16	multibss_sg_old = 0x1234;// Can not assign as zero for init value
	u16	multibss_sg;
		
	switch(rate)
	{
	case MGN_1M:	ret = DESC92S_RATE1M;		break;
	case MGN_2M:	ret = DESC92S_RATE2M;		break;
	case MGN_5_5M:	ret = DESC92S_RATE5_5M;	break;
	case MGN_11M:	ret = DESC92S_RATE11M;	break;
	case MGN_6M:	ret = DESC92S_RATE6M;		break;
	case MGN_9M:	ret = DESC92S_RATE9M;		break;
	case MGN_12M:	ret = DESC92S_RATE12M;	break;
	case MGN_18M:	ret = DESC92S_RATE18M;	break;
	case MGN_24M:	ret = DESC92S_RATE24M;	break;
	case MGN_36M:	ret = DESC92S_RATE36M;	break;
	case MGN_48M:	ret = DESC92S_RATE48M;	break;
	case MGN_54M:	ret = DESC92S_RATE54M;	break;

	// HT rate since here
	case MGN_MCS0:		ret = DESC92S_RATEMCS0;	break;
	case MGN_MCS1:		ret = DESC92S_RATEMCS1;	break;
	case MGN_MCS2:		ret = DESC92S_RATEMCS2;	break;
	case MGN_MCS3:		ret = DESC92S_RATEMCS3;	break;
	case MGN_MCS4:		ret = DESC92S_RATEMCS4;	break;
	case MGN_MCS5:		ret = DESC92S_RATEMCS5;	break;
	case MGN_MCS6:		ret = DESC92S_RATEMCS6;	break;
	case MGN_MCS7:		ret = DESC92S_RATEMCS7;	break;
	case MGN_MCS8:		ret = DESC92S_RATEMCS8;	break;
	case MGN_MCS9:		ret = DESC92S_RATEMCS9;	break;
	case MGN_MCS10:	ret = DESC92S_RATEMCS10;	break;
	case MGN_MCS11:	ret = DESC92S_RATEMCS11;	break;
	case MGN_MCS12:	ret = DESC92S_RATEMCS12;	break;
	case MGN_MCS13:	ret = DESC92S_RATEMCS13;	break;
	case MGN_MCS14:	ret = DESC92S_RATEMCS14;	break;
	case MGN_MCS15:	ret = DESC92S_RATEMCS15;	break;	

	// Set the highest SG rate
	case MGN_MCS0_SG:	
	case MGN_MCS1_SG:
	case MGN_MCS2_SG:	
	case MGN_MCS3_SG:	
	case MGN_MCS4_SG:	
	case MGN_MCS5_SG:
	case MGN_MCS6_SG:	
	case MGN_MCS7_SG:	
	case MGN_MCS8_SG:	
	case MGN_MCS9_SG:
	case MGN_MCS10_SG:
	case MGN_MCS11_SG:	
	case MGN_MCS12_SG:	
	case MGN_MCS13_SG:	
	case MGN_MCS14_SG:
	case MGN_MCS15_SG:	
			ret = DESC92S_RATEMCS15_SG;
			//#if(RTL8192S_PREPARE_FOR_NORMAL_RELEASE==0)
			max_sg_rate = rate&0xf;
			multibss_sg = max_sg_rate | (max_sg_rate<<4) | (max_sg_rate<<8) | (max_sg_rate<<12);
			if (multibss_sg_old != multibss_sg)
			{
				write_nic_dword(dev, SG_RATE, multibss_sg);
				multibss_sg_old = multibss_sg;
			}
			break;
			//#endif


	case (0x80|0x20): 	ret = DESC92S_RATEMCS32; break;

	default:				ret = DESC92S_RATEMCS15;	break;

	}

	return ret;
}
void  rtl8192se_tx_fill_desc(struct net_device* dev, tx_desc * pDesc, cb_desc * cb_desc, struct sk_buff* skb)
{
	u8				*pSeq;
	u16				Temp;
	struct r8192_priv* priv = ieee80211_priv(dev);
	dma_addr_t mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
	// Clear all status
	memset((void*)pDesc, 0, 32);		// 92SE Descriptor size to MAC

	// This part can just fill to the first descriptor of the frame.
//	if(bFirstSeg) //always we set no fragment, so disable here temporarily. WB. 2008.12.25
	{
		
		//pTxFwInfo->EnableCPUDur = pTcb->bTxEnableFwCalcDur;
		pDesc->TXHT		= (cb_desc->data_rate&0x80)?1:0;	

//#if (RTL92SE_FPGA_VERIFY == 0 && DISABLE_CCK == 1)
#if (RTL92SE_FPGA_VERIFY == 0)
		if (priv->card_8192_version== VERSION_8192S_ACUT)
		{
			if (cb_desc->data_rate== MGN_1M || cb_desc->data_rate == MGN_2M || 
				cb_desc->data_rate == MGN_5_5M || cb_desc->data_rate == MGN_11M)
			{
				cb_desc->data_rate = MGN_12M;
			}
		}
#endif			
		pDesc->TxRate	= MRateToHwRate8192SE(dev,cb_desc->data_rate);
		pDesc->TxShort	= QueryIsShort(((cb_desc->data_rate&0x80)?1:0), MRateToHwRate8192SE(dev,cb_desc->data_rate), cb_desc);
		//SET_TX_DESC_TXHT(pDesc,  (pTcb->DataRate&0x80)?1:0);		
		//SET_TX_DESC_TX_RATE(pDesc, MRateToHwRate8192SE(Adapter, (u8)pTcb->DataRate));
		//SET_TX_DESC_TX_SHORT(pDesc,  QueryIsShort(Adapter, ((pTcb->DataRate&0x80)?1:0),  MRateToHwRate8192SE(Adapter, (u8)pTcb->DataRate), pTcb));

		
		//
		// Aggregation related
		//
		if(cb_desc->bAMPDUEnable)
		{			
			pDesc->AggEn = 1;
			//SET_TX_DESC_AGG_ENABLE(pDesc, 1);			
		}
		else
		{			
			pDesc->AggEn = 0;
			//SET_TX_DESC_AGG_ENABLE(pDesc, 0);
		}

		//pTxFwInfo->RxMF = pTcb->AMPDUFactor;
		//pTxFwInfo->RxAMD = pTcb->AMPDUDensity;
		
		// For AMPDU, we must insert SSN into TX_DESC
		pSeq = (u8 *)(skb->data+22);
		Temp = pSeq[0];
		Temp <<= 12;			
		Temp |= (*(u16 *)pSeq)>>4;
		pDesc->Seq = Temp;
		
		
		//
		// Protection mode related
		//
		// For 92S, if RTS/CTS are set, HW will execute RTS.
		// We choose only one protection mode to execute		
		//pDesc->RTSEn	= (pTcb->bRTSEnable)?1:0;				
		pDesc->RTSEn	= (cb_desc->bRTSEnable && cb_desc->bCTSEnable==FALSE)?1:0;				
		pDesc->CTS2Self	= (cb_desc->bCTSEnable)?1:0;		
		pDesc->RTSSTBC	= (cb_desc->bRTSSTBC)?1:0;
		pDesc->RTSHT	= (cb_desc->rts_rate&0x80)?1:0;

//#if (RTL92SE_FPGA_VERIFY == 0 && DISABLE_CCK == 1)
#if (RTL92SE_FPGA_VERIFY == 0)
		if (priv->card_8192_version== VERSION_8192S_ACUT)
		{
			if (cb_desc->rts_rate == MGN_1M || cb_desc->rts_rate == MGN_2M || 
				cb_desc->rts_rate == MGN_5_5M || cb_desc->rts_rate == MGN_11M)
			{
				cb_desc->rts_rate = MGN_12M;
			}
		}
#endif	
		pDesc->RTSRate	= MRateToHwRate8192SE(dev,cb_desc->rts_rate);
		pDesc->RTSRate	= MRateToHwRate8192SE(dev,MGN_24M);
		pDesc->RTSBW	= 0;
		pDesc->RTSSC	= cb_desc->RTSSC;
		pDesc->RTSShort	= (pDesc->RTSHT==0)?(cb_desc->bRTSUseShortPreamble?1:0):(cb_desc->bRTSUseShortGI?1:0);
		//SET_TX_DESC_RTS_ENABLE(pDesc, (pTcb->bRTSEnable)?1:0);		
		//SET_TX_DESC_CTS_ENABLE(pDesc, (pTcb->bCTSEnable)?1:0);
		//SET_TX_DESC_RTS_STBC(pDesc, (pTcb->bRTSSTBC)?1:0);
		//SET_TX_DESC_RTS_HT(pDesc, (pTcb->RTSRate&0x80)?1:0);
		//SET_TX_DESC_RTS_RATE(pDesc, MRateToHwRate8192SE(Adapter, (u8)pTcb->RTSRate));
		//SET_TX_DESC_RTS_SUB_CARRIER(pDesc, (!(pTcb->RTSRate&0x80))?(pTcb->RTSSC):0);
		//SET_TX_DESC_RTS_BANDWIDTH(pDesc, (pTcb->RTSRate&0x80)?((pTcb->bRTSBW)?1:0):0);
		//SET_TX_DESC_RTS_SHORT(pDesc, (!(pTcb->RTSRate&0x80))?(pTcb->bRTSUseShortPreamble?1:0):(pTcb->bRTSUseShortGI?1:0)); 					
	

		//
		// Set Bandwidth and sub-channel settings.
		//		
		if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
		{
			if(cb_desc->bPacketBW)
			{
				pDesc->TxBw		= 1;
				pDesc->TXSC		= 0;
				//SET_TX_DESC_TX_BANDWIDTH(pDesc, 1);
				//SET_TX_DESC_TX_SUB_CARRIER(pDesc, 0);//By SD3's Jerry suggestion, use duplicated mode
			}
			else
			{				
				pDesc->TxBw		= 0;
				pDesc->TXSC		= priv->nCur40MhzPrimeSC;
				//SET_TX_DESC_TX_BANDWIDTH(pDesc, 0);
				//SET_TX_DESC_TX_SUB_CARRIER(pDesc, pHalData->nCur40MhzPrimeSC);
			}
		}
		else
		{
			pDesc->TxBw		= 0;
			pDesc->TXSC		= 0;
			//SET_TX_DESC_TX_BANDWIDTH(pDesc, 0);
			//SET_TX_DESC_TX_SUB_CARRIER(pDesc, 0);		
		}

		//3(3)Fill necessary field in First Descriptor
		/*DWORD 0*/		
		pDesc->LINIP = 0;//bLastInitPacket;		
		pDesc->Offset = 32;//USB_HWDESC_HEADER_LEN;
		pDesc->PktSize = (u16)skb->len;		
		//SET_TX_DESC_LINIP(pDesc, 0);		
		//SET_TX_DESC_OFFSET(pDesc, USB_HWDESC_HEADER_LEN);			
		//SET_TX_DESC_PKT_SIZE(pDesc, (u16)(PktLen));
		//pDesc->CmdInit = 1;		
		
		/*DWORD 1*/
		//pDesc->SecCAMID= 0;
		//pDesc->RATid = pTcb->RATRIndex;
		pDesc->RaBRSRID = cb_desc->RATRIndex;
		//SET_TX_DESC_RA_BRSR_ID(pDesc, pTcb->RATRIndex);
		//SET_TX_DESC_RA_BRSR_ID(pDesc, 1);
		
		//
		// Fill security related
		//
		 if (cb_desc->bHwSec) {
       		 static u8 tmp =0;
       		 if (!tmp) {
            		 tmp = 1;
	        	}
	       	 switch (priv->ieee80211->pairwise_key_type) {
	           		 case KEY_TYPE_WEP40:
	           		 case KEY_TYPE_WEP104:
	               	 	pDesc->SecType = 0x1;
	                	 	break;
	            		case KEY_TYPE_TKIP:
	                		pDesc->SecType = 0x2;
	                		break;
		            case KEY_TYPE_CCMP:
		               	 pDesc->SecType = 0x3;
		               	 break;
		            case KEY_TYPE_NA:
		                	pDesc->SecType = 0x0;
		                	break;
	       	 }
   		 }


		//
		// Set Packet ID
		//		
		pDesc->PktID			= 0x0;		
		// 2008/11/07 MH For SD4 requirement !!!!!
		// We will assign magement queue to BK.
		//if (IsMgntFrame(VirtualAddress))
			//QueueIndex = BK_QUEUE;
		
		pDesc->QueueSel		= MapHwQueueToFirmwareQueue(cb_desc->queue_index); 
		//SET_TX_DESC_QUEUE_SEL(pDesc,  MapHwQueueToFirmwareQueue(QueueIndex));
		
		pDesc->DataRateFBLmt	= 0x1F;		// Alwasy enable all rate fallback range
		pDesc->DISFB	= cb_desc->bTxDisableRateFallBack;
		pDesc->UserRate	= cb_desc->bTxUseDriverAssingedRate;
		//SET_TX_DESC_DISABLE_FB(pDesc, pTcb->bTxDisableRateFallBack);		
		//SET_TX_DESC_USER_RATE(pDesc, pTcb->bTxUseDriverAssingedRate);		

		//
		// 2008/09/25 MH We must change TX gain table for 1T or 2T. If we set force rate
		// We must change gain table by driver. Otherwise, firmware will handle.
		//
		if (pDesc->UserRate == true && pDesc->TXHT == true)		
			RF_ChangeTxPath(dev, cb_desc->data_rate);

	}
	

	//1 Fill fields that are required to be initialized in all of the descriptors
	//DWORD 0
	pDesc->FirstSeg	= 1;//(bFirstSeg)? 1:0;	
	pDesc->LastSeg	= 1;//(bLastSeg)?1:0;			
	// For test only 	
	//SET_TX_DESC_FIRST_SEG(pDesc, (bFirstSeg? 1:0));
	//SET_TX_DESC_LAST_SEG(pDesc, (bLastSeg? 1:0));
	
	//DWORD 7	
	pDesc->TxBufferSize= (u16)skb->len;	
	//SET_TX_DESC_TX_BUFFER_SIZE(pDesc, (u16) BufferLen);	
	
	//DOWRD 8
	pDesc->TxBuffAddr = cpu_to_le32(mapping);

}

u8 HwRateToMRate92S(bool	bIsHT,	u8	rate	)
{
	u8	ret_rate = 0x02;

	if(!bIsHT)
	{
		switch(rate)
		{
		
			case DESC92S_RATE1M:		ret_rate = MGN_1M;		break;
			case DESC92S_RATE2M:		ret_rate = MGN_2M;		break;
			case DESC92S_RATE5_5M:		ret_rate = MGN_5_5M;		break;
			case DESC92S_RATE11M:		ret_rate = MGN_11M;		break;
			case DESC92S_RATE6M:		ret_rate = MGN_6M;		break;
			case DESC92S_RATE9M:		ret_rate = MGN_9M;		break;
			case DESC92S_RATE12M:		ret_rate = MGN_12M;		break;
			case DESC92S_RATE18M:		ret_rate = MGN_18M;		break;
			case DESC92S_RATE24M:		ret_rate = MGN_24M;		break;
			case DESC92S_RATE36M:		ret_rate = MGN_36M;		break;
			case DESC92S_RATE48M:		ret_rate = MGN_48M;		break;
			case DESC92S_RATE54M:		ret_rate = MGN_54M;		break;
		
			default:							
				ret_rate = 0xff;
					break;
		}

	}
	else{
		switch(rate)
		{
		

			case DESC92S_RATEMCS0:	ret_rate = MGN_MCS0;		break;
			case DESC92S_RATEMCS1:	ret_rate = MGN_MCS1;		break;
			case DESC92S_RATEMCS2:	ret_rate = MGN_MCS2;		break;
			case DESC92S_RATEMCS3:	ret_rate = MGN_MCS3;		break;
			case DESC92S_RATEMCS4:	ret_rate = MGN_MCS4;		break;
			case DESC92S_RATEMCS5:	ret_rate = MGN_MCS5;		break;
			case DESC92S_RATEMCS6:	ret_rate = MGN_MCS6;		break;
			case DESC92S_RATEMCS7:	ret_rate = MGN_MCS7;		break;
			case DESC92S_RATEMCS8:	ret_rate = MGN_MCS8;		break;
			case DESC92S_RATEMCS9:	ret_rate = MGN_MCS9;		break;
			case DESC92S_RATEMCS10:	ret_rate = MGN_MCS10;	break;
			case DESC92S_RATEMCS11:	ret_rate = MGN_MCS11;	break;
			case DESC92S_RATEMCS12:	ret_rate = MGN_MCS12;	break;
			case DESC92S_RATEMCS13:	ret_rate = MGN_MCS13;	break;
			case DESC92S_RATEMCS14:	ret_rate = MGN_MCS14;	break;
			case DESC92S_RATEMCS15:	ret_rate = MGN_MCS15;	break;
			case DESC92S_RATEMCS32:	ret_rate = (0x80|0x20);	break;


			default:							
				ret_rate = 0xff;
				break;
		}

	}	
	return ret_rate;
}


bool rtl8192se_rx_query_status_desc(struct net_device* dev, struct ieee80211_rx_stats*  stats, rx_desc *pdesc, struct sk_buff* skb)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	//u32	Length		= pdesc->Length;
	//u32	TID			= pdesc->TID;
	u32	PHYStatus	= pdesc->PHYStatus;
	rx_fwinfo*		pDrvInfo;
	stats->Length 		= (u16)pdesc->Length;	
	stats->RxDrvInfoSize = (u8)pdesc->DrvInfoSize*8;
	stats->RxBufShift 	= (u8)((pdesc->Shift)&0x03);      
	stats->bICV 			= (u16)pdesc->ICVError;
	stats->bCRC 			= (u16)pdesc->CRC32;
	stats->bHwError 		= (u16)(pdesc->CRC32|pdesc->ICVError);	
	stats->Decrypted 	= !pdesc->SWDec;
	// Debug !!!
	stats->rate = (u8)pdesc->RxMCS;
	stats->bShortPreamble= (u16)pdesc->SPLCP;
	stats->bIsAMPDU 		= (bool)(pdesc->PAGGR==1);
	stats->bFirstMPDU 	= (bool)((pdesc->PAGGR==1) && (pdesc->FAGGR==1));
	stats->TimeStampLow 	= pdesc->TSFL;
	stats->RxIs40MHzPacket= (bool)pdesc->BandWidth;
	if(IS_UNDER_11N_AES_MODE(ieee))
	{
		// Always received ICV error packets in AES mode.
		// This fixed HW later MIC write bug.
	//	if (stats->bICV)
	//		printk("received icv error packet:%d, %d\n", stats->bICV, stats->bCRC);
		if(stats->bICV && !stats->bCRC)
		{
			stats->bICV = FALSE;
			stats->bHwError = FALSE;
		}
	}

	
	// 2008/09/30 MH For Marvel/Atheros/Broadcom AMPDU RX, we may receive wrong
	// beacon packet length, we do test here!!!!
	if(stats->Length > 0x2000 || stats->Length < 24)
	{
		RT_TRACE(COMP_ERR, "Err RX pkt len = %x\n", stats->Length);
		stats->bHwError |= 1;
	}
	// Collect rx rate before it transform to MRate
	UpdateReceivedRateHistogramStatistics8190(dev, stats);

	// Transform HwRate to MRate
	if(!stats->bHwError)
		stats->rate = HwRateToMRate92S((bool)(pdesc->RxHT), (u8)(pdesc->RxMCS));
	else
	{
		stats->rate = MGN_1M;
		return false;
	}
	//
	// Be care the below code will influence TP. We should not enable the code if
	// it is useless.
	//	

	//
	// 3. Debug only, collect rx rate/ AMPDU/ TSFL
	//
	//UpdateRxdRateHistogramStatistics8192S(Adapter, pRfd);
//	UpdateRxAMPDUHistogramStatistics8192S(Adapter, stats);	
	UpdateRxPktTimeStamp8190(dev, stats);	

	//
	// Get Total offset of MPDU Frame Body 
	//
	if((stats->RxBufShift + stats->RxDrvInfoSize) > 0)
		stats->bShift = 1;	

	//
	// 3-4. Get Driver Info
	// Driver info are written to the begining of the RxBuffer, it will be 
	// attached after RX DESC(24 or 32 bytes). PHY status 32 bytes and other 
	// variable info size.!!
	//
	//RTPRINT(FRX, RX_PHY_STS, ("pDesc->PHYStatus = %d\n", PHYStatus));
	if (PHYStatus == true)
	{
		// 92SE PHY status is appended before real data frame.!!
		pDrvInfo = (rx_fwinfo*)(skb->data + stats->RxBufShift);
			
		//
		// 4. Get signal quality for this packet, and record aggregation info for 
		//    next packet
		//
		TranslateRxSignalStuff819xpci(dev, skb, stats, pdesc, pDrvInfo); //

	}
	return true;	
}

void rtl8192se_rtx_disable(struct net_device *dev, bool bReset)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));
        int i;
//	u8	OpMode;
	u8	u1bTmp;
	//u16	u2bTmp;
//	u32	ulRegRead;

	RT_TRACE(COMP_INIT, "==> rtl8192se_rtx_disable()\n");
	
	priv->ieee80211->state = IEEE80211_NOLINK;
	rtl8192_update_msr(dev);

	// 
	// Disalbe adapter sequence. Designed/tested from SD4 Scott, SD1 Grent and Jonbon.
	// Added by Bruce, 2008-11-22.
	//
	//==================================================================
	// (1) Switching Power Supply Register : Disable LD12 & SW12 (for IT)
	u1bTmp = read_nic_byte(dev, LDOV12D_CTRL);
	u1bTmp |= BIT0;
	write_nic_byte(dev, LDOV12D_CTRL, u1bTmp);
	write_nic_byte(dev, SPS1_CTRL, 0x0);
#if 0
	// (2) Attach all circuits(MACTOP, BB, PCIE, AFE PLL, RF)
	write_nic_byte(dev, SYS_ISO_CTRL, 0xA6);

	// (3) MAC Tx/Rx enable, BB enable, CCK/OFDM enable
	write_nic_word(dev, CMDR, 0x77FC);

	// (4) Do nothing for reg SYS_CLKR

	// (5) Sleep exit control path switch to S/W, Load done control path switch to S/W
	// 2008/10/29 MH For 92SE S3/S4 test.
	u2bTmp = read_nic_word(dev, SYS_CLKR);
	if (u2bTmp & BIT15)	// If not FW control, we may fail to return from HW SW to HW
	{
		write_nic_byte(dev, SYS_CLKR+1, 0x38);
	}
	// 2008/12/10 MH Prevent NIC IO hang!!!
	udelay(200);

	// (6) MAC IO REG disable, Disable Core digital (MATTOP POR), Disable CPU Core digital (CPU RST)
	write_nic_byte(dev, (SYS_FUNC_EN + 1), 0x71);

	// (7) Isolate Extra Digital I/O, Isolate RF to MATTOP/BB/PCIE Digital
	write_nic_byte(dev, SYS_ISO_CTRL, 0xB6);
		
	// (8) disable APE PLL
	write_nic_byte(dev, AFE_PLL_CTRL, 0x00);

	// (9) disable XTAL
	write_nic_byte(dev, AFE_XTAL_CTRL, 0x0E);
	
	// (10) Disable LDOA15 Macro Block
	write_nic_byte(dev, LDOA15_CTRL, 0x54);

	// (11) Reset Efuse loader
	//EFUSE_ResetLoader(dev);
	
	// For debug ..
	//DBG_FreeTCBs(dev);
#else
        write_nic_byte(dev, TXPAUSE, 0xFF);
	write_nic_word(dev, CMDR, 0x57FC);
	udelay(100);
	write_nic_word(dev, CMDR, 0x77FC);
	write_nic_byte(dev, PHY_CCA, 0x0);
	udelay(10);
	write_nic_word(dev, CMDR, 0x37FC);
	udelay(10);
	write_nic_word(dev, CMDR, 0x77FC);			
	udelay(10);
	write_nic_word(dev, CMDR, 0x57FC);
	write_nic_word(dev, CMDR, 0x0000);
	u1bTmp = read_nic_byte(dev, (SYS_CLKR + 1));
	
	/*  03/27/2009 MH Add description. After switch control path. register
	    after page1 will be invisible. We can not do any IO for register>0x40.
	    After resume&MACIO reset, we need to remember previous reg content. */
	if(u1bTmp & BIT7)
	{
		u1bTmp &= ~(BIT6 | BIT7);					
		if(!HalSetSysClk8192SE(dev, u1bTmp))
		{
			printk("Switch ctrl path fail\n");
			return;
		}
	}
	
	printk("Save MAC\n");
	/*  03/27/2009 MH Add description. Power save for MAC */
	if(priv->ieee80211->RfOffReason==RF_CHANGE_BY_IPS )
	{
		/*  03/27/2009 MH For enable LED function. */
		printk("Save except led\n");
		write_nic_byte(dev, 0x03, 0xF9);
	}
	else		// SW/HW radio off or halt adapter!! For example S3/S4
	{
		// 2009/03/27 MH LED function disable. Power range is about 8mA now.
		printk("Save max pwr\n");
		write_nic_byte(dev, 0x03, 0x71);
	}
	write_nic_byte(dev, 0x09, 0x70);
	write_nic_byte(dev, 0x29, 0x68);
	write_nic_byte(dev, 0x28, 0x00);
	write_nic_byte(dev, 0x20, 0x50);
	write_nic_byte(dev, 0x26, 0x0E);
	RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
	udelay(100);
#endif	
	udelay(20);
	if(!bReset)
	{
		mdelay(20);
#if 0
#if ((DEV_BUS_TYPE == PCI_INTERFACE) && (HAL_CODE_BASE == RTL8192))
		pHalData->bHwRfOffAction = 2;
#endif
#endif
	}	
	
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->ieee80211->skb_waitQ [i]);
        }
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->ieee80211->skb_aggQ [i]);
        }

	skb_queue_purge(&priv->skb_queue);
	RT_TRACE(COMP_INIT, "<== HaltAdapter8192SE()\n");
	return;
}
#else //RTL8192E and 90P
static void rtl8192_read_eeprom_info(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	u8			tempval;
#ifdef RTL8192E
	u8			ICVer8192, ICVer8256;
#endif
	u16			i,usValue, IC_Version;
	u16			EEPROMId;
#ifdef RTL8190P
   	u8			offset;//, tmpAFR;
    	u8      		EepromTxPower[100];
#endif
	u8 bMac_Tmp_Addr[6] = {0x00, 0xe0, 0x4c, 0x00, 0x00, 0x01};
	RT_TRACE(COMP_INIT, "====> rtl8192_read_eeprom_info\n");

	  
	// TODO: I don't know if we need to apply EF function to EEPROM read function
  
	//2 Read EEPROM ID to make sure autoload is success
	EEPROMId = eprom_read(dev, 0);
	if( EEPROMId != RTL8190_EEPROM_ID )
	{
		RT_TRACE(COMP_ERR, "EEPROM ID is invalid:%x, %x\n", EEPROMId, RTL8190_EEPROM_ID); 
		priv->AutoloadFailFlag=true;
	}
	else
	{
		priv->AutoloadFailFlag=false;
	}
	
	//
	// Assign Chip Version ID
	//
	// Read IC Version && Channel Plan
	if(!priv->AutoloadFailFlag)
	{
		// VID, PID	 
		priv->eeprom_vid = eprom_read(dev, (EEPROM_VID >> 1));
		priv->eeprom_did = eprom_read(dev, (EEPROM_DID >> 1));

		usValue = eprom_read(dev, (u16)(EEPROM_Customer_ID>>1)) >> 8 ;
		priv->eeprom_CustomerID = (u8)( usValue & 0xff);	
		usValue = eprom_read(dev, (EEPROM_ICVersion_ChannelPlan>>1));
		priv->eeprom_ChannelPlan = usValue&0xff;
		IC_Version = ((usValue&0xff00)>>8);

#ifdef RTL8190P
		priv->card_8192_version = (VERSION_8190)(IC_Version);
#else
	#ifdef RTL8192E
		ICVer8192 = (IC_Version&0xf);		//bit0~3; 1:A cut, 2:B cut, 3:C cut...
		ICVer8256 = ((IC_Version&0xf0)>>4);//bit4~6, bit7 reserved for other RF chip; 1:A cut, 2:B cut, 3:C cut...
		RT_TRACE(COMP_INIT, "\nICVer8192 = 0x%x\n", ICVer8192);
		RT_TRACE(COMP_INIT, "\nICVer8256 = 0x%x\n", ICVer8256);
		if(ICVer8192 == 0x2)	//B-cut
		{
			if(ICVer8256 == 0x5) //E-cut
				priv->card_8192_version= VERSION_8190_BE;
		}
	#endif
#endif
		switch(priv->card_8192_version)
		{
			case VERSION_8190_BD:
			case VERSION_8190_BE:
				break;
			default:
				priv->card_8192_version = VERSION_8190_BD;
				break;
		}
		RT_TRACE(COMP_INIT, "\nIC Version = 0x%x\n", priv->card_8192_version);
	}
	else
	{
		priv->card_8192_version = VERSION_8190_BD;
		priv->eeprom_vid = 0;
		priv->eeprom_did = 0;
		priv->eeprom_CustomerID = 0;
		priv->eeprom_ChannelPlan = 0;
		RT_TRACE(COMP_INIT, "\nIC Version = 0x%x\n", 0xff);
	}
	
	RT_TRACE(COMP_INIT, "EEPROM VID = 0x%4x\n", priv->eeprom_vid);
	RT_TRACE(COMP_INIT, "EEPROM DID = 0x%4x\n", priv->eeprom_did);
	RT_TRACE(COMP_INIT,"EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);
	
	//2 Read Permanent MAC address
	if(!priv->AutoloadFailFlag)
	{
		for(i = 0; i < 6; i += 2)
		{
			usValue = eprom_read(dev, (u16) ((EEPROM_NODE_ADDRESS_BYTE_0+i)>>1));
			*(u16*)(&dev->dev_addr[i]) = usValue;
		}
	} else {
		// when auto load failed,  the last address byte set to be a random one.
		// added by david woo.2007/11/7
		memcpy(dev->dev_addr, bMac_Tmp_Addr, 6);		
		#if 0
		for(i = 0; i < 6; i++)
		{
			dev->PermanentAddress[i] = sMacAddr[i];
			write_nic_byte(dev, IDR0+i, sMacAddr[i]);
		}
		#endif
	}

	RT_TRACE(COMP_INIT, "Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
			dev->dev_addr[0], dev->dev_addr[1], 
			dev->dev_addr[2], dev->dev_addr[3], 
			dev->dev_addr[4], dev->dev_addr[5]);  
	
		//2 TX Power Check EEPROM Fail or not
	if(priv->card_8192_version > VERSION_8190_BD) {
		priv->bTXPowerDataReadFromEEPORM = true;
	} else {
		priv->bTXPowerDataReadFromEEPORM = false;
	}

	// 2007/11/15 MH 8190PCI Default=2T4R, 8192PCIE dafault=1T2R
	priv->rf_type = RTL819X_DEFAULT_RF_TYPE;

	if(priv->card_8192_version > VERSION_8190_BD)
	{
		// Read RF-indication and Tx Power gain index diff of legacy to HT OFDM rate.
		if(!priv->AutoloadFailFlag)
		{
			tempval = (eprom_read(dev, (EEPROM_RFInd_PowerDiff>>1))) & 0xff;
			priv->EEPROMLegacyHTTxPowerDiff = tempval & 0xf;	// bit[3:0]

			if (tempval&0x80)	//RF-indication, bit[7]
				priv->rf_type = RF_1T2R;
			else
				priv->rf_type = RF_2T4R;
		}
		else
		{
			priv->EEPROMLegacyHTTxPowerDiff = 0x04;//EEPROM_Default_LegacyHTTxPowerDiff;
		}
		RT_TRACE(COMP_INIT, "EEPROMLegacyHTTxPowerDiff = %d\n", 
			priv->EEPROMLegacyHTTxPowerDiff);
		
		// Read ThermalMeter from EEPROM
		if(!priv->AutoloadFailFlag)
		{
			priv->EEPROMThermalMeter = (u8)(((eprom_read(dev, (EEPROM_ThermalMeter>>1))) & 0xff00)>>8);
		}
		else
		{
			priv->EEPROMThermalMeter = EEPROM_Default_ThermalMeter;
		}
		RT_TRACE(COMP_INIT, "ThermalMeter = %d\n", priv->EEPROMThermalMeter);
		//vivi, for tx power track
		priv->TSSI_13dBm = priv->EEPROMThermalMeter *100;
		
		if(priv->epromtype == EEPROM_93C46)
		{
		// Read antenna tx power offset of B/C/D to A and CrystalCap from EEPROM
		if(!priv->AutoloadFailFlag)
		{
				usValue = eprom_read(dev, (EEPROM_TxPwDiff_CrystalCap>>1));
				priv->EEPROMAntPwDiff = (usValue&0x0fff);
				priv->EEPROMCrystalCap = (u8)((usValue&0xf000)>>12);
		}
		else
		{
				priv->EEPROMAntPwDiff = EEPROM_Default_AntTxPowerDiff;
				priv->EEPROMCrystalCap = EEPROM_Default_TxPwDiff_CrystalCap;
		}
			RT_TRACE(COMP_INIT, "EEPROMAntPwDiff = %d\n", priv->EEPROMAntPwDiff);
			RT_TRACE(COMP_INIT, "EEPROMCrystalCap = %d\n", priv->EEPROMCrystalCap);
		
		//
		// Get per-channel Tx Power Level
		//
		for(i=0; i<14; i+=2)
		{
			if(!priv->AutoloadFailFlag)
			{
				usValue = eprom_read(dev, (u16) ((EEPROM_TxPwIndex_CCK+i)>>1) );
			}
			else
			{
				usValue = EEPROM_Default_TxPower;
			}
			*((u16*)(&priv->EEPROMTxPowerLevelCCK[i])) = usValue;
			RT_TRACE(COMP_INIT,"CCK Tx Power Level, Index %d = 0x%02x\n", i, priv->EEPROMTxPowerLevelCCK[i]);
			RT_TRACE(COMP_INIT, "CCK Tx Power Level, Index %d = 0x%02x\n", i+1, priv->EEPROMTxPowerLevelCCK[i+1]);
		}
		for(i=0; i<14; i+=2)
		{
			if(!priv->AutoloadFailFlag)
			{
				usValue = eprom_read(dev, (u16) ((EEPROM_TxPwIndex_OFDM_24G+i)>>1) );
			}
			else
			{
				usValue = EEPROM_Default_TxPower;
			}
			*((u16*)(&priv->EEPROMTxPowerLevelOFDM24G[i])) = usValue;
			RT_TRACE(COMP_INIT, "OFDM 2.4G Tx Power Level, Index %d = 0x%02x\n", i, priv->EEPROMTxPowerLevelOFDM24G[i]);
			RT_TRACE(COMP_INIT, "OFDM 2.4G Tx Power Level, Index %d = 0x%02x\n", i+1, priv->EEPROMTxPowerLevelOFDM24G[i+1]);
		}
		}
		else if(priv->epromtype== EEPROM_93C56)
		{
		#ifdef RTL8190P
			// Read CrystalCap from EEPROM
			if(!priv->AutoloadFailFlag)
			{
				priv->EEPROMAntPwDiff = EEPROM_Default_AntTxPowerDiff;
				priv->EEPROMCrystalCap = (u8)(((eprom_read(dev, (EEPROM_C56_CrystalCap>>1))) & 0xf000)>>12);
			}
			else
			{
				priv->EEPROMAntPwDiff = EEPROM_Default_AntTxPowerDiff;
				priv->EEPROMCrystalCap = EEPROM_Default_TxPwDiff_CrystalCap;
			}
			RT_TRACE(COMP_INIT,"EEPROMAntPwDiff = %d\n", priv->EEPROMAntPwDiff);
			RT_TRACE(COMP_INIT, "EEPROMCrystalCap = %d\n", priv->EEPROMCrystalCap);

			// Get Tx Power Level by Channel
			if(!priv->AutoloadFailFlag)
			{
				    // Read Tx power of Channel 1 ~ 14 from EEPROM.
			       for(i = 0; i < 12; i+=2)
				{
					if (i <6)
						offset = EEPROM_C56_RfA_CCK_Chnl1_TxPwIndex + i;
					else
						offset = EEPROM_C56_RfC_CCK_Chnl1_TxPwIndex + i - 6;
					usValue = eprom_read(dev, (offset>>1));
				       *((u16*)(&EepromTxPower[i])) = usValue;
				}

			       for(i = 0; i < 12; i++)
			       	{
			       		if (i <= 2)
						priv->EEPROMRfACCKChnl1TxPwLevel[i] = EepromTxPower[i];
					else if ((i >=3 )&&(i <= 5))
						priv->EEPROMRfAOfdmChnlTxPwLevel[i-3] = EepromTxPower[i];
					else if ((i >=6 )&&(i <= 8))
						priv->EEPROMRfCCCKChnl1TxPwLevel[i-6] = EepromTxPower[i];
					else
						priv->EEPROMRfCOfdmChnlTxPwLevel[i-9] = EepromTxPower[i];						
				}
			}
			else
			{
				priv->EEPROMRfACCKChnl1TxPwLevel[0] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfACCKChnl1TxPwLevel[1] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfACCKChnl1TxPwLevel[2] = EEPROM_Default_TxPowerLevel;

				priv->EEPROMRfAOfdmChnlTxPwLevel[0] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfAOfdmChnlTxPwLevel[1] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfAOfdmChnlTxPwLevel[2] = EEPROM_Default_TxPowerLevel;
				
				priv->EEPROMRfCCCKChnl1TxPwLevel[0] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfCCCKChnl1TxPwLevel[1] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfCCCKChnl1TxPwLevel[2] = EEPROM_Default_TxPowerLevel;

				priv->EEPROMRfCOfdmChnlTxPwLevel[0] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfCOfdmChnlTxPwLevel[1] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMRfCOfdmChnlTxPwLevel[2] = EEPROM_Default_TxPowerLevel;
			}
			RT_TRACE(COMP_INIT, "priv->EEPROMRfACCKChnl1TxPwLevel[0] = 0x%x\n", priv->EEPROMRfACCKChnl1TxPwLevel[0]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfACCKChnl1TxPwLevel[1] = 0x%x\n", priv->EEPROMRfACCKChnl1TxPwLevel[1]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfACCKChnl1TxPwLevel[2] = 0x%x\n", priv->EEPROMRfACCKChnl1TxPwLevel[2]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfAOfdmChnlTxPwLevel[0] = 0x%x\n", priv->EEPROMRfAOfdmChnlTxPwLevel[0]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfAOfdmChnlTxPwLevel[1] = 0x%x\n", priv->EEPROMRfAOfdmChnlTxPwLevel[1]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfAOfdmChnlTxPwLevel[2] = 0x%x\n", priv->EEPROMRfAOfdmChnlTxPwLevel[2]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCCCKChnl1TxPwLevel[0] = 0x%x\n", priv->EEPROMRfCCCKChnl1TxPwLevel[0]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCCCKChnl1TxPwLevel[1] = 0x%x\n", priv->EEPROMRfCCCKChnl1TxPwLevel[1]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCCCKChnl1TxPwLevel[2] = 0x%x\n", priv->EEPROMRfCCCKChnl1TxPwLevel[2]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCOfdmChnlTxPwLevel[0] = 0x%x\n", priv->EEPROMRfCOfdmChnlTxPwLevel[0]);			
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCOfdmChnlTxPwLevel[1] = 0x%x\n", priv->EEPROMRfCOfdmChnlTxPwLevel[1]);
			RT_TRACE(COMP_INIT, "priv->EEPROMRfCOfdmChnlTxPwLevel[2] = 0x%x\n", priv->EEPROMRfCOfdmChnlTxPwLevel[2]);
#endif

		}
		//
		// Update HAL variables.
		//
		if(priv->epromtype == EEPROM_93C46)
		{
			for(i=0; i<14; i++)
			{
				priv->TxPowerLevelCCK[i] = priv->EEPROMTxPowerLevelCCK[i];
				priv->TxPowerLevelOFDM24G[i] = priv->EEPROMTxPowerLevelOFDM24G[i];
			}
			priv->LegacyHTTxPowerDiff = priv->EEPROMLegacyHTTxPowerDiff;
		// Antenna B gain offset to antenna A, bit0~3
			priv->AntennaTxPwDiff[0] = (priv->EEPROMAntPwDiff & 0xf);
		// Antenna C gain offset to antenna A, bit4~7
			priv->AntennaTxPwDiff[1] = ((priv->EEPROMAntPwDiff & 0xf0)>>4);
		// Antenna D gain offset to antenna A, bit8~11
			priv->AntennaTxPwDiff[2] = ((priv->EEPROMAntPwDiff & 0xf00)>>8);
		// CrystalCap, bit12~15
			priv->CrystalCap = priv->EEPROMCrystalCap;
		// ThermalMeter, bit0~3 for RFIC1, bit4~7 for RFIC2
			priv->ThermalMeter[0] = (priv->EEPROMThermalMeter & 0xf);
			priv->ThermalMeter[1] = ((priv->EEPROMThermalMeter & 0xf0)>>4);
		}
		else if(priv->epromtype == EEPROM_93C56)
		{
			//char	cck_pwr_diff_a=0, cck_pwr_diff_c=0;

			//cck_pwr_diff_a = pHalData->EEPROMRfACCKChnl7TxPwLevel - pHalData->EEPROMRfAOfdmChnlTxPwLevel[1];
			//cck_pwr_diff_c = pHalData->EEPROMRfCCCKChnl7TxPwLevel - pHalData->EEPROMRfCOfdmChnlTxPwLevel[1];
			for(i=0; i<3; i++)	// channel 1~3 use the same Tx Power Level.
			{
				priv->TxPowerLevelCCK_A[i]  = priv->EEPROMRfACCKChnl1TxPwLevel[0];
				priv->TxPowerLevelOFDM24G_A[i] = priv->EEPROMRfAOfdmChnlTxPwLevel[0];
				priv->TxPowerLevelCCK_C[i] =  priv->EEPROMRfCCCKChnl1TxPwLevel[0];
				priv->TxPowerLevelOFDM24G_C[i] = priv->EEPROMRfCOfdmChnlTxPwLevel[0];
			}
			for(i=3; i<9; i++)	// channel 4~9 use the same Tx Power Level
			{
				priv->TxPowerLevelCCK_A[i]  = priv->EEPROMRfACCKChnl1TxPwLevel[1];
				priv->TxPowerLevelOFDM24G_A[i] = priv->EEPROMRfAOfdmChnlTxPwLevel[1];
				priv->TxPowerLevelCCK_C[i] =  priv->EEPROMRfCCCKChnl1TxPwLevel[1];
				priv->TxPowerLevelOFDM24G_C[i] = priv->EEPROMRfCOfdmChnlTxPwLevel[1];
			}
			for(i=9; i<14; i++)	// channel 10~14 use the same Tx Power Level
			{
				priv->TxPowerLevelCCK_A[i]  = priv->EEPROMRfACCKChnl1TxPwLevel[2];
				priv->TxPowerLevelOFDM24G_A[i] = priv->EEPROMRfAOfdmChnlTxPwLevel[2];
				priv->TxPowerLevelCCK_C[i] =  priv->EEPROMRfCCCKChnl1TxPwLevel[2];
				priv->TxPowerLevelOFDM24G_C[i] = priv->EEPROMRfCOfdmChnlTxPwLevel[2];
			}
			for(i=0; i<14; i++)
				RT_TRACE(COMP_INIT, "priv->TxPowerLevelCCK_A[%d] = 0x%x\n", i, priv->TxPowerLevelCCK_A[i]);
			for(i=0; i<14; i++)
				RT_TRACE(COMP_INIT,"priv->TxPowerLevelOFDM24G_A[%d] = 0x%x\n", i, priv->TxPowerLevelOFDM24G_A[i]);
			for(i=0; i<14; i++)
				RT_TRACE(COMP_INIT, "priv->TxPowerLevelCCK_C[%d] = 0x%x\n", i, priv->TxPowerLevelCCK_C[i]);
			for(i=0; i<14; i++)
				RT_TRACE(COMP_INIT, "priv->TxPowerLevelOFDM24G_C[%d] = 0x%x\n", i, priv->TxPowerLevelOFDM24G_C[i]);
			priv->LegacyHTTxPowerDiff = priv->EEPROMLegacyHTTxPowerDiff;
			priv->AntennaTxPwDiff[0] = 0;
			priv->AntennaTxPwDiff[1] = 0;
			priv->AntennaTxPwDiff[2] = 0;
			priv->CrystalCap = priv->EEPROMCrystalCap;
			// ThermalMeter, bit0~3 for RFIC1, bit4~7 for RFIC2
			priv->ThermalMeter[0] = (priv->EEPROMThermalMeter & 0xf);
			priv->ThermalMeter[1] = ((priv->EEPROMThermalMeter & 0xf0)>>4);
		}
	}

	if(priv->rf_type == RF_1T2R)
	{		
		RT_TRACE(COMP_INIT, "\n1T2R config\n");
	}
	else if (priv->rf_type == RF_2T4R)
	{
		RT_TRACE(COMP_INIT, "\n2T4R config\n");
	}

	// 2008/01/16 MH We can only know RF type in the function. So we have to init
	// DIG RATR table again.
	init_rate_adaptive(dev);
	
	//1 Make a copy for following variables and we can change them if we want
	
	priv->rf_chip= RF_8256;

	if(priv->RegChannelPlan == 0xf)
	{
		priv->ChannelPlan = priv->eeprom_ChannelPlan;
	}
	else
	{
		priv->ChannelPlan = priv->RegChannelPlan;
	}

	//
	//  Used PID and DID to Set CustomerID 
	//
	if( priv->eeprom_vid == 0x1186 &&  priv->eeprom_did == 0x3304 )
	{
		priv->CustomerID =  RT_CID_DLINK;
	}
	
	switch(priv->eeprom_CustomerID)
	{
		case EEPROM_CID_DEFAULT:
			priv->CustomerID = RT_CID_DEFAULT;
			break;
		case EEPROM_CID_CAMEO:
			priv->CustomerID = RT_CID_819x_CAMEO;
			break;
		case  EEPROM_CID_RUNTOP:
			priv->CustomerID = RT_CID_819x_RUNTOP;
			break;
		case EEPROM_CID_NetCore:
			priv->CustomerID = RT_CID_819x_Netcore;
			break;	
		case EEPROM_CID_TOSHIBA:        // Merge by Jacken, 2008/01/31
			priv->CustomerID = RT_CID_TOSHIBA;
			if(priv->eeprom_ChannelPlan&0x80)
				priv->ChannelPlan = priv->eeprom_ChannelPlan&0x7f;
			else
				priv->ChannelPlan = 0x0;
			RT_TRACE(COMP_INIT, "Toshiba ChannelPlan = 0x%x\n", 
				priv->ChannelPlan);
			break;
		case EEPROM_CID_Nettronix:
			priv->ScanDelay = 100;	//cosa add for scan
			priv->CustomerID = RT_CID_Nettronix;
			break;			
		case EEPROM_CID_Pronet:
			priv->CustomerID = RT_CID_PRONET;
			break;
		case EEPROM_CID_DLINK:
			priv->CustomerID = RT_CID_DLINK;
			break;

		case EEPROM_CID_WHQL:
			//dev->bInHctTest = TRUE;//do not supported

			//priv->bSupportTurboMode = FALSE;
			//priv->bAutoTurboBy8186 = FALSE;

			//pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
			//pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
			//pMgntInfo->PowerSaveControl.bLeisurePs = FALSE;
			
			break;
		default:
			// value from RegCustomerID
			break;
	}

	//Avoid the channel plan array overflow, by Bruce, 2007-08-27. 
	if(priv->ChannelPlan > CHANNEL_PLAN_LEN - 1)
		priv->ChannelPlan = 0; //FCC

	switch(priv->CustomerID)
	{
		case RT_CID_DEFAULT:
		#ifdef RTL8190P
			priv->LedStrategy = HW_LED;
		#else
			#ifdef RTL8192E
			priv->LedStrategy = SW_LED_MODE1;
			#endif
		#endif
			break;

		case RT_CID_819x_CAMEO:
			priv->LedStrategy = SW_LED_MODE2;
			break;

		case RT_CID_819x_RUNTOP:
			priv->LedStrategy = SW_LED_MODE3;
			break;

		case RT_CID_819x_Netcore:
			priv->LedStrategy = SW_LED_MODE4;
			break;
	
		case RT_CID_Nettronix:
			priv->LedStrategy = SW_LED_MODE5;
			break;
			
		case RT_CID_PRONET:
			priv->LedStrategy = SW_LED_MODE6;
			break;
			
		case RT_CID_TOSHIBA:   //Modify by Jacken 2008/01/31
			// Do nothing.
			//break;

		default:
		#ifdef RTL8190P
			priv->LedStrategy = HW_LED;
		#else
			#ifdef RTL8192E
			priv->LedStrategy = SW_LED_MODE1;
			#endif
		#endif
			break;
	}
/*
	//2008.06.03, for WOL
	if( priv->eeprom_vid == 0x1186 &&  priv->eeprom_did == 0x3304)
		priv->ieee80211->bSupportRemoteWakeUp = true;
	else
		priv->ieee80211->bSupportRemoteWakeUp = FALSE;
*/
	RT_TRACE(COMP_INIT, "RegChannelPlan(%d)\n", priv->RegChannelPlan);
	RT_TRACE(COMP_INIT, "ChannelPlan = %d \n", priv->ChannelPlan);
	RT_TRACE(COMP_INIT, "LedStrategy = %d \n", priv->LedStrategy);
	RT_TRACE(COMP_TRACE, "<==== ReadAdapterInfo\n");
		
	return ;
}

void rtl8192_get_eeprom_size(struct net_device* dev)
{
	u16 curCR = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	RT_TRACE(COMP_INIT, "===========>%s()\n", __FUNCTION__);	
	curCR = read_nic_dword(dev, EPROM_CMD);
	RT_TRACE(COMP_INIT, "read from Reg Cmd9346CR(%x):%x\n", EPROM_CMD, curCR);
	//whether need I consider BIT5?	
	priv->epromtype = (curCR & EPROM_CMD_9356SEL) ? EEPROM_93C56 : EEPROM_93C46;
	RT_TRACE(COMP_INIT, "<===========%s(), epromtype:%d\n", __FUNCTION__, priv->epromtype);
	rtl8192_read_eeprom_info(dev);
}

/******************************************************************************
 *function:  This function actually only set RRSR, RATR and BW_OPMODE registers 
 *	     not to do all the hw config as its name says
 *   input:  net_device dev
 *  output:  none
 *  return:  none
 *  notice:  This part need to modified according to the rate set we filtered
 * ****************************************************************************/
void rtl8192_hwconfig(struct net_device* dev)
{
	u32 regRATR = 0, regRRSR = 0;
	u8 regBwOpMode = 0, regTmp = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);

// Set RRSR, RATR, and BW_OPMODE registers
	//
	switch(priv->ieee80211->mode)
	{
	case WIRELESS_MODE_B:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK;
		regRRSR = RATE_ALL_CCK;
		break;
	case WIRELESS_MODE_A:
		regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_G:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_AUTO:
	case WIRELESS_MODE_N_24G:
		// It support CCK rate by default.
		// CCK rate will be filtered out only when associated AP does not support it.
		regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_N_5G:
		regBwOpMode = BW_OPMODE_5G;
		regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	}

	write_nic_byte(dev, BW_OPMODE, regBwOpMode);
	{
		u32 ratr_value = 0;
		ratr_value = regRATR;
		if (priv->rf_type == RF_1T2R)
		{
			ratr_value &= ~(RATE_ALL_OFDM_2SS);
		}
		write_nic_dword(dev, RATR0, ratr_value);
		write_nic_byte(dev, UFWP, 1);
	}	
	regTmp = read_nic_byte(dev, 0x313);
	regRRSR = ((regTmp) << 24) | (regRRSR & 0x00ffffff);
	write_nic_dword(dev, RRSR, regRRSR);

	//
	// Set Retry Limit here
	//
	write_nic_word(dev, RETRY_LIMIT, 
			priv->ShortRetryLimit << RETRY_LIMIT_SHORT_SHIFT | \
			priv->LongRetryLimit << RETRY_LIMIT_LONG_SHIFT);
	// Set Contention Window here		

	// Set Tx AGC

	// Set Tx Antenna including Feedback control
		
	// Set Auto Rate fallback control
	
	
}


bool rtl8192_adapter_start(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
//	struct ieee80211_device *ieee = priv->ieee80211;
	u32 ulRegRead;
	bool rtStatus = true;
//	static char szMACPHYRegFile[] = RTL819X_PHY_MACPHY_REG;
//	static char szMACPHYRegPGFile[] = RTL819X_PHY_MACPHY_REG_PG;
	//u8 eRFPath;
	u8 tmpvalue;
#ifdef RTL8192E
	u8 ICVersion,SwitchingRegulatorOutput;
#endif
	bool bfirmwareok = true;
#ifdef RTL8190P
	u8 ucRegRead;
#endif
	u32	tmpRegA, tmpRegC, TempCCk;
	int	i =0;
//	u32 dwRegRead = 0;
	
	RT_TRACE(COMP_INIT, "====>%s()\n", __FUNCTION__);
	priv->being_init_adapter = true;	
        rtl8192_pci_resetdescring(dev);
	// 2007/11/02 MH Before initalizing RF. We can not use FW to do RF-R/W.
	priv->Rf_Mode = RF_OP_By_SW_3wire;
#ifdef RTL8192E
        //dPLL on	
        if(priv->ResetProgress == RESET_TYPE_NORESET)
        {
            write_nic_byte(dev, ANAPAR, 0x37);
            // Accordign to designer's explain, LBUS active will never > 10ms. We delay 10ms
            // Joseph increae the time to prevent firmware download fail
            mdelay(500);
        }
#endif
	//PlatformSleepUs(10000);
	// For any kind of InitializeAdapter process, we shall use system now!!
	priv->pFirmware->firmware_status = FW_STATUS_0_INIT;

	// Set to eRfoff in order not to count receive count.
	if(priv->RegRfOff == true)
		priv->ieee80211->eRFPowerState = eRfOff;

	//
	//3 //Config CPUReset Register
	//3//
	//3 Firmware Reset Or Not
	ulRegRead = read_nic_dword(dev, CPU_GEN);	
	if(priv->pFirmware->firmware_status == FW_STATUS_0_INIT)
	{	//called from MPInitialized. do nothing
		ulRegRead |= CPU_GEN_SYSTEM_RESET;
	}else if(priv->pFirmware->firmware_status == FW_STATUS_5_READY)
		ulRegRead |= CPU_GEN_FIRMWARE_RESET;	// Called from MPReset
	else
		RT_TRACE(COMP_ERR, "ERROR in %s(): undefined firmware state(%d)\n", __FUNCTION__,   priv->pFirmware->firmware_status);

#ifdef RTL8190P
	//2008.06.03, for WOL 90 hw bug
	ulRegRead &= (~(CPU_GEN_GPIO_UART));	
#endif
	
	write_nic_dword(dev, CPU_GEN, ulRegRead);
	//mdelay(100);

#ifdef RTL8192E

	//3//
	//3 //Fix the issue of E-cut high temperature issue
	//3//
	// TODO: E cut only
	ICVersion = read_nic_byte(dev, IC_VERRSION);
	if(ICVersion >= 0x4) //E-cut only 
	{
		// HW SD suggest that we should not wirte this register too often, so driver
		// should readback this register. This register will be modified only when 
		// power on reset
		SwitchingRegulatorOutput = read_nic_byte(dev, SWREGULATOR);
		if(SwitchingRegulatorOutput  != 0xb8)
		{
			write_nic_byte(dev, SWREGULATOR, 0xa8);
			mdelay(1);
			write_nic_byte(dev, SWREGULATOR, 0xb8);
		}
	}
#endif


	//3//
	//3// Initialize BB before MAC
	//3//
	//rtl8192_dump_reg(dev);	
	RT_TRACE(COMP_INIT, "BB Config Start!\n");
	rtStatus = rtl8192_BBConfig(dev);
	if(rtStatus != true)
	{
		RT_TRACE(COMP_ERR, "BB Config failed\n");
		return rtStatus;
	}
	RT_TRACE(COMP_INIT,"BB Config Finished!\n");

	//rtl8192_dump_reg(dev);	
	//
	//3//Set Loopback mode or Normal mode
	//3//
	//2006.12.13 by emily. Note!We should not merge these two CPU_GEN register writings 
	//	because setting of System_Reset bit reset MAC to default transmission mode.
		//Loopback mode or not
	priv->LoopbackMode = RTL819X_NO_LOOPBACK;
	//priv->LoopbackMode = RTL819X_MAC_LOOPBACK;
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
	ulRegRead = read_nic_dword(dev, CPU_GEN);	
	if(priv->LoopbackMode == RTL819X_NO_LOOPBACK)
	{
		ulRegRead = ((ulRegRead & CPU_GEN_NO_LOOPBACK_MSK) | CPU_GEN_NO_LOOPBACK_SET);
	}
	else if (priv->LoopbackMode == RTL819X_MAC_LOOPBACK )
	{
		ulRegRead |= CPU_CCK_LOOPBACK;
	}
	else
	{
		RT_TRACE(COMP_ERR,"Serious error: wrong loopback mode setting\n");	
	}

	//2008.06.03, for WOL
	//ulRegRead &= (~(CPU_GEN_GPIO_UART));	
	write_nic_dword(dev, CPU_GEN, ulRegRead);
		
	// 2006.11.29. After reset cpu, we sholud wait for a second, otherwise, it may fail to write registers. Emily
	udelay(500);
	}
	//3Set Hardware(Do nothing now)
	rtl8192_hwconfig(dev);
	//2=======================================================
	// Common Setting for all of the FPGA platform. (part 1)
	//2=======================================================
	// If there is changes, please make sure it applies to all of the FPGA version
	//3 Turn on Tx/Rx
	write_nic_byte(dev, CMDR, CR_RE|CR_TE);

	//2Set Tx dma burst 
#ifdef RTL8190P
	write_nic_byte(dev, PCIF, ((MXDMA2_NoLimit<<MXDMA2_RX_SHIFT) | \
											(MXDMA2_NoLimit<<MXDMA2_TX_SHIFT) | \
											(1<<MULRW_SHIFT)));
#else
	#ifdef RTL8192E
	write_nic_byte(dev, PCIF, ((MXDMA2_NoLimit<<MXDMA2_RX_SHIFT) |\
				   (MXDMA2_NoLimit<<MXDMA2_TX_SHIFT) ));
	#endif
#endif
	//set IDR0 here
	write_nic_dword(dev, MAC0, ((u32*)dev->dev_addr)[0]);
	write_nic_word(dev, MAC4, ((u16*)(dev->dev_addr + 4))[0]);
	//set RCR
	write_nic_dword(dev, RCR, priv->ReceiveConfig);

	//3 Initialize Number of Reserved Pages in Firmware Queue
	#ifdef TO_DO_LIST 
	if(priv->bInHctTest)
	{
		write_nic_dword(dev, RQPN1,  NUM_OF_PAGE_IN_FW_QUEUE_BK_DTM << RSVD_FW_QUEUE_PAGE_BK_SHIFT |\
                                       	NUM_OF_PAGE_IN_FW_QUEUE_BE_DTM << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
					NUM_OF_PAGE_IN_FW_QUEUE_VI_DTM << RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
					NUM_OF_PAGE_IN_FW_QUEUE_VO_DTM <<RSVD_FW_QUEUE_PAGE_VO_SHIFT);												
		write_nic_dword(dev, RQPN2, NUM_OF_PAGE_IN_FW_QUEUE_MGNT << RSVD_FW_QUEUE_PAGE_MGNT_SHIFT);
		write_nic_dword(dev, RQPN3, APPLIED_RESERVED_QUEUE_IN_FW| \
					NUM_OF_PAGE_IN_FW_QUEUE_BCN<<RSVD_FW_QUEUE_PAGE_BCN_SHIFT|\
					NUM_OF_PAGE_IN_FW_QUEUE_PUB_DTM<<RSVD_FW_QUEUE_PAGE_PUB_SHIFT);
	}
	else
	#endif
	{
		write_nic_dword(dev, RQPN1,  NUM_OF_PAGE_IN_FW_QUEUE_BK << RSVD_FW_QUEUE_PAGE_BK_SHIFT |\
					NUM_OF_PAGE_IN_FW_QUEUE_BE << RSVD_FW_QUEUE_PAGE_BE_SHIFT | \
					NUM_OF_PAGE_IN_FW_QUEUE_VI << RSVD_FW_QUEUE_PAGE_VI_SHIFT | \
					NUM_OF_PAGE_IN_FW_QUEUE_VO <<RSVD_FW_QUEUE_PAGE_VO_SHIFT);												
		write_nic_dword(dev, RQPN2, NUM_OF_PAGE_IN_FW_QUEUE_MGNT << RSVD_FW_QUEUE_PAGE_MGNT_SHIFT);
		write_nic_dword(dev, RQPN3, APPLIED_RESERVED_QUEUE_IN_FW| \
					NUM_OF_PAGE_IN_FW_QUEUE_BCN<<RSVD_FW_QUEUE_PAGE_BCN_SHIFT|\
					NUM_OF_PAGE_IN_FW_QUEUE_PUB<<RSVD_FW_QUEUE_PAGE_PUB_SHIFT);
	}

	rtl8192_tx_enable(dev);
	rtl8192_rx_enable(dev);
	//3Set Response Rate Setting Register
	// CCK rate is supported by default.
	// CCK rate will be filtered out only when associated AP does not support it.
	ulRegRead = (0xFFF00000 & read_nic_dword(dev, RRSR))  | RATE_ALL_OFDM_AG | RATE_ALL_CCK;
	write_nic_dword(dev, RRSR, ulRegRead);
	write_nic_dword(dev, RATR0+4*7, (RATE_ALL_OFDM_AG | RATE_ALL_CCK));

	//2Set AckTimeout
	// TODO: (it value is only for FPGA version). need to be changed!!2006.12.18, by Emily
	write_nic_byte(dev, ACK_TIMEOUT, 0x30);

	//rtl8192_actset_wirelessmode(dev,priv->RegWirelessMode);
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	rtl8192_SetWirelessMode(dev, priv->ieee80211->mode);
	//-----------------------------------------------------------------------------
	// Set up security related. 070106, by rcnjko:
	// 1. Clear all H/W keys.
	// 2. Enable H/W encryption/decryption.
	//-----------------------------------------------------------------------------
	CamResetAllEntry(dev);
	{
		u8 SECR_value = 0x0;
		SECR_value |= SCR_TxEncEnable;
		SECR_value |= SCR_RxDecEnable;
		SECR_value |= SCR_NoSKMC;
		write_nic_byte(dev, SECR, SECR_value);
	}
	//3Beacon related
	write_nic_word(dev, ATIMWND, 2);
	write_nic_word(dev, BCN_INTERVAL, 100);
	{	
		int i;
		for (i=0; i<QOS_QUEUE_NUM; i++)
		write_nic_dword(dev, WDCAPARA_ADD[i], 0x005e4332);
	}
	//
	// Switching regulator controller: This is set temporarily. 
	// It's not sure if this can be removed in the future.
	// PJ advised to leave it by default.
	//
	write_nic_byte(dev, 0xbe, 0xc0);

	//2=======================================================
	// Set PHY related configuration defined in MAC register bank
	//2=======================================================
	rtl8192_phy_configmac(dev);

	if (priv->card_8192_version > (u8) VERSION_8190_BD) {
		rtl8192_phy_getTxPower(dev);
		rtl8192_phy_setTxPower(dev, priv->chan);
	}
	
	//if D or C cut
		tmpvalue = read_nic_byte(dev, IC_VERRSION);
		priv->IC_Cut= tmpvalue;
		RT_TRACE(COMP_INIT, "priv->IC_Cut= 0x%x\n", priv->IC_Cut);
		if(priv->IC_Cut>= IC_VersionCut_D)
		{
			//pHalData->bDcut = true;
			if(priv->IC_Cut== IC_VersionCut_D)
				RT_TRACE(COMP_INIT, "D-cut\n");
			if(priv->IC_Cut== IC_VersionCut_E)
			{
				RT_TRACE(COMP_INIT, "E-cut\n");
				// HW SD suggest that we should not wirte this register too often, so driver
				// should readback this register. This register will be modified only when 
				// power on reset
			}
		}
		else
		{
			//pHalData->bDcut = FALSE;
			RT_TRACE(COMP_INIT, "Before C-cut\n");
		}
		
#if 1
	//Firmware download 
	RT_TRACE(COMP_INIT, "Load Firmware!\n");
	bfirmwareok = init_firmware(dev);
	if(bfirmwareok != true) {
		rtStatus = false;
		return rtStatus;
	}
	RT_TRACE(COMP_INIT, "Load Firmware finished!\n");
#endif
	//RF config
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
	RT_TRACE(COMP_INIT, "RF Config Started!\n");
	rtStatus = rtl8192_phy_RFConfig(dev);
	if(rtStatus != true)
	{
		RT_TRACE(COMP_ERR, "RF Config failed\n");
			return rtStatus;
	}	
	RT_TRACE(COMP_INIT, "RF Config Finished!\n");
	}
	rtl8192_phy_updateInitGain(dev);

	/*---- Set CCK and OFDM Block "ON"----*/
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bCCKEn, 0x1);
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, 0x1);

#ifdef RTL8192E
	//Enable Led 
	write_nic_byte(dev, 0x87, 0x0);
#endif
#ifdef RTL8190P
	//2008.06.03, for WOL		
	ucRegRead = read_nic_byte(dev, GPE);
	ucRegRead |= BIT0;
	write_nic_byte(dev, GPE, ucRegRead);

	ucRegRead = read_nic_byte(dev, GPO);
	ucRegRead &= ~BIT0;
	write_nic_byte(dev, GPO, ucRegRead);
#endif

	//2=======================================================
	// RF Power Save
	//2=======================================================
#ifdef ENABLE_IPS
	
{
	if(priv->RegRfOff == true)
	{ // User disable RF via registry.
		RT_TRACE((COMP_INIT|COMP_RF|COMP_POWER), "%s(): Turn off RF for RegRfOff ----------\n",__FUNCTION__);
		MgntActSet_RF_State(dev, eRfOff, RF_CHANGE_BY_SW);
#if 0//cosa, ask SD3 willis and he doesn't know what is this for
		// Those action will be discard in MgntActSet_RF_State because off the same state
	for(eRFPath = 0; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
#endif
	}
	else if(priv->ieee80211->RfOffReason > RF_CHANGE_BY_PS)
	{ // H/W or S/W RF OFF before sleep.
		RT_TRACE((COMP_INIT|COMP_RF|COMP_POWER), "%s(): Turn off RF for RfOffReason(%d) ----------\n", __FUNCTION__,priv->ieee80211->RfOffReason);
		MgntActSet_RF_State(dev, eRfOff, priv->ieee80211->RfOffReason);
	}
	else if(priv->ieee80211->RfOffReason >= RF_CHANGE_BY_IPS)
	{ // H/W or S/W RF OFF before sleep.
		RT_TRACE((COMP_INIT|COMP_RF|COMP_POWER), "%s(): Turn off RF for RfOffReason(%d) ----------\n", __FUNCTION__,priv->ieee80211->RfOffReason);
		MgntActSet_RF_State(dev, eRfOff, priv->ieee80211->RfOffReason);
	}
	else
	{
		RT_TRACE((COMP_INIT|COMP_RF|COMP_POWER), "%s(): RF-ON \n",__FUNCTION__);
		priv->ieee80211->eRFPowerState = eRfOn;
		priv->ieee80211->RfOffReason = 0; 
		//DrvIFIndicateCurrentPhyStatus(dev);
	// LED control
	//dev->HalFunc.LedControlHandler(dev, LED_CTL_POWER_ON);

	//
	// If inactive power mode is enabled, disable rf while in disconnected state.
	// But we should still tell upper layer we are in rf on state.
	// 2007.07.16, by shien chang.
	//
		//if(!dev->bInHctTest)			
	//IPSEnter(dev);				

	}
}
#endif
	if(1){
#ifdef RTL8192E
			// We can force firmware to do RF-R/W
			if(priv->ieee80211->FwRWRF)
				priv->Rf_Mode = RF_OP_By_FW;
			else
				priv->Rf_Mode = RF_OP_By_SW_3wire;
#else
			priv->Rf_Mode = RF_OP_By_SW_3wire;
#endif
	}
#ifdef RTL8190P
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
		dm_initialize_txpower_tracking(dev);
		
		tmpRegA= rtl8192_QueryBBReg(dev,rOFDM0_XATxIQImbalance,bMaskDWord);
		tmpRegC= rtl8192_QueryBBReg(dev,rOFDM0_XCTxIQImbalance,bMaskDWord);
		
		if(priv->rf_type == RF_2T4R){
		for(i = 0; i<TxBBGainTableLength; i++)
		{
			if(tmpRegA == priv->txbbgain_table[i].txbbgain_value)
			{
				priv->rfa_txpowertrackingindex= (u8)i;
				priv->rfa_txpowertrackingindex_real= (u8)i;
				priv->rfa_txpowertracking_default = priv->rfa_txpowertrackingindex;
				break;
			}
		}
		}
		for(i = 0; i<TxBBGainTableLength; i++)
		{
			if(tmpRegC == priv->txbbgain_table[i].txbbgain_value)
			{
				priv->rfc_txpowertrackingindex= (u8)i;
				priv->rfc_txpowertrackingindex_real= (u8)i;
				priv->rfc_txpowertracking_default = priv->rfc_txpowertrackingindex;
				break;
			}
		}
		TempCCk = rtl8192_QueryBBReg(dev, rCCK0_TxFilter1, bMaskByte2);				

		for(i=0 ; i<CCKTxBBGainTableLength ; i++)
		{
			if(TempCCk == priv->cck_txbbgain_table[i].ccktxbb_valuearray[0])
			{
				priv->CCKPresentAttentuation_20Mdefault =(u8) i;
				break;
			}
		}
		priv->CCKPresentAttentuation_40Mdefault = 0;
		priv->CCKPresentAttentuation_difference = 0;
		priv->CCKPresentAttentuation = priv->CCKPresentAttentuation_20Mdefault;
		RT_TRACE(COMP_POWER_TRACKING, "priv->rfa_txpowertrackingindex_initial = %d\n", priv->rfa_txpowertrackingindex);
		RT_TRACE(COMP_POWER_TRACKING, "priv->rfa_txpowertrackingindex_real__initial = %d\n", priv->rfa_txpowertrackingindex_real);
		RT_TRACE(COMP_POWER_TRACKING, "priv->rfc_txpowertrackingindex_initial = %d\n", priv->rfc_txpowertrackingindex);
		RT_TRACE(COMP_POWER_TRACKING, "priv->rfc_txpowertrackingindex_real_initial = %d\n", priv->rfc_txpowertrackingindex_real);
		RT_TRACE(COMP_POWER_TRACKING, "priv->CCKPresentAttentuation_difference_initial = %d\n", priv->CCKPresentAttentuation_difference);
		RT_TRACE(COMP_POWER_TRACKING, "priv->CCKPresentAttentuation_initial = %d\n", priv->CCKPresentAttentuation);
	}
#else
	#ifdef RTL8192E
	if(priv->ResetProgress == RESET_TYPE_NORESET)
	{
		dm_initialize_txpower_tracking(dev);

		if(priv->IC_Cut>= IC_VersionCut_D)
		{
			tmpRegA= rtl8192_QueryBBReg(dev,rOFDM0_XATxIQImbalance,bMaskDWord);
			tmpRegC= rtl8192_QueryBBReg(dev,rOFDM0_XCTxIQImbalance,bMaskDWord);
			for(i = 0; i<TxBBGainTableLength; i++)
			{
				if(tmpRegA == priv->txbbgain_table[i].txbbgain_value)
				{
					priv->rfa_txpowertrackingindex= (u8)i;
					priv->rfa_txpowertrackingindex_real= (u8)i;
					priv->rfa_txpowertracking_default = priv->rfa_txpowertrackingindex;
					break;
				}
			}

		TempCCk = rtl8192_QueryBBReg(dev, rCCK0_TxFilter1, bMaskByte2);				

		for(i=0 ; i<CCKTxBBGainTableLength ; i++)
		{
			if(TempCCk == priv->cck_txbbgain_table[i].ccktxbb_valuearray[0])
			{
				priv->CCKPresentAttentuation_20Mdefault =(u8) i;
				break;
			}
		}
		priv->CCKPresentAttentuation_40Mdefault = 0;
		priv->CCKPresentAttentuation_difference = 0;
		priv->CCKPresentAttentuation = priv->CCKPresentAttentuation_20Mdefault;
			RT_TRACE(COMP_POWER_TRACKING, "priv->rfa_txpowertrackingindex_initial = %d\n", priv->rfa_txpowertrackingindex);
			RT_TRACE(COMP_POWER_TRACKING, "priv->rfa_txpowertrackingindex_real__initial = %d\n", priv->rfa_txpowertrackingindex_real);
			RT_TRACE(COMP_POWER_TRACKING, "priv->CCKPresentAttentuation_difference_initial = %d\n", priv->CCKPresentAttentuation_difference);
			RT_TRACE(COMP_POWER_TRACKING, "priv->CCKPresentAttentuation_initial = %d\n", priv->CCKPresentAttentuation);
			priv->btxpower_tracking = FALSE;//TEMPLY DISABLE
		}
	}
	#endif
#endif
	rtl8192_irq_enable(dev);
	priv->being_init_adapter = false;
	return rtStatus;

}
	 

void rtl8192_net_update(struct net_device *dev)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_network *net;
	u16 BcnTimeCfg = 0, BcnCW = 6, BcnIFS = 0xf;
	u16 rate_config = 0;
	net = &priv->ieee80211->current_network;
	//update Basic rate: RR, BRSR
	rtl8192_config_rate(dev, &rate_config);	
	// 2007.01.16, by Emily
	// Select RRSR (in Legacy-OFDM and CCK)
	// For 8190, we select only 24M, 12M, 6M, 11M, 5.5M, 2M, and 1M from the Basic rate.
	// We do not use other rates.
	 priv->basic_rate = rate_config &= 0x15f;
	//BSSID
	write_nic_dword(dev,BSSIDR,((u32*)net->bssid)[0]);
	write_nic_word(dev,BSSIDR+4,((u16*)net->bssid)[2]);
#if 0 
	//MSR
	rtl8192_update_msr(dev);
#endif


//	rtl8192_update_cap(dev, net->capability);
	if (priv->ieee80211->iw_mode == IW_MODE_ADHOC)
	{
		write_nic_word(dev, ATIMWND, 2);
		write_nic_word(dev, BCN_DMATIME, 256);	
		write_nic_word(dev, BCN_INTERVAL, net->beacon_interval);
	//	write_nic_word(dev, BcnIntTime, 100);
	//BIT15 of BCN_DRV_EARLY_INT will indicate whether software beacon or hw beacon is applied.
		write_nic_word(dev, BCN_DRV_EARLY_INT, 10);
		write_nic_byte(dev, BCN_ERR_THRESH, 100);

		BcnTimeCfg |= (BcnCW<<BCN_TCFG_CW_SHIFT);
	// TODO: BcnIFS may required to be changed on ASIC
	 	BcnTimeCfg |= BcnIFS<<BCN_TCFG_IFS;

		write_nic_word(dev, BCN_TCFG, BcnTimeCfg);
	}
	

}


//updateRATRTabel for MCS only. Basic rate is not implement.
void rtl8192_update_ratr_table(struct net_device* dev)
	//	POCTET_STRING	posLegacyRate,
	//	u8*			pMcsRate)
	//	PRT_WLAN_STA	pEntry)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	u8* pMcsRate = ieee->dot11HTOperationalRateSet;
	//struct ieee80211_network *net = &ieee->current_network;
	u32 ratr_value = 0;
	u8 rate_index = 0;

	rtl8192_config_rate(dev, (u16*)(&ratr_value));
	ratr_value |= (*(u16*)(pMcsRate)) << 12;
//	switch (net->mode)
	switch (ieee->mode)
	{
		case IEEE_A:
			ratr_value &= 0x00000FF0;
			break;
		case IEEE_B:
			ratr_value &= 0x0000000F;
			break;
		case IEEE_G:
			ratr_value &= 0x00000FF7;
			break;
		case IEEE_N_24G:
		case IEEE_N_5G:
			if (ieee->pHTInfo->PeerMimoPs == 0) //MIMO_PS_STATIC
				ratr_value &= 0x0007F007;
			else{
				if (priv->rf_type == RF_1T2R)
					ratr_value &= 0x000FF007;
				else
					ratr_value &= 0x0F81F007;
			}
			break;
		default:
			break; 
	}
	ratr_value &= 0x0FFFFFFF;
	if(ieee->pHTInfo->bCurTxBW40MHz && ieee->pHTInfo->bCurShortGI40MHz){
		ratr_value |= 0x80000000;
	}else if(!ieee->pHTInfo->bCurTxBW40MHz && ieee->pHTInfo->bCurShortGI20MHz){
		ratr_value |= 0x80000000;
	}	
	write_nic_dword(dev, RATR0+rate_index*4, ratr_value);
	write_nic_byte(dev, UFWP, 1);
}
#if 1
void rtl8192_link_change(struct net_device *dev)
{
//	int i;
	
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	//write_nic_word(dev, BCN_INTR_ITV, net->beacon_interval);
	if (ieee->state == IEEE80211_LINKED)
	{
		rtl8192_net_update(dev);
		rtl8192_update_ratr_table(dev);
#if 1
		//add this as in pure N mode, wep encryption will use software way, but there is no chance to set this as wep will not set group key in wext. WB.2008.07.08
		if ((KEY_TYPE_WEP40 == ieee->pairwise_key_type) || (KEY_TYPE_WEP104 == ieee->pairwise_key_type))
		EnableHWSecurityConfig8192(dev);
#endif
	}
	else
	{
		write_nic_byte(dev, 0x173, 0);
	}
	/*update timing params*/
	//rtl8192_set_chan(dev, priv->chan);
	//MSR
	rtl8192_update_msr(dev);
	
	// 2007/10/16 MH MAC Will update TSF according to all received beacon, so we have
	//	// To set CBSSID bit when link with any AP or STA.	
	if (ieee->iw_mode == IW_MODE_INFRA || ieee->iw_mode == IW_MODE_ADHOC)
	{
		u32 reg = 0;
		reg = read_nic_dword(dev, RCR);
		if (priv->ieee80211->state == IEEE80211_LINKED)
			priv->ReceiveConfig = reg |= RCR_CBSSID;
		else
			priv->ReceiveConfig = reg &= ~RCR_CBSSID;
		write_nic_dword(dev, RCR, reg);
	}
}
#endif
void  rtl8192_tx_fill_cmd_desc(struct net_device* dev, tx_desc_cmd * entry, cb_desc * cb_desc, struct sk_buff* skb)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
    dma_addr_t mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
    memset(entry,0,12);
    entry->LINIP = cb_desc->bLastIniPkt;
    entry->FirstSeg = 1;//first segment
    entry->LastSeg = 1; //last segment
    if(cb_desc->bCmdOrInit == DESC_PACKET_TYPE_INIT) {
        entry->CmdInit = DESC_PACKET_TYPE_INIT;
    } else {
	tx_desc* entry_tmp = (tx_desc*)entry;
        entry_tmp->CmdInit = DESC_PACKET_TYPE_NORMAL;
        entry_tmp->Offset = sizeof(TX_FWINFO_8190PCI) + 8;
        entry_tmp->PktSize = (u16)(cb_desc->pkt_size + entry_tmp->Offset);
        entry_tmp->QueueSelect = QSLT_CMD;
        entry_tmp->TxFWInfoSize = 0x08;
        entry_tmp->RATid = (u8)DESC_PACKET_TYPE_INIT;
    }
    entry->TxBufferSize = skb->len;
    entry->TxBuffAddr = cpu_to_le32(mapping);
    entry->OWN = 1;
}


u8 QueryIsShort(u8 TxHT, u8 TxRate, cb_desc *tcb_desc)
{
	u8   tmp_Short;

	tmp_Short = (TxHT==1)?((tcb_desc->bUseShortGI)?1:0):((tcb_desc->bUseShortPreamble)?1:0);

	if(TxHT==1 && TxRate != DESC90_RATEMCS15)
		tmp_Short = 0;

	return tmp_Short;
}

u8 MRateToHwRate8190Pci(u8 rate)
{
	u8  ret = DESC90_RATE1M;

	switch(rate) {
		case MGN_1M:	ret = DESC90_RATE1M;		break;
		case MGN_2M:	ret = DESC90_RATE2M;		break;
		case MGN_5_5M:	ret = DESC90_RATE5_5M;	break;
		case MGN_11M:	ret = DESC90_RATE11M;	break;
		case MGN_6M:	ret = DESC90_RATE6M;		break;
		case MGN_9M:	ret = DESC90_RATE9M;		break;
		case MGN_12M:	ret = DESC90_RATE12M;	break;
		case MGN_18M:	ret = DESC90_RATE18M;	break;
		case MGN_24M:	ret = DESC90_RATE24M;	break;
		case MGN_36M:	ret = DESC90_RATE36M;	break;
		case MGN_48M:	ret = DESC90_RATE48M;	break;
		case MGN_54M:	ret = DESC90_RATE54M;	break;

		// HT rate since here
		case MGN_MCS0:	ret = DESC90_RATEMCS0;	break;
		case MGN_MCS1:	ret = DESC90_RATEMCS1;	break;
		case MGN_MCS2:	ret = DESC90_RATEMCS2;	break;
		case MGN_MCS3:	ret = DESC90_RATEMCS3;	break;
		case MGN_MCS4:	ret = DESC90_RATEMCS4;	break;
		case MGN_MCS5:	ret = DESC90_RATEMCS5;	break;
		case MGN_MCS6:	ret = DESC90_RATEMCS6;	break;
		case MGN_MCS7:	ret = DESC90_RATEMCS7;	break;
		case MGN_MCS8:	ret = DESC90_RATEMCS8;	break;
		case MGN_MCS9:	ret = DESC90_RATEMCS9;	break;
		case MGN_MCS10:	ret = DESC90_RATEMCS10;	break;
		case MGN_MCS11:	ret = DESC90_RATEMCS11;	break;
		case MGN_MCS12:	ret = DESC90_RATEMCS12;	break;
		case MGN_MCS13:	ret = DESC90_RATEMCS13;	break;
		case MGN_MCS14:	ret = DESC90_RATEMCS14;	break;
		case MGN_MCS15:	ret = DESC90_RATEMCS15;	break;
		case (0x80|0x20): ret = DESC90_RATEMCS32; break;

		default:       break;
	}
	return ret;
}

void  rtl8192_tx_fill_desc(struct net_device* dev, tx_desc * pdesc, cb_desc * cb_desc, struct sk_buff* skb)
{
    struct r8192_priv *priv = ieee80211_priv(dev);
    dma_addr_t mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
    TX_FWINFO_8190PCI *pTxFwInfo = NULL;
	 /* fill tx firmware */
    pTxFwInfo = (PTX_FWINFO_8190PCI)skb->data;
    memset(pTxFwInfo,0,sizeof(TX_FWINFO_8190PCI));
    pTxFwInfo->TxHT = (cb_desc->data_rate&0x80)?1:0;
    pTxFwInfo->TxRate = MRateToHwRate8190Pci((u8)cb_desc->data_rate);
    pTxFwInfo->EnableCPUDur = cb_desc->bTxEnableFwCalcDur;
    pTxFwInfo->Short	= QueryIsShort(pTxFwInfo->TxHT, pTxFwInfo->TxRate, cb_desc);

    /* Aggregation related */
    if(cb_desc->bAMPDUEnable) {
        pTxFwInfo->AllowAggregation = 1;
        pTxFwInfo->RxMF = cb_desc->ampdu_factor;
        pTxFwInfo->RxAMD = cb_desc->ampdu_density;
    } else {
        pTxFwInfo->AllowAggregation = 0;
        pTxFwInfo->RxMF = 0;
        pTxFwInfo->RxAMD = 0;
    }

    //
    // Protection mode related
    //
    pTxFwInfo->RtsEnable =	(cb_desc->bRTSEnable)?1:0;
    pTxFwInfo->CtsEnable =	(cb_desc->bCTSEnable)?1:0;
    pTxFwInfo->RtsSTBC =	(cb_desc->bRTSSTBC)?1:0;
    pTxFwInfo->RtsHT=		(cb_desc->rts_rate&0x80)?1:0;
    pTxFwInfo->RtsRate =		MRateToHwRate8190Pci((u8)cb_desc->rts_rate);
    pTxFwInfo->RtsBandwidth = 0;
    pTxFwInfo->RtsSubcarrier = cb_desc->RTSSC;
    pTxFwInfo->RtsShort =	(pTxFwInfo->RtsHT==0)?(cb_desc->bRTSUseShortPreamble?1:0):(cb_desc->bRTSUseShortGI?1:0);
    //
    // Set Bandwidth and sub-channel settings.
    //
    if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
    {
        if(cb_desc->bPacketBW)
        {
            pTxFwInfo->TxBandwidth = 1;
#ifdef RTL8190P
            pTxFwInfo->TxSubCarrier = 3;
#else
            pTxFwInfo->TxSubCarrier = 0;	//By SD3's Jerry suggestion, use duplicated mode, cosa 04012008
#endif
        }
        else
        {
            pTxFwInfo->TxBandwidth = 0;
            pTxFwInfo->TxSubCarrier = priv->nCur40MhzPrimeSC;
        }
    } else {
        pTxFwInfo->TxBandwidth = 0;
        pTxFwInfo->TxSubCarrier = 0;
    }

    if (0)
    {
   	    TX_FWINFO_T 	Tmp_TxFwInfo;
	    /* 2007/07/25 MH  Copy current TX FW info.*/
	    memcpy((void*)(&Tmp_TxFwInfo), (void*)(pTxFwInfo), sizeof(TX_FWINFO_8190PCI));
	    printk("&&&&&&&&&&&&&&&&&&&&&&====>print out fwinf\n");
	    printk("===>enable fwcacl:%d\n", Tmp_TxFwInfo.EnableCPUDur);
	    printk("===>RTS STBC:%d\n", Tmp_TxFwInfo.RtsSTBC);
	    printk("===>RTS Subcarrier:%d\n", Tmp_TxFwInfo.RtsSubcarrier);
	    printk("===>Allow Aggregation:%d\n", Tmp_TxFwInfo.AllowAggregation);
	    printk("===>TX HT bit:%d\n", Tmp_TxFwInfo.TxHT);
	    printk("===>Tx rate:%d\n", Tmp_TxFwInfo.TxRate);
	    printk("===>Received AMPDU Density:%d\n", Tmp_TxFwInfo.RxAMD);
	    printk("===>Received MPDU Factor:%d\n", Tmp_TxFwInfo.RxMF);
	    printk("===>TxBandwidth:%d\n", Tmp_TxFwInfo.TxBandwidth);
	    printk("===>TxSubCarrier:%d\n", Tmp_TxFwInfo.TxSubCarrier);

        printk("<=====**********************out of print\n");

    }

    /* fill tx descriptor */
    memset((u8*)pdesc,0,12);
    /*DWORD 0*/		
    pdesc->LINIP = 0;
    pdesc->CmdInit = 1;
    pdesc->Offset = sizeof(TX_FWINFO_8190PCI) + 8; //We must add 8!! Emily
    pdesc->PktSize = (u16)skb->len-sizeof(TX_FWINFO_8190PCI);

    /*DWORD 1*/
    pdesc->SecCAMID= 0;
    pdesc->RATid = cb_desc->RATRIndex;


    pdesc->NoEnc = 1;
    pdesc->SecType = 0x0;
    if (cb_desc->bHwSec) {
        static u8 tmp =0;
        if (!tmp) {
            printk("==>================hw sec\n");
            tmp = 1;
        }
        switch (priv->ieee80211->pairwise_key_type) {
            case KEY_TYPE_WEP40:
            case KEY_TYPE_WEP104:
                pdesc->SecType = 0x1;
                pdesc->NoEnc = 0;
                break;
            case KEY_TYPE_TKIP:
                pdesc->SecType = 0x2;
                pdesc->NoEnc = 0;
                break;
            case KEY_TYPE_CCMP:
                pdesc->SecType = 0x3;
                pdesc->NoEnc = 0;
                break;
            case KEY_TYPE_NA:
                pdesc->SecType = 0x0;
                pdesc->NoEnc = 1;
                break;
        }
    }

    //
    // Set Packet ID
    //
    pdesc->PktId = 0x0;

    pdesc->QueueSelect = MapHwQueueToFirmwareQueue(cb_desc->queue_index);
    pdesc->TxFWInfoSize = sizeof(TX_FWINFO_8190PCI);

    pdesc->DISFB = cb_desc->bTxDisableRateFallBack;
    pdesc->USERATE = cb_desc->bTxUseDriverAssingedRate;

    pdesc->FirstSeg =1;
    pdesc->LastSeg = 1;
    pdesc->TxBufferSize = skb->len;

    pdesc->TxBuffAddr = cpu_to_le32(mapping);
}

bool rtl8192_rx_query_status_desc(struct net_device* dev, struct ieee80211_rx_stats*  stats, rx_desc *pdesc, struct sk_buff* skb)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	     stats->bICV = pdesc->ICV;
            stats->bCRC = pdesc->CRC32;
            stats->bHwError = pdesc->CRC32 | pdesc->ICV;

            stats->Length = pdesc->Length;
            if(stats->Length < 24)
                stats->bHwError |= 1;

            if(stats->bHwError) {
                stats->bShift = false;

                if(pdesc->CRC32) {
                    if (pdesc->Length <500)
                        priv->stats.rxcrcerrmin++;
                    else if (pdesc->Length >1000)
                        priv->stats.rxcrcerrmax++;
                    else
                        priv->stats.rxcrcerrmid++;
                }
                return false;
            } else {
                prx_fwinfo pDrvInfo = NULL;


                stats->RxDrvInfoSize = pdesc->RxDrvInfoSize;
                stats->RxBufShift = ((pdesc->Shift)&0x03);
                stats->Decrypted = !pdesc->SWDec;

                pDrvInfo = (rx_fwinfo *)(skb->data + stats->RxBufShift);

                stats->rate = HwRateToMRate90((bool)pDrvInfo->RxHT, (u8)pDrvInfo->RxRate);
                stats->bShortPreamble = pDrvInfo->SPLCP;

                /* it is debug only. It should be disabled in released driver. 
                 * 2007.1.11 by Emily 
                 * */
                UpdateReceivedRateHistogramStatistics8190(dev, stats);

                stats->bIsAMPDU = (pDrvInfo->PartAggr==1);
                stats->bFirstMPDU = (pDrvInfo->PartAggr==1) && (pDrvInfo->FirstAGGR==1);

               stats->TimeStampLow = pDrvInfo->TSFL;
                stats->TimeStampHigh = read_nic_dword(dev, TSFR+4);

                UpdateRxPktTimeStamp8190(dev, stats);

                //
                // Get Total offset of MPDU Frame Body 
                //
                if((stats->RxBufShift + stats->RxDrvInfoSize) > 0)
                    stats->bShift = 1;

                stats->RxIs40MHzPacket = pDrvInfo->BW;
                
                /* ???? */
                TranslateRxSignalStuff819xpci(dev,skb, stats, pdesc, pDrvInfo);

                /* Rx A-MPDU */
                if(pDrvInfo->FirstAGGR==1 || pDrvInfo->PartAggr == 1)
                    RT_TRACE(COMP_RXDESC, "pDrvInfo->FirstAGGR = %d, pDrvInfo->PartAggr = %d\n", 
                            pDrvInfo->FirstAGGR, pDrvInfo->PartAggr);
		   skb_trim(skb, skb->len - 4/*sCrcLng*/);


                stats->packetlength = stats->Length-4;
                stats->fraglength = stats->packetlength;
                stats->fragoffset = 0;
                stats->ntotalfrag = 1;
				return true;		
            	}
}
void rtl8192_rtx_disable(struct net_device *dev, bool reset)
{
	u8 cmd;
	struct r8192_priv *priv = ieee80211_priv(dev);
        int i;

	cmd=read_nic_byte(dev,CMDR);
//	if(!priv->ieee80211->bSupportRemoteWakeUp) {
		write_nic_byte(dev, CMDR, cmd &~ \
				(CR_TE|CR_RE));
//	}
	force_pci_posting(dev);
	mdelay(30);

        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->ieee80211->skb_waitQ [i]);
        }
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->ieee80211->skb_aggQ [i]);
        }


	skb_queue_purge(&priv->skb_queue);
	return;
}
#endif

//by lzm for 92se radio on/off 20090323
#ifdef RTL8192SE_CONFIG_ASPM_OR_D3
bool PlatformSwitchDevicePciASPM(struct net_device *dev, u8	value)
{
	bool bResult = false;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	//PMP_ADAPTER	pDevice = &(Adapter->NdisAdapter);
	u32	ulResult=0;
	u8	Buffer;

	Buffer= value;	
		
	pci_write_config_byte(	priv->pdev,0x80,value);
	
	printk("PlatformSwitchASPM= %x\n", value);
	
	udelay(100);

	return bResult;
}
//
// 2008/12/23 MH When we set 0x01 to enable clk request. Set 0x0 to disable clk req.
//
bool PlatformSwitchClkReq(struct net_device *dev, u8 value)
{
	bool bResult = false;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	//PMP_ADAPTER	pDevice = &(Adapter->NdisAdapter);
	u32	ulResult=0;
	u8	Buffer;

	Buffer= value;	
	
	pci_write_config_byte(priv->pdev,0x81,value);
	printk("PlatformSwitchClkReq = %x\n", value);
		
	bResult = true;
	if(Buffer) //0x01: Enable Clk request
	{
		priv->ClkReqState = true;
	}
	else
	{
		priv->ClkReqState = false;
	}
	
	udelay(100);

	return bResult;
}

//Description: 
//		Disable RTL8192SE ASPM & Disable Pci Bridge ASPM
//2008.12.19 tynli porting from 818xB
void
PlatformDisableASPM(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	//PMP_ADAPTER				pDevice = &(Adapter->NdisAdapter);
	u32					PciCfgAddrPort=0;
	u1					Num4Bytes;
	//PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));//GET_POWER_SAVE_CONTROL(pMgntInfo);

	u8	LinkCtrlReg;
	u16	PciBridgeLinkCtrlReg, ASPMLevel=0;

	// Retrieve original configuration settings.
	LinkCtrlReg = priv->LinkCtrlReg;
	PciBridgeLinkCtrlReg = priv->PciBridgeLinkCtrlReg;

	// Set corresponding value.
	ASPMLevel |= BIT0|BIT1;
//	ASPMLevel |=  PCI_CONFIG_CLK_REQ ;
	LinkCtrlReg &=~ASPMLevel;
	PciBridgeLinkCtrlReg &=~BIT0;

	//When there exists anyone's BusNum, DevNum, and FuncNum that are set to 0xff,
	// we do not execute any action and return. Added by tynli.
	if( (priv->BusNumber == 0xff && priv->DevNumber == 0xff && priv->FuncNumber == 0xff) ||
	     (priv->PciBridgeBusNum == 0xff && priv->PciBridgeDevNum == 0xff && priv->PciBridgeFuncNum == 0xff) )
	{

	}
	else
	{
	//4 //Disable Pci Bridge ASPM 
	PciCfgAddrPort= (priv->PciBridgeBusNum << 16)|(priv->PciBridgeDevNum<< 11)|(priv->PciBridgeFuncNum <<  8)|(1 << 31);
	Num4Bytes = (priv->PciBridgePCIeHdrOffset+0x10)/4;
	// set up address port at 0xCF8 offset field= 0 (dev|vend)
	//NdisRawWritePortUlong((ULONG_PTR)PCI_CONF_ADDRESS , PciCfgAddrPort+(Num4Bytes << 2));
	outl((PciCfgAddrPort+(Num4Bytes << 2)), PCI_CONF_ADDRESS);
	// now grab data port with device|vendor 4 byte dword
	//NdisRawWritePortUchar((ULONG_PTR)PCI_CONF_DATA, Adapter->NdisAdapter.PciBridgeLinkCtrlReg);	
	outb(PciBridgeLinkCtrlReg&0xff, PCI_CONF_DATA);
	RT_TRACE(COMP_POWER, "PlatformDisableASPM():PciBridge BusNumber[%x], DevNumbe[%x], FuncNumber[%x], Write reg[%x] = %x\n",
		priv->PciBridgeBusNum, priv->PciBridgeDevNum, priv->PciBridgeFuncNum, (priv->PciBridgePCIeHdrOffset+0x10), priv->PciBridgeLinkCtrlReg);
		udelay(100);
	}

	PlatformSwitchDevicePciASPM(dev, priv->LinkCtrlReg);
	PlatformSwitchClkReq(dev, 0x0);
	if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_CLK_REQ)
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_CLK_REQ);
	udelay(100);
	/*
	//4 Disable RTL8192SE ASPM 
	PciCfgAddrPort= (pDevice->BusNumber << 16)|(pDevice->DevNumber<< 11)|(pDevice->FuncNumber <<  8)|(1 << 31);
	Num4Bytes = (Adapter->NdisAdapter.PCIeHdrOffset +0x10)/4;
	// set up address port at 0xCF8 offset field= 0 (dev|vend)
	NdisRawWritePortUlong((ULONG_PTR)PCI_CONF_ADDRESS , PciCfgAddrPort+(Num4Bytes << 2));
	// now grab data port with device|vendor 4 byte dword
	NdisRawWritePortUshort((ULONG_PTR)PCI_CONF_DATA, Adapter->NdisAdapter.LinkCtrlReg);
	RT_TRACE(COMP_POWER, DBG_LOUD,("PlatformDisableASPM():PCI BusNumber[%x], DevNumbe[%x], FuncNumber[%x], Write reg[%x] = %x\n",
		pDevice->BusNumber, pDevice->DevNumber, pDevice->FuncNumber, (Adapter->NdisAdapter.PCIeHdrOffset +0x10), Adapter->NdisAdapter.LinkCtrlReg));
		*/

}
//[ASPM]
//Description: 
//		Enable RTL8192SE ASPM & Enable Pci Bridge ASPM for power saving
//		We should follow the sequence to enable RTL8192SE first then enable Pci Bridge ASPM
//		or the system will show bluescreen.
//2008.12.19 tynli porting from 818xB
void PlatformEnableASPM(struct net_device *dev)
{
	//PMP_ADAPTER				pDevice = &(Adapter->NdisAdapter);
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u32	PciCfgAddrPort=0;
	u8	Num4Bytes;
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));
	u16	ASPMLevel = 0;

	//When there exists anyone's BusNum, DevNum, and FuncNum that are set to 0xff,
	// we do not execute any action and return. Added by tynli. 

	// if it is not intel bus then don't enable ASPM.
	if( (priv->BusNumber == 0xff && priv->DevNumber == 0xff && priv->FuncNumber == 0xff) ||
		(priv->PciBridgeBusNum == 0xff && priv->PciBridgeDevNum == 0xff && priv->PciBridgeFuncNum == 0xff) )
	{
		RT_TRACE(COMP_INIT, "PlatformEnableASPM(): Fail to enable ASPM. Cannot find the Bus of PCI(Bridge).\n");
		return;
	}

	ASPMLevel |= priv->RegDevicePciASPMSetting;
	PlatformSwitchDevicePciASPM(dev, (priv->LinkCtrlReg | ASPMLevel));
	if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_CLK_REQ)
	{
		PlatformSwitchClkReq(dev,(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_CLK_REQ) ? 1 : 0);
		RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_CLK_REQ);
	}
	udelay(100);

/*	//4 Enable RTL8192SE ASPM 
	PciCfgAddrPort= (pDevice->BusNumber << 16)|(pDevice->DevNumber<< 11)|(pDevice->FuncNumber <<  8)|(1 << 31);
	Num4Bytes = (Adapter->NdisAdapter.PCIeHdrOffset +0x10)/4;
	// set up address port at 0xCF8 offset field= 0 (dev|vend)
	NdisRawWritePortUlong((ULONG_PTR)PCI_CONF_ADDRESS , PciCfgAddrPort+(Num4Bytes << 2));
	// now grab data port with device|vendor 4 byte dword
	NdisRawWritePortUshort((ULONG_PTR)PCI_CONF_DATA, (Adapter->NdisAdapter.LinkCtrlReg | ASPMLevel));
	RT_TRACE(COMP_INIT, DBG_LOUD,("PlatformEnableASPM():PCI BusNumber[%x], DevNumbe[%x], FuncNumber[%x], Write reg[%x] = %x\n", 
		pDevice->BusNumber, pDevice->DevNumber,  pDevice->FuncNumber, (Adapter->NdisAdapter.PCIeHdrOffset +0x10), (Adapter->NdisAdapter.LinkCtrlReg | ASPMLevel)));
*/
	//4 Enable Pci Bridge ASPM 
	PciCfgAddrPort= (priv->PciBridgeBusNum << 16)|(priv->PciBridgeDevNum<< 11)|(priv->PciBridgeFuncNum <<  8)|(1 << 31);
	Num4Bytes = (priv->NdisAdapter.PciBridgePCIeHdrOffset+0x10)/4;
	// set up address port at 0xCF8 offset field= 0 (dev|vend)
	outl(PciCfgAddrPort+(Num4Bytes << 2), PCI_CONF_ADDRESS);
	// now grab data port with device|vendor 4 byte dword
	outb((priv->PciBridgeLinkCtrlReg | priv->RegHostPciASPMSetting)&0xff,PCI_CONF_DATA, );
	RT_TRACE(COMP_INIT, "PlatformEnableASPM():PciBridge BusNumber[%x], DevNumbe[%x], FuncNumber[%x], Write reg[%x] = %x\n",
		priv->PciBridgeBusNum, priv->PciBridgeDevNum, priv->PciBridgeFuncNum, (priv->PciBridgePCIeHdrOffset+0x10), (priv->PciBridgeLinkCtrlReg | PCI_CONFIG_ASPM));
	udelay(100);
}
//
// 2008/12/26 MH When we exit from D3 we must reset PCI mmory space. Otherwise,
// ASPM and clock request will not work at next time.
//
u32 PlatformResetPciSpace(struct net_device *dev,u8 Value)
{
	u32	bResult = false;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);

	pci_write_config_byte(priv->pdev,0x04,Value);	

	return 1;
	
}
//
// 2008/12/23 MH For 9x series D3 mode switch, we do not support D3 spin lock.
// Because we only permit to enter D3 mode and exit when we switch RF state. If 
// state is the same, we will not switch D3 mode.
//
bool PlatformSetPMCSR(struct net_device *dev,u8 value,bool bTempSetting)
{
	bool bResult = false;
	struct r8192_priv *priv = (struct r8192_priv *)ieee80211_priv(dev);
	u32			ulResult=0;
	u8			Buffer;
	bool 		bActuallySet=false, bSetFunc=false;
	unsigned long flag;
	// We do not check if we can Do IO. If we can perform the code, it means we can change
	// configuration space for PCI-(E)

	Buffer= value;

	spin_lock_irqsave(&priv->D3_lock,flag);
	if(bTempSetting) //temp open 
	{
		if(Buffer==0x00) //temp leave D3
		{
			priv->LeaveD3Cnt++;

			//if(pDevice->LeaveD3Cnt == 1)
			{
				bActuallySet =true;
			}
		}
		else //0x03: enter D3
		{
			priv->LeaveD3Cnt--;

			if(priv->LeaveD3Cnt == 0) 
			{
				bActuallySet=true;
			}
		}
	}
	else
	{
		priv->LeaveD3Cnt=0;
		bActuallySet=true;
		bSetFunc=true;
	}


	if(bActuallySet)
	{
		if(Buffer)
		{
			// Enable clock request to lower down  the current.
			PlatformSwitchClkReq(dev, 0x01);
		}
		else
		{
			PlatformSwitchClkReq(dev, 0x00);
		}
		
		pci_write_config_byte(priv->pdev,0x44,Buffer);

		RT_TRACE(COMP_POWER, "PlatformSetPMCSR(): D3(value: %d)\n", Buffer);

		bResult = true;

		if(!Buffer)
		{
			//
			// Reset PCIE to simulate host behavior. to prevent ASPM power
			// save problem.
			//
			PlatformResetPciSpace(dev, 0x06);
			PlatformResetPciSpace(dev, 0x07);
		}

		if(bSetFunc)
		{
			if(Buffer) //0x03: enter D3
			{
#ifdef TO_DO_LIST
				RT_DISABLE_FUNC(Adapter, DF_IO_D3_BIT);
#endif
			}
			else
			{
#ifdef TO_DO_LIST
				RT_ENABLE_FUNC(Adapter, DF_IO_D3_BIT);
#endif
			}
		}
		
	}
	spin_unlock_irqrestore(&priv->D3_lock,flag);
	return bResult;
}

#endif

#if 1
void check_rfctrl_gpio_timer(unsigned long data)
{
	struct r8192_priv* priv = ieee80211_priv((struct net_device *)data);
	//struct net_device* dev = (struct net_device*)data;

        //printk("===========>%s():RFChangeInProgress:%d\n",__func__, priv->RFChangeInProgress);
        //return;

	priv->polling_timer_on = 1;//add for S3/S4

	//if(priv->driver_upping == 0){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20) 
        queue_delayed_work(priv->priv_wq,&priv->gpio_change_rf_wq,0);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
        schedule_task(&priv->gpio_change_rf_wq);
#else
        queue_work(priv->priv_wq,&priv->gpio_change_rf_wq);
#endif
        //}

	mod_timer(&priv->gpio_polling_timer, jiffies + MSECS(IEEE80211_WATCH_DOG_TIME));
}
#endif
//added by amy for IPS 090407
bool NicIFEnableNIC(struct net_device* dev)
{
	bool init_status = true;
	struct r8192_priv* priv = ieee80211_priv(dev);
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));

	// <1> Reset memory: descriptor, buffer,..
	//NicIFResetMemory(Adapter);

	// <2> Enable Adapter
	printk("===========>%s()\n",__FUNCTION__);
	init_status = priv->ops->initialize_adapter(dev);
	if(init_status != true)
	{
		RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n",__FUNCTION__);
		return -1;
	}
	RT_TRACE(COMP_INIT, "start adapter finished\n");
	RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);

	// <3> Enable Interrupt
	rtl8192_irq_enable(dev);
	priv->bdisable_nic = false;
	return init_status;
}
extern void PHY_SetRtl8192seRfHalt(struct net_device* dev)
{
	struct r8192_priv* priv = ieee80211_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->ieee80211->PowerSaveControl));

	u8 u1bTmp;
	
	/*  03/27/2009 MH Add description. Power save for BB/RF */
	printk("PHY_SetRtl8192seRfHalt save BB/RF\n");
	u1bTmp = read_nic_byte(dev, LDOV12D_CTRL);
	u1bTmp |= BIT0;
	write_nic_byte(dev, LDOV12D_CTRL, u1bTmp);
	write_nic_byte(dev, SPS1_CTRL, 0x0);
	write_nic_byte(dev, TXPAUSE, 0xFF);
	write_nic_word(dev, CMDR, 0x57FC);
	udelay(100);
	write_nic_word(dev, CMDR, 0x77FC);
	write_nic_byte(dev, PHY_CCA, 0x0);
	udelay(10);
	write_nic_word(dev, CMDR, 0x37FC);
	udelay(10);
	write_nic_word(dev, CMDR, 0x77FC);			
	udelay(10);
	write_nic_word(dev, CMDR, 0x57FC);
	write_nic_word(dev, CMDR, 0x0000);
	u1bTmp = read_nic_byte(dev, (SYS_CLKR + 1));
	
	/*  03/27/2009 MH Add description. After switch control path. register
	    after page1 will be invisible. We can not do any IO for register>0x40.
	    After resume&MACIO reset, we need to remember previous reg content. */
	if(u1bTmp & BIT7)
	{
		u1bTmp &= ~(BIT6 | BIT7);					
		if(!HalSetSysClk8192SE(dev, u1bTmp))
		{
			printk("Switch ctrl path fail\n");
			return;
		}
	}

	printk("Save MAC\n");
	/*  03/27/2009 MH Add description. Power save for MAC */
	if(priv->ieee80211->RfOffReason==RF_CHANGE_BY_IPS )
	{
		/*  03/27/2009 MH For enable LED function. */
		printk("Save except led\n");
		write_nic_byte(dev, 0x03, 0xF9);
	}
	else		// SW/HW radio off or halt adapter!! For example S3/S4
	{
		// 2009/03/27 MH LED function disable. Power range is about 8mA now.
		printk("Save max pwr\n");
		write_nic_byte(dev, 0x03, 0x71);
	}
	write_nic_byte(dev, 0x09, 0x70);
	write_nic_byte(dev, 0x29, 0x68);
	write_nic_byte(dev, 0x28, 0x00);
	write_nic_byte(dev, 0x20, 0x50);
	write_nic_byte(dev, 0x26, 0x0E);
	RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
	udelay(100);
	
	udelay(20);
	
}	// PHY_SetRtl8192seRfHalt

RT_STATUS NicIFDisableNIC(struct net_device* dev)
{
	RT_STATUS	status = RT_STATUS_SUCCESS;
	struct r8192_priv* priv = ieee80211_priv(dev);
	u8 tmp_state = 0;
	// <1> Disable Interrupt
	printk("=========>%s()\n",__FUNCTION__);
	tmp_state = priv->ieee80211->state;
	ieee80211_softmac_stop_protocol(priv->ieee80211,false);
	priv->ieee80211->state = tmp_state;
	rtl8192_cancel_deferred_work(priv);
	rtl8192_irq_disable(dev);

	// <2> Stop all timer

	// <3> Disable Adapter
	PHY_SetRtl8192seRfHalt(dev);
	priv->bdisable_nic = true;	
	//NicIFHaltAdapter(Adapter, TRUE);

	return status;
}
//added by amy for LPS 090407 end
/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/
module_init(rtl8192_pci_module_init);
module_exit(rtl8192_pci_module_exit);
