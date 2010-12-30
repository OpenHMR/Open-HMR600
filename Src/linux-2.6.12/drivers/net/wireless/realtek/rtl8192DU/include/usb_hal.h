#ifndef __USB_HAL_H__
#define __USB_HAL_H__


extern u8 usb_hal_bus_init(_adapter * adapter);
extern u8 usb_hal_bus_deinit(_adapter * adapter);

BOOLEAN HalUsbSetQueuePipeMapping8192CUsb(
	IN	PADAPTER	pAdapter,
	IN	u8		NumInPipe,
	IN	u8		NumOutPipe
	);

#endif //__USB_HAL_H__

