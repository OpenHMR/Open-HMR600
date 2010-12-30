NIC_SELECT = RTL8192SE

EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -O2

ifeq ($(NIC_SELECT), RTL8192SE)
	EXTRA_CFLAGS += -DRTL8192S_DISABLE_FW_DM=0
	EXTRA_CFLAGS += -DRTL8192SE
#	EXTRA_CFLAGS += -DENABLE_IPS
#	EXTRA_CFLAGS += -DENABLE_LPS
endif

ifeq ($(NIC_SELECT), RTL8192SU)
	EXTRA_CFLAGS += -DRTL8192S_DISABLE_FW_DM=0
	EXTRA_CFLAGS += -DRTL8192SU
	EXTRA_CFLAGS += -DENABLE_IPS
	EXTRA_CFLAGS += -DENABLE_LPS
endif

ifeq ($(NIC_SELECT), RTL8192E)
	EXTRA_CFLAGS += -DENABLE_IPS
endif

ifeq ($(NIC_SELECT),RTL8192U)
	#EXTRA_CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE
	#EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
endif

# For RTK DMP platform
EXTRA_CFLAGS += -DRTK_DMP_PLATFORM

#EXTRA_CFLAGS += -DJOHN_NOCPY
EXTRA_CFLAGS  += -DTHOMAS_TURBO
#flags to enable or disble 80211D feature
EXTRA_CFLAGS  += -DENABLE_DOT11D

ieee80211-ral-objs := ieee80211_rx.o \
		      ieee80211_softmac.o \
		      ieee80211_tx.o \
		      ieee80211_wx.o \
		      ieee80211_module.o \
		      ieee80211_softmac_wx.o\
		      rtl819x_HTProc.o\
		      rtl819x_TSProc.o\
		      rtl819x_BAProc.o\
		      dot11d.o

ieee80211_crypt-ral-objs      := ieee80211_crypt.o
ieee80211_crypt_tkip-ral-objs := ieee80211_crypt_tkip.o
ieee80211_crypt_ccmp-ral-objs := ieee80211_crypt_ccmp.o
ieee80211_crypt_wep-ral-objs  := ieee80211_crypt_wep.o

obj-$(CONFIG_REALTEK_PCI_RTL8192SE) += ieee80211-ral.o
obj-$(CONFIG_REALTEK_PCI_RTL8192SE) += ieee80211_crypt-ral.o
obj-$(CONFIG_REALTEK_PCI_RTL8192SE) += ieee80211_crypt_wep-ral.o
obj-$(CONFIG_REALTEK_PCI_RTL8192SE) += ieee80211_crypt_tkip-ral.o
obj-$(CONFIG_REALTEK_PCI_RTL8192SE) += ieee80211_crypt_ccmp-ral.o
