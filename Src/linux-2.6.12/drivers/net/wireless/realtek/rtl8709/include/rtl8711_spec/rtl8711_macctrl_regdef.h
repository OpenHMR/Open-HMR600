

#ifndef __RTL8711_MACCTRL_REGDEF_H__
#define __RTL8711_MACCTRL_REGDEF_H__



/* -----------------------------------------------------------------------------

					For WLAN Controller

------------------------------------------------------------------------------*/
#define IDR				(RTL8711_WLANCTRL_ + 0x0000)
#define MAR				(RTL8711_WLANCTRL_ + 0x0008)
#define BSSIDR			(RTL8711_WLANCTRL_ + 0x0010)
#define TSFERR			(RTL8711_WLANCTRL_ + 0x0020)
#define TUBASE			(RTL8711_WLANCTRL_ + 0x0024)
#define SLOT_TIME			(RTL8711_WLANCTRL_ + 0x0026)
#define USTUNE			(RTL8711_WLANCTRL_ + 0x0027)
#define TSFTR				(RTL8711_WLANCTRL_ + 0x0028)
#define SIFS				(RTL8711_WLANCTRL_ + 0x0030)
#define DIFS				(RTL8711_WLANCTRL_ + 0x0034)
#define PIFS				(RTL8711_WLANCTRL_ + 0x0035)
#define EIFS				(RTL8711_WLANCTRL_ + 0x0036)
#define ACKTO			(RTL8711_WLANCTRL_ + 0x0037)
#define BCNITV			(RTL8711_WLANCTRL_ + 0x0038)
#define ATIMWND			(RTL8711_WLANCTRL_ + 0x003A)
#define BINTRITV			(RTL8711_WLANCTRL_ + 0x003C)
#define ATIMTRITV			(RTL8711_WLANCTRL_ + 0x0040)
#define TSFTUNE			(RTL8711_WLANCTRL_ + 0x0042)
#define TXOPSTALLTIME		(RTL8711_WLANCTRL_ + 0x0044)
#define TX_AGC_ANT_CTRL	(RTL8711_WLANCTRL_ + 0x0045)
#define CCK_TX_AGC		(RTL8711_WLANCTRL_ + 0x0046)
#define OFDM_TX_AGC		(RTL8711_WLANCTRL_ + 0x0047)

//------------ Rate Control Register  ---------
#define ARFR				(RTL8711_WLANCTRL_ + 0x0048)
#define BRSR				(RTL8711_WLANCTRL_ + 0x004C)
#define RATE_FALBACK_CTRL	(RTL8711_WLANCTRL_ + 0x004E)
#define TSSISTARTTIME		(RTL8711_WLANCTRL_ + 0x004F)


//------------ MAC Command Register  ---------
#define TCR				(RTL8711_WLANCTRL_ + 0x0050)
#define RCR				(RTL8711_WLANCTRL_ + 0x0054)
#define CR				(RTL8711_WLANCTRL_ + 0x0058)
#define CONFIG_REG0		(RTL8711_WLANCTRL_ + 0x0059)
#define CONFIG_REG1		(RTL8711_WLANCTRL_ + 0x005A)
#define TPPOLL			(RTL8711_WLANCTRL_ + 0x005B)
#define MSR				(RTL8711_WLANCTRL_ + 0x005C)
#define TESTR				(RTL8711_WLANCTRL_ + 0x005D)
#define BTCOEXCFG			(RTL8711_WLANCTRL_ + 0x005E)
#define CCACRSTXSTOP		(RTL8711_WLANCTRL_ + 0x005F)

//------------ MAC FIFO and Status Register  ---------
#define FREETX			(RTL8711_WLANCTRL_ + 0x0060)
#define FREETXCTRL		(RTL8711_WLANCTRL_ + 0x0064)
#define RXFFSTATUS		(RTL8711_WLANCTRL_ + 0x0068)
#define RXFFCMD			(RTL8711_WLANCTRL_ + 0x006A)
#define TXFFERR			(RTL8711_WLANCTRL_ + 0x006C)

//------------ Security Control Register  ---------
#define CAM_WE			(BIT(16))
#define CAM_POLLING		(BIT(31))
#define CAM_VALID			(BIT(15))
#define CAM_INVALID		(~BIT(15))
#define CAMENTRYSZ		6
#define MAXCAMINX		64

#define CAM_RW			(RTL8711_WLANCTRL_ + 0x0070)
#define CAM_WRDATA		(RTL8711_WLANCTRL_ + 0x0074)
#define CAM_DEBUG		(RTL8711_WLANCTRL_ + 0x007C)
#define WPA_CONFIG		(RTL8711_WLANCTRL_ + 0x0080)
#define AES_MASK			(RTL8711_WLANCTRL_ + 0x0082)
#define AES_MASK_QC		(RTL8711_WLANCTRL_ + 0x0084)
#define AES_MASK_SC		(RTL8711_WLANCTRL_ + 0x0086)

//------------ MAC Command Register  ---------
#define TXCMD_FECTCHPTR	(RTL8711_WLANCTRL_ + 0x0098)

//------------ TX/RX Turbo Mode Control Register  ---------
#define VO_TB_RT_PARAM	(RTL8711_WLANCTRL_ + 0x00A0)
#define VI_TB_RT_PARAM		(RTL8711_WLANCTRL_ + 0x00A4)
#define BE_TB_RT_PARAM	(RTL8711_WLANCTRL_ + 0x00A8)
#define BK_TB_RT_PARAM	(RTL8711_WLANCTRL_ + 0x00AC)

//------------ FWMAC Register  ---------
#define RXFLTMAP0			(RTL8711_WLANCTRL_ + 0x00B0)
#define RXFLTCTRL0		(RTL8711_WLANCTRL_ + 0x00B4)
#define RXFLTMAP1			(RTL8711_WLANCTRL_ + 0x00B8)
#define RXFLTCTRL1		(RTL8711_WLANCTRL_ + 0x00BC)
#define RXFLTMAP2			(RTL8711_WLANCTRL_ + 0x00C0)
#define RXFLTCTRL2		(RTL8711_WLANCTRL_ + 0x00C4)

#define MACLBK			(RTL8711_WLANCTRL_ + 0x00D0)	//0x02: delay MAC loopback

//------------ QoS(EDCA) Control Register  ---------
#define AC_VO_PARAM		(RTL8711_WLANCTRL_ + 0x0100)
#define AC_VI_PARAM		(RTL8711_WLANCTRL_ + 0x0108)
#define AC_BE_PARAM		(RTL8711_WLANCTRL_ + 0x0110)
#define AC_BK_PARAM		(RTL8711_WLANCTRL_ + 0x0118)
#define BM_MGT_PARAM		(RTL8711_WLANCTRL_ + 0x0120)
#define BE_RETRY			(RTL8711_WLANCTRL_ + 0x0124)
#define BK_RETRY			(RTL8711_WLANCTRL_ + 0x0126)
#define VI_RETRY			(RTL8711_WLANCTRL_ + 0x0128)
#define VO_RETRY			(RTL8711_WLANCTRL_ + 0x012A)
#define TS_RETRY			(RTL8711_WLANCTRL_ + 0x012C)
#define MGT_RETRY			(RTL8711_WLANCTRL_ + 0x012E)
#define EDCAAP			(RTL8711_WLANCTRL_ + 0x0130)
#define BE_ADTM			(RTL8711_WLANCTRL_ + 0x0134)
#define BE_USED_TIME		(RTL8711_WLANCTRL_ + 0x0138)
#define BK_ADTM			(RTL8711_WLANCTRL_ + 0x013C)
#define BK_USED_TIME		(RTL8711_WLANCTRL_ + 0x0140)
#define VI_ADTM			(RTL8711_WLANCTRL_ + 0x0144)
#define VI_USED_TIME		(RTL8711_WLANCTRL_ + 0x0148)
#define VO_ADTM			(RTL8711_WLANCTRL_ + 0x014C)
#define VO_USED_TIME		(RTL8711_WLANCTRL_ + 0x0150)
#define ACCTRL			(RTL8711_WLANCTRL_ + 0x0154)
#define NAVCTRL			(RTL8711_WLANCTRL_ + 0x0158)

//------------ QoS(HCCA) Control Register  ---------
#define TS_EVENT_TIMER		(RTL8711_WLANCTRL_ + 0x0160)
#define TS_TXNUM			(RTL8711_WLANCTRL_ + 0x0162)
#define PIGGYBACK_CTRL	(RTL8711_WLANCTRL_ + 0x0163)
#define TS_SERVICE_TSF		(RTL8711_WLANCTRL_ + 0x0164)
#define TS_CTRL_REG		(RTL8711_WLANCTRL_ + 0x0168)
#define EVENT_TIMER		(RTL8711_WLANCTRL_ + 0x016C)
#define HCCA_MAC_CMD	(RTL8711_WLANCTRL_ + 0x0170)	
#define CF_POLLED			(RTL8711_WLANCTRL_ + 0x0178)
#define QOS_NULLED		(RTL8711_WLANCTRL_ + 0x017C)
#define MAX_SI			(RTL8711_WLANCTRL_ + 0x0180)

//------------ BALT Control Register
#define R_BALT_CONFIG		(RTL8711_WLANCTRL_ + 0x018A)
#define R_BALT_ENTRY		(RTL8711_WLANCTRL_ + 0x018C)
#define R_BALT_SQN		(RTL8711_WLANCTRL_ + 0x018E)
#define R_BALT_BITMAP		(RTL8711_WLANCTRL_ + 0x0190)


//------------ PHY Control Register  ---------
#define ANA_PARM0		(RTL8711_WLANCTRL_ + 0x0200)
#define ANA_PARM1		(RTL8711_WLANCTRL_ + 0x0204)
#define ANA_PARM2		(RTL8711_WLANCTRL_ + 0x0228)
#define ANA_PARM3		(RTL8711_WLANCTRL_ + 0x022C)

#define RF_SW_CONFIG		(RTL8711_WLANCTRL_ + 0x0230)

#define PhyAddr			(RTL8711_WLANCTRL_ + 0x0234)
#define PhyDataW			(RTL8711_WLANCTRL_ + 0x0235)
#define PhyDataR			(RTL8711_WLANCTRL_ + 0x0236)
#define PhyASel			(RTL8711_WLANCTRL_ + 0x0237)

#define RFPinsOutput		(RTL8711_WLANCTRL_ + 0x0238)
#define RFPinsEnable		(RTL8711_WLANCTRL_ + 0x023A)
#define RFPinsSelect		(RTL8711_WLANCTRL_ + 0x023C)
#define RFPinsInput			(RTL8711_WLANCTRL_ + 0x023E)

#define RF_PARA			(RTL8711_WLANCTRL_ + 0x0240)
#define RF_TIMING			(RTL8711_WLANCTRL_ + 0x0244)

#define GPO				(RTL8711_WLANCTRL_ + 0x0248)
#define GPE				(RTL8711_WLANCTRL_ + 0x0249)
#define GPCFG				(RTL8711_WLANCTRL_ + 0x024A)
#define GPI				(RTL8711_WLANCTRL_ + 0x024B)

#define PHYMUX			(RTL8711_WLANCTRL_ + 0x024C)
#define CCKPHYMUX		(RTL8711_WLANCTRL_ + 0x0250)

#define PWRDNCFG			(RTL8711_WLANCTRL_ + 0x0254)
#define WAKEUPCFG		(RTL8711_WLANCTRL_ + 0x0258)

#define HWCFG			(RTL8711_WLANCTRL_ + 0x025C)
#define SWCFG			(RTL8711_WLANCTRL_ + 0x0260)

#define TX_AGC_CONTROL	(RTL8711_WLANCTRL_ + 0x0270)
#define CCK_AGC_CONTROL	(RTL8711_WLANCTRL_ + 0x0271)
#define OFDM_AGC_CONTROL	(RTL8711_WLANCTRL_ + 0x0272)

#define ANTSEL			(RTL8711_WLANCTRL_ + 0x0273)

#define ANA_WE			0x40


/*------------------------------------------------------------------------------

						For WLAN FIFO/TXCMD/RXCMD

------------------------------------------------------------------------------*/


#define	VOCMD			(RTL8711_WLANFF_ + 0x0000)
#define VOCMD_ENTRYCNT	(8)

#define	VICMD			(RTL8711_WLANFF_ + 0x0080)
#define VICMD_ENTRYCNT	(8)


#define	BECMD			(RTL8711_WLANFF_ + 0x0100)
#define BECMD_ENTRYCNT	(8)

#define	BKCMD			(RTL8711_WLANFF_ + 0x0180)
#define BKCMD_ENTRYCNT	(8)

#define	TSCMD			(RTL8711_WLANFF_ + 0x0200)
#define TSCMD_ENTRYCNT	(4)


#define	MGTCMD			(RTL8711_WLANFF_ + 0x0240)
#define MGTCMD_ENTRYCNT	(4)


#define	BMCMD			(RTL8711_WLANFF_ + 0x0280)
#define BMCMD_ENTRYCNT	(4)

#define	BCNCMD			(RTL8711_WLANFF_ + 0x02c0)
#define BCNCMD_ENTRYCNT	(1)

#define	RXCMD0			(RTL8711_WLANFF_ + 0x0300)

#define	RXCMD1			(RTL8711_WLANFF_ + 0x0400)

#define	VOFFWP			(RTL8711_WLANFF_ + 0x0440)
#define	VIFFWP			(RTL8711_WLANFF_ + 0x0444)
#define	BEFFWP			(RTL8711_WLANFF_ + 0x0448)
#define	BKFFWP			(RTL8711_WLANFF_ + 0x044c)
#define	TSFFWP			(RTL8711_WLANFF_ + 0x0450)
#define	MGTFFWP		(RTL8711_WLANFF_ + 0x0454)
#define	BMFFWP			(RTL8711_WLANFF_ + 0x0458)
#define	BCNFFWP		(RTL8711_WLANFF_ + 0x045C)

#define	RXFF0RP			(RTL8711_WLANFF_ + 0x0460)
#define	RXFF1RP			(RTL8711_WLANFF_ + 0x0464)

#define RXCMD0_ENTRYCNT (16)
#define RXCMD1_ENTRYCNT (4)


#endif // __RTL8711_MACCTRL_REGDEF_H__

