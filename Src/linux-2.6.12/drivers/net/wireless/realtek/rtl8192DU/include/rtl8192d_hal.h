#ifndef __RTL8192D_HAL_H__
#define __RTL8192D_HAL_H__

#include "Hal8192CPhyReg.h"
#include "Hal8192CPhyCfg.h"
#include "Hal8192CDM.h"

#if (DEV_BUS_TYPE == PCI_INTERFACE)

	#define RTL819X_DEFAULT_RF_TYPE			RF_2T2R
	//#define RTL819X_DEFAULT_RF_TYPE			RF_1T2R
	#define RTL819X_TOTAL_RF_PATH				2

	//2TODO:  The following need to check!!
	#define	RTL8192C_FW_IMG					"rtl8192CE\\rtl8192cfw.bin"
	#define	RTL8188C_FW_IMG					"rtl8192CE\\rtl8192cfw.bin"

	#define RTL8188C_PHY_REG					"rtl8192CE\\PHY_REG_1T.txt"
	#define RTL8188C_PHY_RADIO_A				"rtl8192CE\\radio_a_1T.txt"
	#define RTL8188C_PHY_RADIO_B				"rtl8192CE\\radio_b_1T.txt"
	#define RTL8188C_AGC_TAB					"rtl8192CE\\AGC_TAB_1T.txt"
	#define RTL8188C_PHY_MACREG				"rtl8192CE\\MACREG_1T.txt"

	#define RTL8192C_PHY_REG					"rtl8192CE\\PHY_REG_2T.txt"
	#define RTL8192C_PHY_RADIO_A				"rtl8192CE\\radio_a_2T.txt"
	#define RTL8192C_PHY_RADIO_B				"rtl8192CE\\radio_b_2T.txt"
	#define RTL8192C_AGC_TAB					"rtl8192CE\\AGC_TAB_2T.txt"
	#define RTL8192C_PHY_MACREG				"rtl8192CE\\MACREG_2T.txt"

	#define RTL819X_PHY_MACPHY_REG			"rtl8192CE\\MACPHY_reg.txt"
	#define RTL819X_PHY_MACPHY_REG_PG		"rtl8192CE\\MACPHY_reg_PG.txt"
	#define RTL819X_PHY_MACREG				"rtl8192CE\\MAC_REG.txt"
	#define RTL819X_PHY_REG					"rtl8192CE\\PHY_REG.txt"
	#define RTL819X_PHY_REG_1T2R				"rtl8192CE\\PHY_REG_1T2R.txt"
	#define RTL819X_PHY_REG_to1T1R				"rtl8192CE\\phy_to1T1R_a.txt"
	#define RTL819X_PHY_REG_to1T2R				"rtl8192CE\\phy_to1T2R.txt"
	#define RTL819X_PHY_REG_to2T2R				"rtl8192CE\\phy_to2T2R.txt"
	#define RTL819X_PHY_REG_PG					"rtl8192CE\\PHY_REG_PG.txt"
	#define RTL819X_AGC_TAB					"rtl8192CE\\AGC_TAB.txt"
	#define RTL819X_PHY_RADIO_A				"rtl8192CE\\radio_a.txt"
	#define RTL819X_PHY_RADIO_A_1T			"rtl8192CE\\radio_a_1t.txt"
	#define RTL819X_PHY_RADIO_A_2T			"rtl8192CE\\radio_a_2t.txt"
	#define RTL819X_PHY_RADIO_B				"rtl8192CE\\radio_b.txt"
	#define RTL819X_PHY_RADIO_B_GM			"rtl8192CE\\radio_b_gm.txt"
	#define RTL819X_PHY_RADIO_C				"rtl8192CE\\radio_c.txt"
	#define RTL819X_PHY_RADIO_D				"rtl8192CE\\radio_d.txt"
	#define RTL819X_EEPROM_MAP				"rtl8192CE\\8192ce.map"
	#define RTL819X_EFUSE_MAP					"rtl8192CE\\8192ce.map"

	#if (HARDWARE_TYPE_IS_RTL8192D == 1)

	// The file name "_2T" is for 92CE, "_1T"  is for 88CE. Modified by tynli. 2009.11.24.
	#define Rtl819XFwImageArray					Rtl8192DEFwImgArray
	#define Rtl819XMAC_Array					Rtl8192DEMAC_2T_Array
	#define Rtl819XAGCTAB_5GArray				Rtl8192DEAGCTAB_5GArray
	#define Rtl819XAGCTAB_2GArray				Rtl8192DEAGCTAB_2GArray
	#define Rtl819XPHY_REG_2TArray				Rtl8192DEPHY_REG_2TArray			
	#define Rtl819XPHY_REG_1TArray				Rtl8192DEPHY_REG_1TArray
	#define Rtl819XRadioA_2TArray				Rtl8192DERadioA_2TArray
	#define Rtl819XRadioA_1TArray				Rtl8192DERadioA_1TArray
	#define Rtl819XRadioB_2TArray				Rtl8192DERadioB_2TArray
	#define Rtl819XRadioB_1TArray				Rtl8192DERadioB_1TArray
	#define Rtl819XPHY_REG_Array_PG 			Rtl8192DEPHY_REG_Array_PG
	//#define Rtl819XPHY_REG_Array_MP 			Rtl8192DEPHY_REG_Array_MP
	#define Rtl819XAGCTAB_2TArray				Rtl8192DEAGCTAB_2TArray
	#define Rtl819XAGCTAB_1TArray				Rtl8192DEAGCTAB_1TArray

	#else

	// The file name "_2T" is for 92CE, "_1T"  is for 88CE. Modified by tynli. 2009.11.24.
	#define Rtl819XFwImageArray					Rtl8192CEFwImgArray
	#define Rtl819XMAC_Array					Rtl8192CEMAC_2T_Array
	#define Rtl819XAGCTAB_2TArray				Rtl8192CEAGCTAB_2TArray
	#define Rtl819XAGCTAB_1TArray				Rtl8192CEAGCTAB_1TArray
	#define Rtl819XPHY_REG_2TArray				Rtl8192CEPHY_REG_2TArray			
	#define Rtl819XPHY_REG_1TArray				Rtl8192CEPHY_REG_1TArray
	#define Rtl819XRadioA_2TArray				Rtl8192CERadioA_2TArray
	#define Rtl819XRadioA_1TArray				Rtl8192CERadioA_1TArray
	#define Rtl819XRadioB_2TArray				Rtl8192CERadioB_2TArray
	#define Rtl819XRadioB_1TArray				Rtl8192CERadioB_1TArray
	#define Rtl819XPHY_REG_Array_PG 			Rtl8192CEPHY_REG_Array_PG
	#define Rtl819XPHY_REG_Array_MP 			Rtl8192CEPHY_REG_Array_MP

	#define Rtl819XAGCTAB_5GArray				Rtl8192CEAGCTAB_5GArray
	#define Rtl819XAGCTAB_2GArray				Rtl8192CEAGCTAB_2GArray

	#endif

#elif (DEV_BUS_TYPE == USB_INTERFACE)

	#if (HARDWARE_TYPE_IS_RTL8192D == 1)
	#include "Hal8192DUHWImg.h"			
	#else
	#include "Hal8192CUHWImg.h"
	#endif

	//2TODO: We should define 8192S firmware related macro settings here!!
	#define RTL819X_DEFAULT_RF_TYPE			RF_1T2R
	#define RTL819X_TOTAL_RF_PATH			2

	//2TODO:  The following need to check!!
	#define	RTL8192C_FW_IMG						"rtl8192CU\\rtl8192cfw.bin"
	#define	RTL8188C_FW_IMG						"rtl8192CU\\rtl8192cfw.bin"
	#define	RTL8192D_FW_IMG						"rtl8192DU\\rtl8192dfw.bin"
	//#define RTL819X_FW_BOOT_IMG   				"rtl8192CU\\boot.img"
	//#define RTL819X_FW_MAIN_IMG				"rtl8192CU\\main.img"
	//#define RTL819X_FW_DATA_IMG				"rtl8192CU\\data.img"

	#define RTL8188C_PHY_REG					"rtl8192CU\\PHY_REG_1T.txt"	
	#define RTL8188C_PHY_RADIO_A				"rtl8192CU\\radio_a_1T.txt"
	#define RTL8188C_PHY_RADIO_B				"rtl8192CU\\radio_b_1T.txt"	
	#define RTL8188C_AGC_TAB					"rtl8192CU\\AGC_TAB_1T.txt"
	#define RTL8188C_PHY_MACREG					"rtl8192CU\\MAC_REG.txt"
	
	#define RTL8192C_PHY_REG					"rtl8192CU\\PHY_REG.txt"	
	#define RTL8192C_PHY_RADIO_A				"rtl8192CU\\radio_a.txt"
	#define RTL8192C_PHY_RADIO_B				"rtl8192CU\\radio_b.txt"	
	#define RTL8192C_AGC_TAB					"rtl8192CU\\AGC_TAB.txt"
	#define RTL8192C_PHY_MACREG					"rtl8192CU\\MAC_REG.txt"

	//For 92DU
	#define RTL8192D_PHY_REG					"rtl8192DU\\PHY_REG.txt"
	#define RTL8192D_AGC_TAB				"rtl8192DU\\AGC_TAB.txt"
	#define RTL8192D_AGC_TAB_2G				"rtl8192DU\\AGC_TAB_2G.txt"
	#define RTL8192D_AGC_TAB_5G				"rtl8192DU\\AGC_TAB_5G.txt"
	#define RTL8192D_PHY_RADIO_A				"rtl8192DU\\radio_a.txt"
	#define RTL8192D_PHY_RADIO_B				"rtl8192DU\\radio_b.txt"
	#define RTL8192D_PHY_MACREG				"rtl8192DU\\MAC_REG.txt"
		
	#define RTL819X_PHY_MACPHY_REG				"rtl8192CU\\MACPHY_reg.txt"
	#define RTL819X_PHY_MACPHY_REG_PG			"rtl8192CU\\MACPHY_reg_PG.txt"
	#define RTL819X_PHY_MACREG					"rtl8192CU\\MAC_REG.txt"
	#define RTL819X_PHY_REG						"rtl8192CU\\PHY_REG.txt"
	#define RTL819X_PHY_REG_1T2R				"rtl8192CU\\PHY_REG_1T2R.txt"
	#define RTL819X_PHY_REG_to1T1R				"rtl8192CU\\phy_to1T1R_a.txt"
	#define RTL819X_PHY_REG_to1T2R				"rtl8192CU\\phy_to1T2R.txt"
	#define RTL819X_PHY_REG_to2T2R				"rtl8192CU\\phy_to2T2R.txt"
	#define RTL819X_PHY_REG_PG					"rtl8192CU\\PHY_REG_PG.txt"
	#define RTL819X_PHY_REG_MP 					"rtl8192CU\\PHY_REG_MP.txt" 		

	#define RTL819X_AGC_TAB						"rtl8192CU\\AGC_TAB.txt"
	#define RTL819X_PHY_RADIO_A					"rtl8192CU\\radio_a.txt"
	#define RTL819X_PHY_RADIO_B					"rtl8192CU\\radio_b.txt"			
	#define RTL819X_PHY_RADIO_B_GM				"rtl8192CU\\radio_b_gm.txt"	
	#define RTL819X_PHY_RADIO_C					"rtl8192CU\\radio_c.txt"
	#define RTL819X_PHY_RADIO_D					"rtl8192CU\\radio_d.txt"
	#define RTL819X_EEPROM_MAP					"rtl8192CU\\8192cu.map"
	#define RTL819X_EFUSE_MAP					"rtl8192CU\\8192cu.map"
	#define RTL819X_PHY_RADIO_A_1T				"rtl8192CU\\radio_a_1t.txt"
	#define RTL819X_PHY_RADIO_A_2T				"rtl8192CU\\radio_a_2t.txt"

	#if (HARDWARE_TYPE_IS_RTL8192D == 1)
	// The file name "_2T" is for 92CU, "_1T"  is for 88CU. Modified by tynli. 2009.11.24.
	#define Rtl819XFwImageArray					Rtl8192DUFwImgArray
	#define Rtl819XMAC_Array					Rtl8192DUMAC_2T_Array
	#define Rtl819XAGCTAB_5GArray				Rtl8192DUAGCTAB_5GArray
	#define Rtl819XAGCTAB_2GArray				Rtl8192DUAGCTAB_2GArray
	#define Rtl819XPHY_REG_2TArray				Rtl8192DUPHY_REG_2TArray			
	#define Rtl819XPHY_REG_1TArray				Rtl8192DUPHY_REG_1TArray
	#define Rtl819XRadioA_2TArray				Rtl8192DURadioA_2TArray
	#define Rtl819XRadioA_1TArray				Rtl8192DURadioA_1TArray
	#define Rtl819XRadioB_2TArray				Rtl8192DURadioB_2TArray
	#define Rtl819XRadioB_1TArray				Rtl8192DURadioB_1TArray
	#define Rtl819XPHY_REG_Array_PG 			Rtl8192DUPHY_REG_Array_PG
	//#define Rtl819XPHY_REG_Array_MP 			Rtl8192DUPHY_REG_Array_MP

	#define Rtl819XAGCTAB_2TArray				Rtl8192DUAGCTAB_2TArray
	#define Rtl819XAGCTAB_1TArray				Rtl8192DUAGCTAB_1TArray

	#else
	// The file name "_2T" is for 92CU, "_1T"  is for 88CU. Modified by tynli. 2009.11.24.
	#define Rtl819XFwImageArray					Rtl8192CUFwImgArray
	#define Rtl819XMAC_Array					Rtl8192CUMAC_2T_Array
	#define Rtl819XAGCTAB_2TArray				Rtl8192CUAGCTAB_2TArray
	#define Rtl819XAGCTAB_1TArray				Rtl8192CUAGCTAB_1TArray
	#define Rtl819XPHY_REG_2TArray				Rtl8192CUPHY_REG_2TArray			
	#define Rtl819XPHY_REG_1TArray				Rtl8192CUPHY_REG_1TArray
	#define Rtl819XRadioA_2TArray				Rtl8192CURadioA_2TArray
	#define Rtl819XRadioA_1TArray				Rtl8192CURadioA_1TArray
	#define Rtl819XRadioB_2TArray				Rtl8192CURadioB_2TArray
	#define Rtl819XRadioB_1TArray				Rtl8192CURadioB_1TArray
	#define Rtl819XPHY_REG_Array_PG 			Rtl8192CUPHY_REG_Array_PG
	#define Rtl819XPHY_REG_Array_MP 			Rtl8192CUPHY_REG_Array_MP
	#define Rtl819XAGCTAB_5GArray				Rtl8192CUAGCTAB_5GArray
	#define Rtl819XAGCTAB_2GArray				Rtl8192CUAGCTAB_2GArray

	#endif

#endif


enum RTL871X_HCI_TYPE {

	RTL8192C_SDIO,
	RTL8192C_USB,
	RTL8192C_PCIE
};


#define FW_8192C_SIZE					0x4000
#define FW_8192C_START_ADDRESS		0x1000
#define FW_8192C_END_ADDRESS		0x3FFF

#define MAX_PAGE_SIZE			4096	// @ page : 4k bytes

#define IS_FW_HEADER_EXIST(_pFwHdr)	((_pFwHdr->Signature&0xFFF0) == 0x92C0 ||\
					 (_pFwHdr->Signature&0xFFF0) == 0x88C0)

typedef enum _FIRMWARE_SOURCE{
	FW_SOURCE_IMG_FILE = 0,
	FW_SOURCE_HEADER_FILE = 1,		//from header file
}FIRMWARE_SOURCE, *PFIRMWARE_SOURCE;

typedef struct _RT_FIRMWARE{
	FIRMWARE_SOURCE	eFWSource;
	u8			szFwBuffer[FW_8192C_SIZE];
	u32			ulFwLength;
}RT_FIRMWARE, *PRT_FIRMWARE, RT_FIRMWARE_92C, *PRT_FIRMWARE_92C;

//
// This structure must be cared byte-ordering
//
// Added by tynli. 2009.12.04.
typedef struct _RT_8192C_FIRMWARE_HDR {//8-byte alinment required

	//--- LONG WORD 0 ----
	u16		Signature;	// 92C0: test chip; 92C, 88C0: test chip; 88C1: MP A-cut; 92C1: MP A-cut
	u8		Category;	// AP/NIC and USB/PCI
	u8		Function;	// Reserved for different FW function indcation, for further use when driver needs to download different FW in different conditions
	u16		Version;		// FW Version
	u8		Subversion;	// FW Subversion, default 0x00
	u16		Rsvd1;


	//--- LONG WORD 1 ----
	u8		Month;	// Release time Month field
	u8		Date;	// Release time Date field
	u8		Hour;	// Release time Hour field
	u8		Minute;	// Release time Minute field
	u16		RamCodeSize;	// The size of RAM code
	u16		Rsvd2;

	//--- LONG WORD 2 ----
	u32		SvnIdx;	// The SVN entry index
	u32		Rsvd3;

	//--- LONG WORD 3 ----
	u32		Rsvd4;
	u32		Rsvd5;

}RT_8192C_FIRMWARE_HDR, *PRT_8192C_FIRMWARE_HDR;

#define DRIVER_EARLY_INT_TIME		0x05
#define BCN_DMA_ATIME_INT_TIME		0x02

#define USB_HIGH_SPEED_BULK_SIZE	512
#define USB_FULL_SPEED_BULK_SIZE	64

#if USB_RX_AGGREGATION_92C

typedef enum _USB_RX_AGG_MODE{
	USB_RX_AGG_DISABLE,
	USB_RX_AGG_DMA,
	USB_RX_AGG_USB,
	USB_RX_AGG_DMA_USB
}USB_RX_AGG_MODE;

#define MAX_RX_DMA_BUFFER_SIZE	10240		// 10K for 8192C RX DMA buffer

#endif


#define TX_SELE_HQ			BIT(0)		// High Queue
#define TX_SELE_LQ			BIT(1)		// Low Queue
#define TX_SELE_NQ			BIT(2)		// Normal Queue


// Note: We will divide number of page equally for each queue other than public queue!

#define TX_TOTAL_PAGE_NUMBER		0xF8
#define TX_PAGE_BOUNDARY		(TX_TOTAL_PAGE_NUMBER + 1)

// For Normal Chip Setting
// (HPQ + LPQ + NPQ + PUBQ) shall be TX_TOTAL_PAGE_NUMBER
#define NORMAL_PAGE_NUM_PUBQ		0x56


// For Test Chip Setting
// (HPQ + LPQ + PUBQ) shall be TX_TOTAL_PAGE_NUMBER
#define TEST_PAGE_NUM_PUBQ		0x7E
#define TX_TOTAL_PAGE_NUMBER_92DNORMAL		0x7C
#define TEST_PAGE_NUM_PUBQ_92DNORMAL		0x34

// For Test Chip Setting
#define WMM_TEST_TX_TOTAL_PAGE_NUMBER	0xF5
#define WMM_TEST_TX_PAGE_BOUNDARY	(WMM_TEST_TX_TOTAL_PAGE_NUMBER + 1) //F6

#define WMM_TEST_PAGE_NUM_PUBQ		0xA3
#define WMM_TEST_PAGE_NUM_HPQ		0x29
#define WMM_TEST_PAGE_NUM_LPQ		0x29


//Note: For Normal Chip Setting ,modify later
#define WMM_NORMAL_TX_TOTAL_PAGE_NUMBER	0xF5
#define WMM_NORMAL_TX_PAGE_BOUNDARY	(WMM_TEST_TX_TOTAL_PAGE_NUMBER + 1) //F6

#define WMM_NORMAL_PAGE_NUM_PUBQ		0xB0
#define WMM_NORMAL_PAGE_NUM_HPQ		0x29
#define WMM_NORMAL_PAGE_NUM_LPQ			0x1C
#define WMM_NORMAL_PAGE_NUM_NPQ		0x1C

#define WMM_NORMAL_PAGE_NUM_PUBQ_92D		0X67//0x82
#define WMM_NORMAL_PAGE_NUM_HPQ_92D		0X32//0x29
#define WMM_NORMAL_PAGE_NUM_LPQ_92D		0X32
#define WMM_NORMAL_PAGE_NUM_NPQ_92D		0X32

//-------------------------------------------------------------------------
//	Chip specific
//-------------------------------------------------------------------------
#define NORMAL_CHIP			BIT(4)

#define CHIP_92C_BITMASK		BIT(0)
#define CHIP_92D_SINGLEPHY              BIT(1)
#define CHIP_92C			0x01
#define CHIP_88C			0x00

#define IS_NORMAL_CHIP(version)	((version & NORMAL_CHIP) ? _TRUE : _FALSE)
#define IS_92C_SERIAL(version)	((version & CHIP_92C_BITMASK) ? _TRUE : _FALSE)
#define IS_92D_SINGLEPHY(version)     ((version & CHIP_92D_SINGLEPHY) ? _TRUE : _FALSE)

typedef enum _VERSION_8192C{
	VERSION_TEST_CHIP_92C = 0x01,
	VERSION_TEST_CHIP_88C = 0x00,
	VERSION_NORMAL_CHIP_92C = 0x11,
	VERSION_NORMAL_CHIP_88C = 0x10,
	VERSION_TEST_CHIP_92D_SINGLEPHY= 0x82,
	VERSION_TEST_CHIP_92D_DUALPHY = 0x80,
	VERSION_NORMAL_CHIP_92D_SINGLEPHY= 0x92,
	VERSION_NORMAL_CHIP_92D_DUALPHY = 0x90,
}VERSION_8192C,*PVERSION_8192C;


//-------------------------------------------------------------------------
//	Channel Plan
//-------------------------------------------------------------------------
enum ChannelPlan{
	CHPL_FCC	= 0,
	CHPL_IC		= 1,
	CHPL_ETSI	= 2,
	CHPL_SPAIN	= 3,
	CHPL_FRANCE	= 4,
	CHPL_MKK	= 5,
	CHPL_MKK1	= 6,
	CHPL_ISRAEL	= 7,
	CHPL_TELEC	= 8,
	CHPL_GLOBAL	= 9,
	CHPL_WORLD	= 10,
};

typedef struct _TxPowerInfo{
	u8 CCKIndex[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT40_1SIndex[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT40_2SIndexDiff[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT20IndexDiff[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 OFDMIndexDiff[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT40MaxOffset[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT20MaxOffset[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 TSSI_A[2];
	u8 TSSI_B[2];
}TxPowerInfo, *PTxPowerInfo;

struct hal_priv
{
	unsigned char (*hal_bus_init)(_adapter *padapter);
	unsigned char  (*hal_bus_deinit)(_adapter *padapter);

	unsigned short HardwareType;
	unsigned short VersionID;
	unsigned short CustomerID;

	// add for 92D Phy mode/mac/Band mode 
	MACPHY_MODE_8192D		MacPhyMode92D;
	//u8						BandType92D;	//0: 2.4G,        1:  5G
	BAND_TYPE				CurrentBandType92D;	//0:2.4G, 1:5G
	BAND_TYPE				CurrentBandTypeBackup92D;
	BAND_TYPE				BandSet92D;

	unsigned short FirmwareVersion;
	unsigned short FirmwareVersionRev;
	unsigned short FirmwareSubVersion;

	//current WIFI_PHY values
	unsigned long	ReceiveConfig;
	unsigned char	CurrentChannel;
	int			CurrentWirelessMode;
	HT_CHANNEL_WIDTH	CurrentChannelBW;
	unsigned char	nCur40MhzPrimeSC;// Control channel sub-carrier

       BOOLEAN		bPhyValueInitReady;

	//rf_ctrl
	unsigned char rf_chip;
	unsigned char rf_type;
	unsigned char NumTotalRFPath;

	BB_REGISTER_DEFINITION_T	PHYRegDef[4];	//Radio A/B/C/D

	// Channel Access related register.
	u8					SlotTime;
	u32					SifsTime;

	// Read/write are allow for following hardware information variables
	u8					framesync;
	unsigned int			framesyncC34;
	u8					framesyncMonitor;
	u8					DefaultInitialGain[4];
	u8					pwrGroupCnt;
	u32					MCSTxPowerLevelOriginalOffset[4][7];
	u32					CCKTxPowerLevelOriginalOffset;
	u8					TxPowerLevelCCK[14];			// CCK channel 1~14
	u8					TxPowerLevelOFDM24G[14];		// OFDM 2.4G channel 1~14
	u8					TxPowerLevelOFDM5G[14];			// OFDM 5G
	u8					AntennaTxPwDiff[3];	// Antenna gain offset, index 0 for B, 1 for C, and 2 for D

	//u8					ThermalMeter[2];	// ThermalMeter, index 0 for RFIC0, and 1 for RFIC1
	u32					AntennaTxPath;					// Antenna path Tx
	u32					AntennaRxPath;					// Antenna path Rx
	u8					BluetoothCoexist;
	u8					ExternalPA;

	u32					LedControlNum;
	u32					LedControlMode;
	//u32					TxPowerTrackControl;
	u8					b1x1RecvCombine;	// for 1T1R receive combining

	u8					bCCKinCH14;

	BOOLEAN				bTXPowerDataReadFromEEPORM;
	//vivi, for tx power tracking, 20080407
	//u2Byte					TSSI_13dBm;
	//u4Byte					Pwr_Track;
	// The current Tx Power Level
	unsigned char	CurrentCckTxPwrIdx;
	unsigned char	CurrentOfdm24GTxPwrIdx;

	unsigned char	LegacyHTTxPowerDiff;// Legacy to HT rate power diff

	unsigned char bCckHighPower;

	unsigned int	RfRegChnlVal[2];

	u8					EEPROMRegulatory;

	//
	// The same as 92CE.
	//
	u8					ThermalMeter[2];	// ThermalMeter, index 0 for RFIC0, and 1 for RFIC1
	u8					ThermalValue;
	u8					ThermalValue_LCK;
	u8					ThermalValue_IQK;
	u8					bRfPiEnable;

	//for APK
	u32					APKoutput[2][2];	//path A/B; output1_1a/output1_2a
	u8					bAPKdone;
	u8					bAPKThermalMeterIgnore;

	//for TxPwrTracking
	int					RegE94;
	int 					RegE9C;
	int					RegEB4;
	int					RegEBC;

	//for IQK
	unsigned int		RegC04;
	unsigned int		Reg874;
	unsigned int		RegC08;
	unsigned int		Reg88C;
	unsigned int		ADDA_backup[16];

	//RDG enable
	BOOLEAN		bRDGEnable;

       BOOLEAN		bInSetPower;

	u8	RegTxPause;

	//for host message to fw
	u8	LastHMEBoxNum;

	u8	RegBcnCtrlVal;

#ifdef CONFIG_USB_HCI

	// For 92C USB endpoint setting
	//

	unsigned int UsbBulkOutSize;

	int	RtNumInPipes;	// Number of Rx pipes in the ring, RtInPipe.
	int	RtNumOutPipes;	// Number of Tx pipes in the ring, RtOutPipe
	int	RtBulkOutPipes[4];
	int	RtBulkInPipes;
	int	RtIntInPipes;
	// Add for 92D  dual MAC 0--Mac0 1--Mac1
	u32	interfaceIndex;

	unsigned char	OutEpQueueSel;
	unsigned char OutEpNumber;

	int Queue2EPNum[8];//for out endpoint number mapping

#if USB_TX_AGGREGATION_92C
	unsigned char			UsbTxAggMode;
	unsigned char			UsbTxAggDescNum;
#endif
#if USB_RX_AGGREGATION_92C
	u16				HwRxPageSize;				// Hardware setting
	u32				MaxUsbRxAggBlock;

	USB_RX_AGG_MODE	UsbRxAggMode;
	u8				UsbRxAggBlockCount;			// USB Block count. Block size is 512-byte in hight speed and 64-byte in full speed
	u8				UsbRxAggBlockTimeout;
	u8				UsbRxAggPageCount;			// 8192C DMA page count
	u8				UsbRxAggPageTimeout;
#endif

#endif

	u8 fw_ractrl;

	// 20100428 Guangan: for 92D MAC1 init radio A
	BOOLEAN                    bDuringMac1InitRadioA;
};

void NicIFAssociateNIC(PADAPTER Adapter);
void NicIFReadAdapterInfo(PADAPTER Adapter);
void _ReadChipVersion(IN PADAPTER Adapter);


typedef struct hal_priv HAL_DATA_TYPE, *PHAL_DATA_TYPE;
typedef struct eeprom_priv EEPROM_EFUSE_PRIV, *PEEPROM_EFUSE_PRIV;

#define GET_HAL_DATA(priv)	(&priv->halpriv)
#define GET_RF_TYPE(priv)	(GET_HAL_DATA(priv)->rf_type)

#define GET_EEPROM_EFUSE_PRIV(priv)	(&priv->eeprompriv)

#define IS_HARDWARE_TYPE_8192CE(_Adapter)	(((PHAL_DATA_TYPE)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8192CE)
#define IS_HARDWARE_TYPE_8192CU(_Adapter)	(((PHAL_DATA_TYPE)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8192CU)
#define IS_HARDWARE_TYPE_8192DE(_Adapter)	(((PHAL_DATA_TYPE)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8192DE)
#define IS_HARDWARE_TYPE_8192DU(_Adapter)	(((PHAL_DATA_TYPE)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8192DU)

#define	IS_HARDWARE_TYPE_8192C(_Adapter)			\
(IS_HARDWARE_TYPE_8192CE(_Adapter) || IS_HARDWARE_TYPE_8192CU(_Adapter))

#define	IS_HARDWARE_TYPE_8192D(_Adapter)			\
(IS_HARDWARE_TYPE_8192DE(_Adapter) || IS_HARDWARE_TYPE_8192DU(_Adapter))

#endif

