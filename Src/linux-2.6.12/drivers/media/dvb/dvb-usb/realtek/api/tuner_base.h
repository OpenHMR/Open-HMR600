#ifndef __TUNER_BASE_H
#define __TUNER_BASE_H

/**

@file

@brief   Tuner base module definition

Tuner base module definitions contains tuner module structure, tuner funciton pointers, and tuner definitions.



@par Example:
@code


#include "demod_xxx.h"
#include "tuner_xxx.h"



int
CustomI2cRead(
	BASE_INTERFACE_MODULE *pBaseInterface,
	unsigned char DeviceAddr,
	unsigned char *pReadingBytes,
	unsigned char ByteNum
	)
{
	...

	return FUNCTION_SUCCESS;

error_status:
	return FUNCTION_ERROR;
}



int
CustomI2cWrite(
	BASE_INTERFACE_MODULE *pBaseInterface,
	unsigned char DeviceAddr,
	const unsigned char *pWritingBytes,
	unsigned char ByteNum
	)
{
	...

	return FUNCTION_SUCCESS;

error_status:
	return FUNCTION_ERROR;
}



void
CustomWaitMs(
	BASE_INTERFACE_MODULE *pBaseInterface,
	unsigned long WaitTimeMs
	)
{
	...
}



int main(void)
{
	BASE_INTERFACE_MODULE *pBaseInterface;
	BASE_INTERFACE_MODULE BaseInterfaceModuleMemory;

	XXX_DEMOD_MODULE *pDemod;
	XXX_DEMOD_MODULE XxxDemodModuleMemory;

	TUNER_MODULE *pTuner;
	TUNER_MODULE TunerModuleMemory;

	I2C_BRIDGE_MODULE I2cBridgeModuleMemory;

	unsigned long RfFreqHz;

	int TunerType;
	unsigned char DeviceAddr;



	// Build base interface module.
	BuildBaseInterface(
		&pBaseInterface,
		&BaseInterfaceModuleMemory,
		9,								// Set maximum I2C reading byte number with 9.
		8,								// Set maximum I2C writing byte number with 8.
		CustomI2cRead,					// Employ CustomI2cRead() as basic I2C reading function.
		CustomI2cWrite,					// Employ CustomI2cWrite() as basic I2C writing function.
		CustomWaitMs					// Employ CustomWaitMs() as basic waiting function.
		);


	// Build dmeod XXX module.
	// Note: Demod module builder will set I2cBridgeModuleMemory for tuner I2C command forwarding.
	//       Must execute demod builder to set I2cBridgeModuleMemory before use tuner functions.
	BuildDemodXxxModule(
		&pDemod,
		&XxxDemodModuleMemory,
		&BaseInterfaceModuleMemory,
		&I2cBridgeModuleMemory,
		...								// Other arguments by each demod module
		)


	// Build tuner XXX module.
	BuildTunerXxxModule(
		&pTuner,
		&TunerModuleMemory,
		&BaseInterfaceModuleMemory,
		&I2cBridgeModuleMemory,
		0xc0,							// Tuner I2C device address is 0xc0 in 8-bit format.
		...								// Other arguments by each demod module
		);





	// ==== Initialize tuner and set its parameters =====

	// Initialize tuner.
	pTuner->Initialize(pTuner);


	// Set tuner parameters. (RF frequency)
	// Note: In the example, RF frequency is 474 MHz.
	RfFreqHz = 474000000;

	pTuner->SetIfFreqHz(pTuner, RfFreqHz);





	// ==== Get tuner information =====

	// Get tuenr type.
	// Note: One can find tuner type in MODULE_TYPE enumeration.
	pTuner->GetTunerType(pTuner, &TunerType);

	// Get tuner I2C device address.
	pTuner->GetDeviceAddr(pTuner, &DeviceAddr);


	// Get tuner parameters. (RF frequency)
	pTuner->GetRfFreqHz(pTuner, &RfFreqHz);



	return 0;
}


@endcode

*/


#include "foundation.h"





/// tuner module pre-definition
typedef struct TUNER_MODULE_TAG TUNER_MODULE;





/**

@brief   Tuner type getting function pointer

One can use TUNER_FP_GET_TUNER_TYPE() to get tuner type.


@param [in]    pTuner       The tuner module pointer
@param [out]   pTunerType   Pointer to an allocated memory for storing tuner type


@note
	-# Tuner building function will set TUNER_FP_GET_TUNER_TYPE() with the corresponding function.


@see   TUNER_TYPE

*/
typedef void
(*TUNER_FP_GET_TUNER_TYPE)(
	TUNER_MODULE *pTuner,
	int *pTunerType
	);





/**

@brief   Tuner I2C device address getting function pointer

One can use TUNER_FP_GET_DEVICE_ADDR() to get tuner I2C device address.


@param [in]    pTuner        The tuner module pointer
@param [out]   pDeviceAddr   Pointer to an allocated memory for storing tuner I2C device address


@retval   FUNCTION_SUCCESS   Get tuner device address successfully.
@retval   FUNCTION_ERROR     Get tuner device address unsuccessfully.


@note
	-# Tuner building function will set TUNER_FP_GET_DEVICE_ADDR() with the corresponding function.

*/
typedef void
(*TUNER_FP_GET_DEVICE_ADDR)(
	TUNER_MODULE *pTuner,
	unsigned char *pDeviceAddr
	);





/**

@brief   Tuner initializing function pointer

One can use TUNER_FP_INITIALIZE() to initialie tuner.


@param [in]   pTuner   The tuner module pointer


@retval   FUNCTION_SUCCESS   Initialize tuner successfully.
@retval   FUNCTION_ERROR     Initialize tuner unsuccessfully.


@note
	-# Tuner building function will set TUNER_FP_INITIALIZE() with the corresponding function.

*/
typedef int
(*TUNER_FP_INITIALIZE)(
	TUNER_MODULE *pTuner
	);





/**

@brief   Tuner RF frequency setting function pointer

One can use TUNER_FP_SET_RF_FREQ_HZ() to set tuner RF frequency in Hz.


@param [in]   pTuner     The tuner module pointer
@param [in]   RfFreqHz   RF frequency in Hz for setting


@retval   FUNCTION_SUCCESS   Set tuner RF frequency successfully.
@retval   FUNCTION_ERROR     Set tuner RF frequency unsuccessfully.


@note
	-# Tuner building function will set TUNER_FP_SET_RF_FREQ_HZ() with the corresponding function.

*/
typedef int
(*TUNER_FP_SET_RF_FREQ_HZ)(
	TUNER_MODULE *pTuner,
	unsigned long RfFreqHz
	);





/**

@brief   Tuner RF frequency getting function pointer

One can use TUNER_FP_GET_RF_FREQ_HZ() to get tuner RF frequency in Hz.


@param [in]   pTuner      The tuner module pointer
@param [in]   pRfFreqHz   Pointer to an allocated memory for storing demod RF frequency in Hz


@retval   FUNCTION_SUCCESS   Get tuner RF frequency successfully.
@retval   FUNCTION_ERROR     Get tuner RF frequency unsuccessfully.


@note
	-# Tuner building function will set TUNER_FP_GET_RF_FREQ_HZ() with the corresponding function.

*/
typedef int
(*TUNER_FP_GET_RF_FREQ_HZ)(
	TUNER_MODULE *pTuner,
	unsigned long *pRfFreqHz
	);





/// Tuner module structure
struct TUNER_MODULE_TAG
{
	unsigned char		b_set_tuner_power_gpio4;					//3add by chialing 2008.08.19
	// Private variables
	int           TunerType;									///<   Tuner type
	unsigned char DeviceAddr;									///<   Tuner I2C device address
	unsigned long RfFreqHz;										///<   Tuner RF frequency in Hz

	int IsRfFreqHzSet;											///<   Tuner RF frequency in Hz (setting status)

	void *pExtra;												///<   Tuner extra module used by driving module
	BASE_INTERFACE_MODULE *pBaseInterface;						///<   Base interface module
	I2C_BRIDGE_MODULE *pI2cBridge;								///<   I2C bridge module


	// Tuner manipulating functions
	TUNER_FP_GET_TUNER_TYPE          GetTunerType;				///<   Tuner type getting function pointer
	TUNER_FP_GET_DEVICE_ADDR         GetDeviceAddr;				///<   Tuner I2C device address getting function pointer

	TUNER_FP_INITIALIZE              Initialize;				///<   Tuner initializing function pointer
	TUNER_FP_SET_RF_FREQ_HZ          SetRfFreqHz;				///<   Tuner RF frequency setting function pointer
	TUNER_FP_GET_RF_FREQ_HZ          GetRfFreqHz;				///<   Tuner RF frequency getting function pointer

};

















#endif
