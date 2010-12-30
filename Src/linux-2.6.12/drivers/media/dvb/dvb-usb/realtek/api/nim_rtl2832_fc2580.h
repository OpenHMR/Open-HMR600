#ifndef __NIM_RTL2832_FC2580
#define __NIM_RTL2832_FC2580

/**

@file

@brief   RTL2832 FC2580 NIM module declaration

One can manipulate RTL2832 FC2580 NIM through RTL2832 FC2580 NIM module.
RTL2832 FC2580 NIM module is derived from DVB-T NIM module.



@par Example:
@code

// The example is the same as the NIM example in dvbt_nim_base.h except the listed lines.



#include "nim_rtl2832_fc2580.h"


...



int main(void)
{
	DVBT_NIM_MODULE *pNim;
	DVBT_NIM_MODULE DvbtNimModuleMemory;
	RTL2832_EXTRA_MODULE Rtl2832ExtraModuleMemory;
	FC2580_EXTRA_MODULE Fc2580ExtraModuleMemory;

	...



	// Build RTL2832 FC2580 NIM module.
	BuildRtl2832Fc2580Module(
		&pNim,
		&DvbtNimModuleMemory,

		9,								// Maximum I2C reading byte number is 9.
		8,								// Maximum I2C writing byte number is 8.
		CustomI2cRead,					// Employ CustomI2cRead() as basic I2C reading function.
		CustomI2cWrite,					// Employ CustomI2cWrite() as basic I2C writing function.
		CustomWaitMs,					// Employ CustomWaitMs() as basic waiting function.

		&Rtl2832ExtraModuleMemory,		// Employ RTL2832 extra module for RTL2832 module.
		0x20,							// The RTL2832 I2C device address is 0x20 in 8-bit format.
		CRYSTAL_FREQ_28800000HZ,		// The RTL2832 crystal frequency is 28.8 MHz.
		RTL2832_APPLICATION_STB,		// The RTL2832 application mode is STB mode.
		50,								// The RTL2832 update function reference period is 50 millisecond
		ON,								// The RTL2832 Function 1 enabling status is on.

		&Fc2580ExtraModuleMemory,		// Employ FC2580 extra module for FC2580 module.
		0xac,							// The FC2580 I2C device address is 0xac in 8-bit format.
		CRYSTAL_FREQ_16384000HZ,		// The FC2580 crystal frequency is 16.384 MHz.
		FC2580_AGC_EXTERNAL				// The FC2580 AGC mode is external AGC mode.
		);



	// See the example for other NIM functions in dvbt_nim_base.h

	...


	return 0;
}


@endcode

*/


#include "demod_rtl2832.h"
#include "tuner_fc2580.h"
#include "dvbt_nim_base.h"





// Definitions
#define RTL2832_FC2580_ADDITIONAL_INIT_REG_TABLE_LEN		21





// Builder
void
BuildRtl2832Fc2580Module(
	DVBT_NIM_MODULE **ppNim,								// DVB-T NIM dependence
	DVBT_NIM_MODULE *pDvbtNimModuleMemory,

	unsigned char I2cReadingByteNumMax,						// Base interface dependence
	unsigned char I2cWritingByteNumMax,
	BASE_FP_I2C_READ I2cRead,
	BASE_FP_I2C_WRITE I2cWrite,
	BASE_FP_WAIT_MS WaitMs,

	RTL2832_EXTRA_MODULE *pRtl2832ExtraModuleMemory,		// Demod dependence
	unsigned char DemodDeviceAddr,
	unsigned long DemodCrystalFreqHz,
	int DemodAppMode,
	int DemodTsInterfaceMode,
	unsigned long DemodUpdateFuncRefPeriodMs,
	int DemodIsFunc1Enabled,

	FC2580_EXTRA_MODULE *pFc2580ExtraModuleMemory,			// Tuner dependence
	unsigned char TunerDeviceAddr,
	unsigned long TunerCrystalFreqHz,
	int TunerAgcMode
	);





// RTL2832 FC2580 NIM manipulaing functions
int
rtl2832_fc2580_Initialize(
	DVBT_NIM_MODULE *pNim
	);

int
rtl2832_fc2580_SetParameters(
	DVBT_NIM_MODULE *pNim,
	unsigned long RfFreqHz,
	int BandwidthMode
	);







#endif


