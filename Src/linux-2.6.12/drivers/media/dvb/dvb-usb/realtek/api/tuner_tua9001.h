#ifndef __TUNER_TUA9001_H
#define __TUNER_TUA9001_H

/**

@file

@brief   TUA9001 tuner module declaration

One can manipulate TUA9001 tuner through TUA9001 module.
TUA9001 module is derived from tunerd module.



@par Example:
@code

// The example is the same as the tuner example in tuner_base.h except the listed lines.



#include "tuner_tua9001.h"


...



int main(void)
{
	TUNER_MODULE        *pTuner;
	TUA9001_EXTRA_MODULE *pTunerExtra;

	TUNER_MODULE          TunerModuleMemory;
	TUA9001_EXTRA_MODULE   Tua9001ExtraModuleMemory;
	BASE_INTERFACE_MODULE BaseInterfaceModuleMemory;
	I2C_BRIDGE_MODULE     I2cBridgeModuleMemory;

	int BandwidthMode;


	...



	// Build TUA9001 tuner module.
	BuildTua9001Module(
		&pTuner,
		&TunerModuleMemory,
		&Tua9001ExtraModuleMemory,
		&BaseInterfaceModuleMemory,
		&I2cBridgeModuleMemory,
		0xc0								// I2C device address is 0xc0 in 8-bit format.
		);





	// Get TUA9001 tuner extra module.
	pTunerExtra = (T2266_EXTRA_MODULE *)(pTuner->pExtra);





	// ==== Initialize tuner and set its parameters =====

	...

	// Set TUA9001 bandwidth.
	pTunerExtra->SetBandwidthMode(pTuner, TUA9001_BANDWIDTH_6MHZ);





	// ==== Get tuner information =====

	...

	// Get TUA9001 bandwidth.
	pTunerExtra->GetBandwidthMode(pTuner, &BandwidthMode);





	// See the example for other tuner functions in tuner_base.h


	return 0;
}


@endcode

*/


#include "tuner_base.h"
//#include "i2c_rtl2832usb.h"





// The following context is source code provided by Infineon.





// Infineon source code - driver_tua9001.h



/* ============================================================================
** Copyright (C) 1997-2007 Infineon AG All rights reserved.
** ============================================================================
**
** ============================================================================
** Revision Information :
**    File name: driver_tua9001.h
**    Version: 
**    Date: 
**
** ============================================================================
** History: 
** 
** Date         Author  Comment
** ----------------------------------------------------------------------------
**
** 2007.11.06   Walter Pichler    created.
** ============================================================================
*/

 
/*============================================================================
 Named Constants Definitions
============================================================================*/

#define TUNER_OK                       0
#define TUNER_ERR                      0xff

#define H_LEVEL                        1
#define L_LEVEL                        0


/*============================================================================
 Types definition
============================================================================*/


typedef enum {
        TUNER_BANDWIDTH_8MHZ,
        TUNER_BANDWIDTH_7MHZ,
        TUNER_BANDWIDTH_6MHZ,
        TUNER_BANDWIDTH_5MHZ,
        } tunerDriverBW_t;


typedef enum {
        PLL_LOCKED,
        PLL_NOT_LOCKED
        }tunerPllLocked_t;


typedef enum {
        STANDBY,
        IDLE,
        RX
        } tunerReceiverState_t;


typedef enum {
        L_INPUT_ACTIVE,
        UHF_INPUT_ACTIVE,
        VHF_INPUT_ACTIVE
        } tunerActiveInput_t;

 

/*============================================================================
 Public functions
============================================================================*/

/**
 * tuner initialisation
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/
extern int initializeTua9001 (TUNER_MODULE *pTuner);


/**
 * tuner tune
 * @param i_freq   tuning frequency
 * @param i_bandwidth  channel  bandwidth
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/
extern int tuneTua9001 (TUNER_MODULE *pTuner, long i_freq, tunerDriverBW_t i_bandwidth);


/**
 * Get tuner state
 * @param o_tunerState tuner state
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/
extern int getReceiverStateTua9001 (TUNER_MODULE *pTuner, tunerReceiverState_t *o_tunerState);

/**
 * Get active input
 * @param o_activeInput active input info
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/
extern int getActiveInputTua9001 (TUNER_MODULE *pTuner, tunerActiveInput_t *o_activeInput);


/**
 * Get baseband gain value
 * @param o_basebandGain  baseband gain value
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/
extern int getBasebandGainTua9001 (TUNER_MODULE *pTuner, char *o_basebandGain);























// Infineon source code - driver_tua9001_NeededFunctions.h



/*========================================================================================================================
 additional needed external funtions ( have to  be provided by the user! )
========================================================================================================================*/

/**
 * set / reset tuner reset input
 * @param i_state   level
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/

int setRESETN (TUNER_MODULE *pTuner, unsigned int i_state);


/**
 * set / reset tuner receive enable input
 * @param i_state   level
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/

int setRXEN (TUNER_MODULE *pTuner, unsigned int i_state);


/**
 * set / reset tuner chiop enable input
 * @param i_state   level
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/

int setCEN (TUNER_MODULE *pTuner, unsigned int i_state);


/**
 * waitloop 
 * @param i_looptime   * 1uS
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/

int waitloop (TUNER_MODULE *pTuner, unsigned int i_looptime);


/**
 * i2cBusWrite 
 * @param deviceAdress    chip address 
 * @param registerAdress  register address 
 * @param *data           pointer to data source
 * @param length          number of bytes to transmit
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/

 int i2cBusWrite (TUNER_MODULE *pTuner, unsigned char deviceAddress, unsigned char registerAddress, char *data,
	 unsigned int length);


/**
 * i2cBusRead 
 * @param deviceAdress    chip address 
 * @param registerAdress  register address 
 * @param *data           pointer to data destination
 * @param length          number of bytes to read
 * @retval TUNER_OK No error
 * @retval TUNER_ERR Error
*/

 int i2cBusRead (TUNER_MODULE *pTuner, unsigned char deviceAddress, unsigned char registerAddress, char *data,
	 unsigned int length);

/*========================================================================================================================
 end of additional needed external funtions
========================================================================================================================*/























// The following context is TUA9001 tuner API source code





/**

@file

@brief   TUA9001 tuner module declaration

One can manipulate TUA9001 tuner through TUA9001 module.
TUA9001 module is derived from tuner module.

*/





// Definitions

// Constant
#define GPO_ADDR	0x1

// Bandwidth modes
enum TUA9001_BANDWIDTH_MODE
{
	TUA9001_BANDWIDTH_5MHZ = TUNER_BANDWIDTH_5MHZ,
	TUA9001_BANDWIDTH_6MHZ = TUNER_BANDWIDTH_6MHZ,
	TUA9001_BANDWIDTH_7MHZ = TUNER_BANDWIDTH_7MHZ,
	TUA9001_BANDWIDTH_8MHZ = TUNER_BANDWIDTH_8MHZ,
};

// Default value
#define TUA9001_RF_FREQ_HZ_DEFAULT			50000000;
#define TUA9001_BANDWIDTH_MODE_DEFAULT		TUA9001_BANDWIDTH_5MHZ;





/// TUA9001 extra module alias
typedef struct TUA9001_EXTRA_MODULE_TAG TUA9001_EXTRA_MODULE;





// Extra manipulaing function
typedef int
(*TUA9001_FP_SET_BANDWIDTH_MODE)(
	TUNER_MODULE *pTuner,
	int BandwidthMode
	);

typedef int
(*TUA9001_FP_GET_BANDWIDTH_MODE)(
	TUNER_MODULE *pTuner,
	int *pBandwidthMode
	);

typedef int
(*TUA9001_FP_GET_REG_BYTES_WITH_REG_ADDR)(
	TUNER_MODULE *pTuner,
	unsigned char DeviceAddr,
	unsigned char RegAddr,
	unsigned char *pReadingByte,
	unsigned char ByteNum
	);

typedef int
(*TUA9001_FP_SET_SYS_REG_BYTE)(
	TUNER_MODULE *pTuner,
	unsigned short RegAddr,
	unsigned char WritingByte
	);

typedef int
(*TUA9001_FP_GET_SYS_REG_BYTE)(
	TUNER_MODULE *pTuner,
	unsigned short RegAddr,
	unsigned char *pReadingByte
	);





// TUA9001 extra module
struct TUA9001_EXTRA_MODULE_TAG
{
	// TUA9001 extra variables
	int BandwidthMode;
	int IsBandwidthModeSet;

	// TUA9001 extra function pointers
	TUA9001_FP_SET_BANDWIDTH_MODE            SetBandwidthMode;
	TUA9001_FP_GET_BANDWIDTH_MODE            GetBandwidthMode;
	TUA9001_FP_GET_REG_BYTES_WITH_REG_ADDR   GetRegBytesWithRegAddr;
	TUA9001_FP_SET_SYS_REG_BYTE              SetSysRegByte;
	TUA9001_FP_GET_SYS_REG_BYTE              GetSysRegByte;
};





// Builder
void
BuildTua9001Module(
	TUNER_MODULE **ppTuner,
	TUNER_MODULE *pTunerModuleMemory,
	TUA9001_EXTRA_MODULE *pTua9001ExtraModuleMemory,
	BASE_INTERFACE_MODULE *pBaseInterfaceModuleMemory,
	I2C_BRIDGE_MODULE *pI2cBridgeModuleMemory,
	unsigned char DeviceAddr
	);





// Manipulaing functions
void
tua9001_GetTunerType(
	TUNER_MODULE *pTuner,
	int *pTunerType
	);

void
tua9001_GetDeviceAddr(
	TUNER_MODULE *pTuner,
	unsigned char *pDeviceAddr
	);

int
tua9001_Initialize(
	TUNER_MODULE *pTuner
	);

int
tua9001_SetRfFreqHz(
	TUNER_MODULE *pTuner,
	unsigned long RfFreqHz
	);

int
tua9001_GetRfFreqHz(
	TUNER_MODULE *pTuner,
	unsigned long *pRfFreqHz
	);





// Extra manipulaing functions
int
tua9001_SetBandwidthMode(
	TUNER_MODULE *pTuner,
	int BandwidthMode
	);

int
tua9001_GetBandwidthMode(
	TUNER_MODULE *pTuner,
	int *pBandwidthMode
	);

int
tua9001_GetRegBytesWithRegAddr(
	TUNER_MODULE *pTuner,
	unsigned char DeviceAddr,
	unsigned char RegAddr,
	unsigned char *pReadingBytes,
	unsigned char ByteNum
	);

int
tua9001_SetSysRegByte(
	TUNER_MODULE *pTuner,
	unsigned short RegAddr,
	unsigned char WritingByte
	);

int
tua9001_GetSysRegByte(
	TUNER_MODULE *pTuner,
	unsigned short RegAddr,
	unsigned char *pReadingByte
	);





// I2C birdge module demod argument setting
void
tua9001_SetI2cBridgeModuleTunerArg(
	TUNER_MODULE *pTuner
	);















#endif
