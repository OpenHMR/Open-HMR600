/*
 * Automatically generated C config: don't edit
 */
#define AUTOCONF_INCLUDED
#define RTL8711_MODULE_NAME "xyz"
#define CONFIG_DEBUG_RTL8711 1
#undef  CONFIG_USB_HCI 
#undef  CONFIG_CFIO_HCI 
#undef  CONFIG_LM_HCI
#define CONFIG_SDIO_HCI	1
#define CONFIG_RTL8711	1
#undef  CONFIG_RTL8712
#undef  CONFIG_RTL8716
#define USE_SYNC_IRP	1	
#undef  USE_ASYNC_IRP 
#define CONFIG_LITTLE_ENDIAN 1
#undef CONFIG_BIG_ENDIAN
#undef CONFIG_PWRCTRL
#define RX_BUF_LEN (2400)
#define TX_BUFSZ (2048)
#undef  CONFIG_TXSKBQSZ_16
#define CONFIG_TXSKBQSZ_32 1
#undef  CONFIG_TXSKBQSZ_64
#define MAXCMDSZ (64)
#define CONFIG_HWPKT_END 1
#undef PLATFORM_WINDOWS
#undef PLATFORM_OS_XP
#undef PLATFORM_OS_CE
#define PLATFORM_LINUX 1
#define PLATFORM_OS_LINUX 1
#undef CONFIG_MP_INCLUDED
#define CONFIG_POLLING_MODE 1

#define dlfw_blk_wo_chk
#define burst_read_eeprom
#define CONFIG_EMBEDDED_FWIMG 1
#define RD_BLK_MODE 1
