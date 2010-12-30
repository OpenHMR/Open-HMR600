/******************************************************************************
 * include/linux/mtd/rtk_nand_reg.h
 * Overview: Realtek Nand Flash HW Controller Register Map
 * Copyright (c) 2008 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2008-06-11 Ken-Yu   create file
 * 
 *******************************************************************************/
#ifndef __RTK_NAND_REG_H
#define __RTK_NAND_REG_H

#define DDR_BASE_ADDR	0x80000000

#define REG_BASE_ADDR	0xb8010000

#define	REG_PAGE_ADR0	( REG_BASE_ADDR  + 0 )
#define	REG_PAGE_ADR1	( REG_BASE_ADDR  + 0x4 )
#define	REG_PAGE_ADR2	( REG_BASE_ADDR  + 0x8 )	//ADDR[20:16] = [4:0]
#define	REG_PAGE_ADR3	( REG_BASE_ADDR  + 0x2c )
#define	REG_COL_ADR0		( REG_BASE_ADDR  + 0xc )
#define	REG_COL_ADR1		( REG_BASE_ADDR  + 0x3c )

#define	REG_ADR_MODE	( REG_BASE_ADDR  + 0x8 )	//[7:5]
#define	REG_DATA				( REG_BASE_ADDR  + 0x14 )
#define	REG_CTL					( REG_BASE_ADDR  + 0x18 )

#define	REG_DATA_CNT1	( REG_BASE_ADDR  + 0x100 )
#define	REG_DATA_CNT2	( REG_BASE_ADDR  + 0x104 )

#define	REG_CMD1				( REG_BASE_ADDR  + 0x10 )
#define	REG_CMD2				( REG_BASE_ADDR  + 0x274 )
#define	REG_CMD3				( REG_BASE_ADDR  + 0x28 )

#define REG_ECC_STATE	( REG_BASE_ADDR  + 0x38 )
#define REG_ECC_NUM		( REG_BASE_ADDR  + 0x204 )
#define	REG_ECC_ADR1L	( REG_BASE_ADDR  + 0x208 )
#define	REG_ECC_ADR1H	( REG_BASE_ADDR  + 0x20c )
#define	REG_ECC_ADR2L	( REG_BASE_ADDR  + 0x210 )
#define	REG_ECC_ADR2H	( REG_BASE_ADDR  + 0x214 )
#define	REG_ECC_ADR3L	( REG_BASE_ADDR  + 0x218 )
#define	REG_ECC_ADR3H	( REG_BASE_ADDR  + 0x21c )
#define	REG_ECC_ADR4L	( REG_BASE_ADDR  + 0x220 )
#define	REG_ECC_ADR4H	( REG_BASE_ADDR  + 0x224 )
#define	REG_ECC_STOP		( REG_BASE_ADDR  + 0x264 )
#define	REG_ECC_PAGE		( REG_BASE_ADDR  + 0x268 )
#define	REG_MULTICHNL_MODE		( REG_BASE_ADDR  + 0x27c )

#define	REG_PAGE_CNT		( REG_BASE_ADDR  + 0x26c )
#define	REG_PAGE_LEN		( REG_BASE_ADDR  + 0x270 )

#define REG_CMP_STATE	( REG_BASE_ADDR  + 0x22c )
#define REG_CMP_MASK	( REG_BASE_ADDR  + 0x230 )

#define REG_T3	( REG_BASE_ADDR  + 0x234 )
#define REG_T2	( REG_BASE_ADDR  + 0x238 )
#define REG_T1	( REG_BASE_ADDR  + 0x23c )

#define REG_PP_CTL0		( REG_BASE_ADDR  + 0x110 )	//ADDR[9:8] = [5:4]
#define REG_PP_CTL1		( REG_BASE_ADDR  + 0x12c )	//ADDR[7:0]
#define REG_PP_RDY		( REG_BASE_ADDR  + 0x228 )

#define REG_TABLE_CTL	( REG_BASE_ADDR  + 0x13c )

#define REG_AUTO_TRIG	( REG_BASE_ADDR  + 0x200 )
#define REG_POLL_STATUS	( REG_BASE_ADDR  + 0x30 )

#define REG_SRAM_CTL		( REG_BASE_ADDR  + 0x300 )
#define REG_DMA1_CTL		( REG_BASE_ADDR  + 0x10c )
#define	REG_DMA_CTL1		( REG_BASE_ADDR  + 0x304 )
#define	REG_DMA_CTL2		( REG_BASE_ADDR  + 0x308 )
#define	REG_DMA_CTL3		( REG_BASE_ADDR  + 0x30c )
#define	REG_DMA_CTL4		( REG_BASE_ADDR  + 0x310 )

#define REG_DES_CMD		( REG_BASE_ADDR  + 0x314 )
#define REG_DES_MODE	( REG_BASE_ADDR  + 0x318 )
#define REG_DES_USER		( REG_BASE_ADDR  + 0x31c )
#define	REG_DES_BASE		( REG_BASE_ADDR  + 0x330 )
#define	REG_DES_LIMIT		( REG_BASE_ADDR  + 0x334 )
#define	REG_DES_WP			( REG_BASE_ADDR  + 0x338 )
#define	REG_DES_RP			( REG_BASE_ADDR  + 0x33c )
#define	REG_DES_CNT		( REG_BASE_ADDR  + 0x340 )

#define	REG_CHIP_EN		( REG_BASE_ADDR  + 0x130 )

#define	REG_USER_MASK	( REG_BASE_ADDR  + 0x320 )

#define	REG_ISR			( REG_BASE_ADDR  + 0x324 )
#define	REG_ISREN		( REG_BASE_ADDR  + 0x328 )

#define REG_DELAY_CNT	( REG_BASE_ADDR  + 0x260 )

#define REG_BLANK_CHECK	( REG_BASE_ADDR  + 0x34 )

#endif  /* __RTK_NAND_REG_H */
