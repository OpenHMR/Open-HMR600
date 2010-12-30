
EXTRA_CFLAGS += -I drivers/media/dvb/dvb-usb/
EXTRA_CFLAGS += -I drivers/media/dvb/dvb-core/
EXTRA_CFLAGS += -I drivers/media/dvb/frontends/
EXTRA_CFLAGS += -I drivers/media/dvb/dvb-usb/realtek/
EXTRA_CFLAGS += -I drivers/media/dvb/dvb-usb/realtek/api

dvb-usb-rtl2832u-objs = rtl2832u.o \
                        rtl2832u_fe.o \
                        rtl2832u_io.o

obj-$(CONFIG_DVB_USB_RTL2832U) += api/ dvb-usb-rtl2832u.o

