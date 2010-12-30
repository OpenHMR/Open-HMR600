

#include "rtl2832u_io.h"
#include <linux/time.h>

#define ERROR_TRY_MAX_NUM		4



void
platform_wait(
	unsigned long nMinDelayTime)
{
	// The unit of Sleep() waiting time is millisecond.
	unsigned long usec;
	do {
		usec = (nMinDelayTime > 8000) ? 8000 : nMinDelayTime;
		msleep(usec);
		nMinDelayTime -= usec;
		} while (nMinDelayTime > 0);

	return;
	
}





static int 
read_usb_register(
	struct dvb_usb_device*	dib,
	unsigned short	offset,
	unsigned char*	data,
	unsigned short	bytelength)
{
	int ret = -ENOMEM;
 
        ret = usb_control_msg(dib->udev,								/* pointer to device */
                usb_rcvctrlpipe( dib->udev,RTL2832_CTRL_ENDPOINT),			/* pipe to control endpoint */
                0,														/* USB message request value */
                SKEL_VENDOR_IN,										/* USB message request type value */
                (USB_BASE_ADDRESS<<8) + offset,						/* USB message value */
                0x0100,													/* USB message index value */
                data,													/* pointer to the receive buffer */
                bytelength,												/* length of the buffer */
                DIBUSB_I2C_TIMEOUT);									/* time to wait for the message to complete before timing out */
 	
        if (ret != bytelength)
	{
		deb_info(" %s: offset=0x%x, error code=0x%x !\n", __FUNCTION__, offset, ret);
		return 1;
       }

	return 0; 
}



static int 
write_usb_register(
	struct dvb_usb_device*	dib,
	unsigned short	offset,
	unsigned char*	data,
	unsigned short	bytelength)
{
	int ret = -ENOMEM;
	unsigned char try_num;

	try_num = 0;	
error_write_again:
	try_num++;	
 
        ret = usb_control_msg(dib->udev,								/* pointer to device */
                usb_sndctrlpipe( dib->udev,RTL2832_CTRL_ENDPOINT),		/* pipe to control endpoint */
                0,														/* USB message request value */
                SKEL_VENDOR_OUT,										/* USB message request type value */
                (USB_BASE_ADDRESS<<8) + offset,						/* USB message value */
                0x0110,													/* USB message index value */
                data,													/* pointer to the receive buffer */
                bytelength,												/* length of the buffer */
                DIBUSB_I2C_TIMEOUT);									/* time to wait for the message to complete before timing out */

        if (ret != bytelength)
	{
		deb_info("error try = %d, %s: offset=0x%x, error code=0x%x !\n",try_num ,__FUNCTION__, offset, ret);

		if( try_num > ERROR_TRY_MAX_NUM )	goto error;
		else				goto error_write_again;
       }

	return 0;
error:
	return 1;
 }




static int 
read_sys_register(
	struct dvb_usb_device*	dib,
	unsigned short	offset,
	unsigned char*	data,
	unsigned short	bytelength)
{
	int ret = -ENOMEM;
 
        ret = usb_control_msg(dib->udev,								/* pointer to device */
                usb_rcvctrlpipe( dib->udev,RTL2832_CTRL_ENDPOINT),		/* pipe to control endpoint */
                0,														/* USB message request value */
                SKEL_VENDOR_IN,										/* USB message request type value */
                (SYS_BASE_ADDRESS<<8) + offset,						/* USB message value */
                0x0200,													/* USB message index value */
                data,													/* pointer to the receive buffer */
                bytelength,												/* length of the buffer */
                DIBUSB_I2C_TIMEOUT);									/* time to wait for the message to complete before timing out */
			
        if (ret != bytelength)
	{
		deb_info(" %s: offset=0x%x, error code=0x%x !\n", __FUNCTION__, offset, ret);
		return 1;
       }

	return 0; 

  }


static int 
write_sys_register(
	struct dvb_usb_device*	dib,
	unsigned short	offset,
	unsigned char*	data,
	unsigned short	bytelength)
{ 
	int ret = -ENOMEM;
	unsigned char try_num;

	try_num = 0;	
error_write_again:	
	try_num++;	

        ret = usb_control_msg(dib->udev,								/* pointer to device */
                usb_sndctrlpipe( dib->udev,RTL2832_CTRL_ENDPOINT),		/* pipe to control endpoint */
                0,														/* USB message request value */
                SKEL_VENDOR_OUT,										/* USB message request type value */
                (SYS_BASE_ADDRESS<<8) + offset,						/* USB message value */
                0x0210,													/* USB message index value */
                data,													/* pointer to the receive buffer */
                bytelength,												/* length of the buffer */
                DIBUSB_I2C_TIMEOUT);									/* time to wait for the message to complete before timing out */
		
        if (ret != bytelength)
	{
		deb_info(" error try= %d, %s: offset=0x%x, error code=0x%x !\n",try_num, __FUNCTION__, offset, ret);
		if( try_num > ERROR_TRY_MAX_NUM )	goto error;
		else				goto error_write_again;	
        }

	return 0;
error:
	return 1;
 }




int 
read_rtl2832_demod_register(
	struct dvb_usb_device*dib,
	unsigned char			demod_device_addr,	
	unsigned char 		page,
	unsigned char 		offset,
	unsigned char*		data,
	unsigned short		bytelength)
{
	int ret = -ENOMEM;
	int i;

	if (down_interruptible(&dib->usb_sem))	
	    goto error;

        ret = usb_control_msg(dib->udev,								/* pointer to device */
                usb_rcvctrlpipe( dib->udev,RTL2832_CTRL_ENDPOINT),			/* pipe to control endpoint */
                0,														/* USB message request value */
                SKEL_VENDOR_IN,										/* USB message request type value */
                demod_device_addr + (offset<<8),						/* USB message value */
                (0x0000 + page),										/* USB message index value */
                data,													/* pointer to the receive buffer */
                bytelength,												/* length of the buffer */
                DIBUSB_I2C_TIMEOUT);									/* time to wait for the message to complete before timing out */

	up(&dib->usb_sem);

	
//		deb_info("%s: ret=%d, DA=0x%x, len=%d, page=%d, offset=0x%x, data=(", __FUNCTION__, ret, demod_device_addr, bytelength,page, offset);
//		for(i = 0; i < bytelength; i++)
//			deb_info("0x%x,", data[i]);
//		deb_info(")\n");
			
        if (ret != bytelength)
	{
		deb_info("error!! %s: ret=%d, DA=0x%x, len=%d, page=%d, offset=0x%x, data=(", __FUNCTION__, ret, demod_device_addr, bytelength,page, offset);
		for(i = 0; i < bytelength; i++)
			deb_info("0x%x,", data[i]);
		deb_info(")\n");
		
		goto error;
       }

	return 0;  

error:
	return 1;
}




int
write_rtl2832_demod_register(
	struct dvb_usb_device*dib,
	unsigned char			demod_device_addr,		
	unsigned char			page,
	unsigned char			offset,
	unsigned char			*data,
	unsigned short		bytelength)
{
	int ret = -ENOMEM;
	unsigned char  i, try_num;

	try_num = 0;	
error_write_again:	
	try_num++;

	if( down_interruptible(&dib->usb_sem) )	goto error;
	
        ret = usb_control_msg(dib->udev,								/* pointer to device */
                usb_sndctrlpipe( dib->udev,RTL2832_CTRL_ENDPOINT),		/* pipe to control endpoint */
                0,														/* USB message request value */
                SKEL_VENDOR_OUT,										/* USB message request type value */
                demod_device_addr + (offset<<8),						/* USB message value */
                (0x0010 + page),										/* USB message index value */
                data,													/* pointer to the receive buffer */
                bytelength,												/* length of the buffer */
                DIBUSB_I2C_TIMEOUT);									/* time to wait for the message to complete before timing out */

	up(&dib->usb_sem);

//		deb_info("%s: ret=%d, DA=0x%x, len=%d, page=%d, offset=0x%x, data=(", __FUNCTION__, ret, demod_device_addr, bytelength, page,offset);
//		for(i = 0; i < bytelength; i++)
//			deb_info("0x%x,", data[i]);
//		deb_info(")\n");


        if (ret != bytelength)
	{
		deb_info("error try = %d!! %s: ret=%d, DA=0x%x, len=%d, page=%d, offset=0x%x, data=(",try_num , __FUNCTION__, ret, demod_device_addr, bytelength,page,offset);
		for(i = 0; i < bytelength; i++)
			deb_info("0x%x,", data[i]);
		deb_info(")\n");
	
		if( try_num > ERROR_TRY_MAX_NUM )	goto error;
		else				goto error_write_again;
        }

	return 0;

error:
	return 1;
 }






int 
read_rtl2832_tuner_register(
	struct dvb_usb_device	*dib,
	unsigned char			device_address,
	unsigned char			offset,
	unsigned char			*data,
	unsigned short		bytelength)
{
	int ret = -ENOMEM;
 	int i;
	unsigned char data_tmp[128];	


	if( down_interruptible(&dib->usb_sem) )	goto error;
	
        ret = usb_control_msg(dib->udev,								/* pointer to device */
                usb_rcvctrlpipe( dib->udev,RTL2832_CTRL_ENDPOINT),			/* pipe to control endpoint */
                0,														/* USB message request value */
                SKEL_VENDOR_IN,										/* USB message request type value */
                device_address+(offset<<8),							/* USB message value */
                0x0300,													/* USB message index value */
                data_tmp,												/* pointer to the receive buffer */
                bytelength,												/* length of the buffer */
                DIBUSB_I2C_TIMEOUT);									/* time to wait for the message to complete before timing out */

	up(&dib->usb_sem);

//		deb_info("%s: ret=%d, DA=0x%x, len=%d, offset=0x%x, data=(", __FUNCTION__, ret, device_address, bytelength,offset);
//		for(i = 0; i < bytelength; i++)
//			deb_info("0x%x,", data_tmp[i]);
//		deb_info(")\n");
			
        if (ret != bytelength)
 	{
		deb_info("error!! %s: ret=%d, DA=0x%x, len=%d, offset=0x%x, data=(", __FUNCTION__, ret, device_address, bytelength,offset);
		for(i = 0; i < bytelength; i++)
			deb_info("0x%x,", data_tmp[i]);
		deb_info(")\n");
		
		goto error;
        }

	memcpy(data,data_tmp,bytelength);

	return 0;
	
error:
	return 1;   
	
 
}

int write_rtl2832_tuner_register(
	struct dvb_usb_device *dib,
	unsigned char			device_address,
	unsigned char			offset,
	unsigned char			*data,
	unsigned short		bytelength)
{
	int ret = -ENOMEM;
	unsigned char  i, try_num;

	try_num = 0;	
error_write_again:	
	try_num++;

	if( down_interruptible(&dib->usb_sem) )	goto error;
 
        ret = usb_control_msg(dib->udev,								/* pointer to device */
                usb_sndctrlpipe( dib->udev,RTL2832_CTRL_ENDPOINT),		/* pipe to control endpoint */
                0,														/* USB message request value */
                SKEL_VENDOR_OUT,										/* USB message request type value */
                device_address+(offset<<8),							/* USB message value */
                0x0310,													/* USB message index value */
                data,													/* pointer to the receive buffer */
                bytelength,												/* length of the buffer */
                DIBUSB_I2C_TIMEOUT);									/* time to wait for the message to complete before timing out */

	up(&dib->usb_sem);

//		deb_info("%s: ret=%d, DA=0x%x, len=%d, offset=0x%x, data=(", __FUNCTION__, ret, device_address, bytelength, offset);
//		for(i = 0; i < bytelength; i++)
//			deb_info("0x%x,", data[i]);
//		deb_info(")\n");

			
        if (ret != bytelength)
	{
		deb_info("error try= %d!! %s: ret=%d, DA=0x%x, len=%d, offset=0x%x, data=(",try_num, __FUNCTION__, ret, device_address, bytelength, offset);
		for(i = 0; i < bytelength; i++)
			deb_info("0x%x,", data[i]);
		deb_info(")\n");
		
		if( try_num > ERROR_TRY_MAX_NUM )	goto error;
		else				goto error_write_again;
       }

	return 0;

error:
	return 1;
 }




int
read_rtl2832_stdi2c(
	struct dvb_usb_device*	dib,
	unsigned short			dev_i2c_addr,
	unsigned char*			data,
	unsigned short			bytelength)
{
	int i;
	int ret = -ENOMEM;
	unsigned char  try_num;
	unsigned char data_tmp[128];	

	try_num = 0;	
error_write_again:		
	try_num ++;	
        

	if(bytelength >= 128)
	{
		deb_info("%s error bytelength >=128  \n", __FUNCTION__);
		goto error;
	}

	if( down_interruptible(&dib->usb_sem) )	goto error;
	
	ret = usb_control_msg(dib->udev,								/* pointer to device */
		usb_rcvctrlpipe( dib->udev,RTL2832_CTRL_ENDPOINT),			/* pipe to control endpoint */
		0,														/* USB message request value */
		SKEL_VENDOR_IN,											/* USB message request type value */
		dev_i2c_addr,											/* USB message value */
		0x0600,													/* USB message index value */
		data_tmp,												/* pointer to the receive buffer */
		bytelength,												/* length of the buffer */
		DIBUSB_I2C_TIMEOUT);									/* time to wait for the message to complete before timing out */

	up(&dib->usb_sem);
 
	if (ret != bytelength)
	{
		deb_info("error try= %d!! %s: ret=%d, DA=0x%x, len=%d, data=(",try_num, __FUNCTION__, ret, dev_i2c_addr, bytelength);
		for(i = 0; i < bytelength; i++)
			deb_info("0x%x,", data_tmp[i]);
		deb_info(")\n");
		
		if( try_num > ERROR_TRY_MAX_NUM )	goto error;
		else				goto error_write_again;
	}

 	memcpy(data,data_tmp,bytelength);

	return 0;  
error: 
	return 1;

}



int 
write_rtl2832_stdi2c(
	struct dvb_usb_device*	dib,
	unsigned short			dev_i2c_addr,
	unsigned char*			data,
	unsigned short			bytelength)
{
	int i;
	int ret = -ENOMEM;  
	unsigned char  try_num;

	try_num = 0;	
error_write_again:		
	try_num ++;	

	if( down_interruptible(&dib->usb_sem) )	goto error;

	ret = usb_control_msg(dib->udev,								/* pointer to device */
		usb_sndctrlpipe( dib->udev,RTL2832_CTRL_ENDPOINT),			/* pipe to control endpoint */
		0,														/* USB message request value */
		SKEL_VENDOR_OUT,										/* USB message request type value */
		dev_i2c_addr,											/* USB message value */
		0x0610,													/* USB message index value */
		data,													/* pointer to the receive buffer */
		bytelength,												/* length of the buffer */
		DIBUSB_I2C_TIMEOUT);									/* time to wait for the message to complete before timing out */

	up(&dib->usb_sem);
 
	if (ret != bytelength)
	{
		deb_info("error try= %d!! %s: ret=%d, DA=0x%x, len=%d, data=(",try_num, __FUNCTION__, ret, dev_i2c_addr, bytelength);
		for(i = 0; i < bytelength; i++)
			deb_info("0x%x,", data[i]);
		deb_info(")\n");
		
		if( try_num > ERROR_TRY_MAX_NUM )	goto error;
		else				goto error_write_again;		

	}
 
	return 0;
	
error:
	return 1;  
	
}






//3for return PUCHAR value

int
read_usb_sys_char_bytes(
	struct dvb_usb_device*	dib,
	RegType	type,
	unsigned short	byte_addr,
	unsigned char*	buf,
	unsigned short	byte_num)
{
	int ret = 1;

	if( byte_num != 1 && byte_num !=2 && byte_num !=4)
	{
		deb_info("error!! %s: length = %d \n", __FUNCTION__, byte_num);
		return 1;
	}


	if( down_interruptible(&dib->usb_sem) )	return 1;
		
	if( type == RTD2832U_USB )
	{
		ret = read_usb_register( dib , byte_addr , buf , byte_num);
	}
	else if ( type == RTD2832U_SYS )
	{
		ret = read_sys_register( dib , byte_addr , buf , byte_num);
	}
	
	up(&dib->usb_sem);

	return ret;
	
}



int
write_usb_sys_char_bytes(
	struct dvb_usb_device*	dib,
	RegType	type,
	unsigned short	byte_addr,
	unsigned char*	buf,
	unsigned short	byte_num)
{
	int ret = 1;

	if( byte_num != 1 && byte_num !=2 && byte_num !=4)
	{
		deb_info("error!! %s: length = %d \n", __FUNCTION__, byte_num);
		return 1;
	}

	if( down_interruptible(&dib->usb_sem) )	return 1;	
	
	if( type == RTD2832U_USB )
	{
		ret = write_usb_register( dib , byte_addr , buf , byte_num);
	}
	else if ( type == RTD2832U_SYS )
	{
		ret = write_sys_register( dib , byte_addr , buf , byte_num);
	}
	
	up(&dib->usb_sem);	
	
	return ret;
	
}


//3for return INT value
int
read_usb_sys_int_bytes(
	struct dvb_usb_device*	dib,
	RegType	type,
	unsigned short	byte_addr,
	unsigned short	n_bytes,
	int*	p_val)
{
	int	i;
	u8	val[LEN_4_BYTE];
	int	nbit_shift; 

	*p_val= 0;

	if (read_usb_sys_char_bytes( dib, type, byte_addr, val , n_bytes)) goto error;

	for (i= 0; i< n_bytes; i++)
	{				
		nbit_shift = i<<3 ;
		*p_val = *p_val + (val[i]<<nbit_shift );
       }

	return 0;
error:
	return 1;
			
}



int
write_usb_sys_int_bytes(
	struct dvb_usb_device*	dib,
	RegType	type,
	unsigned short	byte_addr,
	unsigned short	n_bytes,
	int	val)
{
	int	i;
	int	nbit_shift;
	u8	u8_val[LEN_4_BYTE];		

	for (i= n_bytes- 1; i>= 0; i--)
	{
		nbit_shift= i << 3;		
		u8_val[i] = (val>> nbit_shift) & 0xff;
    	}

	if( write_usb_sys_char_bytes( dib , type , byte_addr, u8_val , n_bytes) ) goto error;			

	return 0;
error:
	return 1;
}














