/* ------------------------------------------------------------------------- */
/* i2c-venus-priv.c  venus i2c hw driver for Realtek Venus DVR I2C           */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2008 Kevin Wang <kevin_wang@realtek.com.tw>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.    
------------------------------------------------------------------------- 
Update List :
------------------------------------------------------------------------- 
    1.1     |   20080530    | Using for loop instead of while loop
-------------------------------------------------------------------------     
    1.1a    |   20080531    | Add 1 ms delay at the end of each xfer 
                            | for timing compatibility with old i2c driver
-------------------------------------------------------------------------     
    1.1b    |   20080531    | After Xfer complete, ISR will Disable All Interrupts 
                            | and Disable I2C first, then wakeup the caller   
-------------------------------------------------------------------------     
    1.1c    |   20080617    | Add API get_tx_abort_reason
-------------------------------------------------------------------------     
    1.1d    |   20080630    | Add I2C bus jammed recover feature.
            |               | send 10 bits clock with gpio 4 to recover bus jam problem.
-------------------------------------------------------------------------     
    1.1e    |   20080702    | Disable GP 4/5 interript during detect bus jam   
-------------------------------------------------------------------------     
    1.1f    |   20080707    | Only do bus jam detect/recover after timeout occurs
-------------------------------------------------------------------------     
    1.2     |   20080711    | modified to support mars i2c
-------------------------------------------------------------------------     
    1.2a    |   20080714    | modified the way of i2c/GPIO selection
-------------------------------------------------------------------------     
    1.3     |   20080729    | Support Non Stop Write Transfer 
-------------------------------------------------------------------------     
    1.3a    |   20080729    | Fix bug of non stop write transfer
            |               |    1) MSB first   
            |               |    2) no stop after write ack
-------------------------------------------------------------------------     
    1.3b    |   20080730    | Support non stop write in Mars            
            |               | Support bus jam recorver in Mars            
-------------------------------------------------------------------------     
    1.3c    |   20080807    | Support minimum delay feature      
-------------------------------------------------------------------------     
    1.4     |   20081016    | Mars I2C_1 Support
-------------------------------------------------------------------------     
    1.5     |   20090330    | Add Spin Lock to Protect venus_i2c data structure
-------------------------------------------------------------------------     
    1.6     |   20090407    | Add FIFO threshold to avoid timing issue caused 
            |               | by performance issue
-------------------------------------------------------------------------     
    1.7     |   20090423    | Add Suspen/Resume Feature       
-------------------------------------------------------------------------     
    2.0     |   20091019    | Add Set Speed feature
-------------------------------------------------------------------------     
    2.0a    |   20091020    | change speed of bus jam recover via spd argument
-------------------------------------------------------------------------     
    2.1     |   20100511    | Support GPIO Read Write
-------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/i2c.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <platform.h>
#include "i2c-venus-priv.h"
#include "i2c-venus-reg.h"


////////////////////////////////////////////////////////////////////
#define wr_reg(x,y)                     writel(y,(volatile unsigned int*)x)
#define rd_reg(x)                       readl((volatile unsigned int*)x)

#ifdef CONFIG_MARS_I2C_EN

#define GET_MUX_PAD0()                  rd_reg(MUX_PAD0)
#define SET_MUX_PAD0(x)                 wr_reg(MUX_PAD0, x)
#define GET_MUX_PAD3()                  rd_reg(MUX_PAD3)
#define SET_MUX_PAD3(x)                 wr_reg(MUX_PAD3, x)
#define GET_MUX_PAD5()                  rd_reg(MUX_PAD5)
#define SET_MUX_PAD5(x)                 wr_reg(MUX_PAD5, x)

#else

#define GET_MIS_PSELL()                 rd_reg(MIS_PSELL)
#define SET_MIS_PSELL(x)                wr_reg(MIS_PSELL, x)

#endif

#define GET_MIS_GP0DIR()                rd_reg(MIS_GP0DIR)
#define GET_MIS_GP0DATO()               rd_reg(MIS_GP0DATO)
#define GET_MIS_GP0DATI()               rd_reg(MIS_GP0DATI)
#define GET_MIS_GP0IE()                 rd_reg(MIS_GP0IE)
#define SET_MIS_GP0DIR(x)               wr_reg(MIS_GP0DIR, x)
#define SET_MIS_GP0DATO(x)              wr_reg(MIS_GP0DATO, x)
#define SET_MIS_GP0DATI(x)              wr_reg(MIS_GP0DATI, x)
#define SET_MIS_GP0IE(x)                wr_reg(MIS_GP0IE, x)

#define SET_MIS_ISR(x)                  wr_reg(MIS_ISR, x)
#define GET_MIS_ISR()                   rd_reg(MIS_ISR)


#define SET_IC_ENABLE(i,x)              wr_reg(IC_ENABLE[i],x)
#define SET_IC_CON(i,x)                 wr_reg(IC_CON[i], x)
#define GET_IC_CON(i)                   rd_reg(IC_CON[i])
#define SET_IC_SAR(i,x)                 wr_reg(IC_SAR[i], x)
#define SET_IC_TAR(i,x)                 wr_reg(IC_TAR[i], x)
#define GET_IC_TAR(i)                   rd_reg(IC_TAR[i])
#define SET_IC_DATA_CMD(i, x)           wr_reg(IC_DATA_CMD[i], x)
#define GET_IC_DATA_CMD(i)              rd_reg(IC_DATA_CMD[i])
                                        
#define SET_IC_SS_SCL_HCNT(i,x)         wr_reg(IC_SS_SCL_HCNT[i], x)
#define SET_IC_SS_SCL_LCNT(i,x)         wr_reg(IC_SS_SCL_LCNT[i], x)
                                        
#define GET_IC_STATUS(i)                rd_reg(IC_STATUS[i])
                                        
#define SET_IC_INTR_MASK(i,x)           wr_reg(IC_INTR_MASK[i], x)
#define GET_IC_INTR_MASK(i)             rd_reg(IC_INTR_MASK[i])
                                        
#define GET_IC_INTR_STAT(i)             rd_reg(IC_INTR_STAT[i])
#define GET_IC_RAW_INTR_STAT(i)         rd_reg(IC_RAW_INTR_STAT[i])
                                        
#define CLR_IC_INTR(i)     	            rd_reg(IC_CLR_INTR[i])
#define CLR_IC_RX_UNDER(i)     	        rd_reg(IC_CLR_RX_UNDER[i])
#define CLR_IC_TX_OVER(i)     	        rd_reg(IC_CLR_TX_OVER[i])
#define CLR_IC_RD_REQ(i)     	        rd_reg(IC_CLR_RD_REQ[i])
#define CLR_IC_RX_DONE(i)     	        rd_reg(IC_CLR_RX_DONE[i])
#define CLR_IC_ACTIVITY(i)     	        rd_reg(IC_CLR_ACTIVITY[i])
#define CLR_IC_GEN_CALL(i)     	        rd_reg(IC_CLR_GEN_CALL[i])
#define CLR_IC_TX_ABRT(i)     	        rd_reg(IC_CLR_TX_ABRT[i])
#define CLR_IC_STOP_DET(i)     	        rd_reg(IC_CLR_STOP_DET[i])
                                        
#define GET_IC_COMP_PARAM_1(i)          rd_reg(IC_COMP_PARAM_1[i])
                                        
#define GET_IC_TXFLR(i)                 rd_reg(IC_TXFLR[i])
#define GET_IC_RXFLR(i)                 rd_reg(IC_RXFLR[i])

#define GET_IC_RX_TL(i)                 rd_reg(IC_RX_TL[i])
#define GET_IC_TX_TL(i)                 rd_reg(IC_TX_TL[i])
#define SET_IC_RX_TL(i, x)              wr_reg(IC_RX_TL[i], x)
#define SET_IC_TX_TL(i, x)              wr_reg(IC_TX_TL[i], x)

                                        
#define GET_IC_TX_ABRT_SOURCE(i)        rd_reg(IC_TX_ABRT_SOURCE[i])
                                        
#define NOT_TXFULL(i)                   (GET_IC_STATUS(i) & ST_TFNF_BIT)
#define NOT_RXEMPTY(i)                  (GET_IC_STATUS(i) & ST_RFNE_BIT)
// for debug

#define i2c_print                       printk
#define dbg_char(x)                     wr_reg(0xb801b200, (unsigned long) (x))

#ifdef SPIN_LOCK_PROTECT_EN    
    #define LOCK_VENUS_I2C(a,b)         spin_lock_irqsave(a, b)     
    #define UNLOCK_VENUS_I2C(a, b)      spin_unlock_irqrestore(a, b)     
#else    
    #define LOCK_VENUS_I2C(a,b)         do { b = 1; }while(0)
    #define UNLOCK_VENUS_I2C(a, b)      do { b = 0; }while(0)
#endif

#ifdef I2C_PROFILEING_EN
#define LOG_EVENT(x)                    log_event(x)
#else
#define LOG_EVENT(x)                    
#endif

#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER           
int  venus_i2c_bus_jam_detect(venus_i2c* p_this);
void venus_i2c_bus_jam_recover_proc(venus_i2c* p_this);
#endif



/*------------------------------------------------------------------
 * Func : venus_gpio_set_ie
 *
 * Desc : set gpio interrupt enable 
 *
 * Parm : id : gpio index
 *
 *        on_off : 0      : disable
 *                 others : enable 
 *
 * Retn : 0 
 *------------------------------------------------------------------*/
static int venus_gpio_set_ie(
    unsigned char           id,
    unsigned char           on_off
    )
{   
    unsigned long MIS_GPIE = MIS_GP0IE; 
    unsigned long GPIO_BIT = GPIO_MASK(id);

    if (id < GPIO_COUNT)    
    {                          
        MIS_GPIE += (GPIO_GROUP(id)<<2); 
        wr_reg(MIS_GPIE, (on_off) ? (rd_reg(MIS_GPIE) | GPIO_BIT) : (rd_reg(MIS_GPIE) & ~GPIO_BIT));            
        //printk("GPIO %d Set IE : [%08x] = %08x\n", id, MIS_GPIE,  rd_reg(MIS_GPIE));            
        return 0;
    }
    
    return -1;        
}    



/*------------------------------------------------------------------
 * Func : venus_gpio_set_dir
 *
 * Desc : set gpio direction 
 *
 * Parm : id : gpio index
 *
 *        dir : 0      : input
 *              others : output 
 *
 * Retn : 0 
 *------------------------------------------------------------------*/
static int venus_gpio_set_dir(
    unsigned char           id,
    unsigned char           dir
    )
{
    unsigned long MIS_GPDIR = MIS_GP0DIR; 
    unsigned long GPIO_BIT = GPIO_MASK(id);

    if (id < GPIO_COUNT)    
    {
        MIS_GPDIR += (GPIO_GROUP(id)<<2); 
        wr_reg(MIS_GPDIR, (dir) ? (rd_reg(MIS_GPDIR) | GPIO_BIT) : (rd_reg(MIS_GPDIR) & ~GPIO_BIT));
        //printk("GPIO %d Set DIR : [%08x] = %08x\n", id, MIS_GPDIR,  rd_reg(MIS_GPDIR));                        
        return 0;
    }

    return -1;        
}           



/*------------------------------------------------------------------
 * Func : venus_gpio_write
 *
 * Desc : set gpio interrupt enable 
 *
 * Parm : id : gpio index
 *
 *        level : 0      : low level
 *                others : high level
 *
 * Retn : 0 
 *------------------------------------------------------------------*/
static int venus_gpio_write(
    unsigned char           id,
    unsigned char           level
    )
{
    unsigned long MIS_GPDATO = MIS_GP0DATO; 
    unsigned long GPIO_BIT = GPIO_MASK(id);

    if (id < GPIO_COUNT)    
    {        
        MIS_GPDATO += (GPIO_GROUP(id)<<2);
        wr_reg(MIS_GPDATO, (level) ? (rd_reg(MIS_GPDATO) | GPIO_BIT) : (rd_reg(MIS_GPDATO) & ~GPIO_BIT));
        //printk("GPIO %d Write : [%08x] = %08x\n", id ,MIS_GPDATO,  rd_reg(MIS_GPDATO));                        
        return 0;
    }
    
    return -1;
}    




/*------------------------------------------------------------------
 * Func : venus_gpio_read
 *
 * Desc : set gpio interrupt enable 
 *
 * Parm : id : gpio index 
 *
 * Retn : 0 / 1
 *------------------------------------------------------------------*/
static int venus_gpio_read(
    unsigned char           id    
    )
{
    unsigned long MIS_GPDATI = MIS_GP0DATI; 
    unsigned long GPIO_BIT = GPIO_MASK(id);

    if (id < GPIO_COUNT)    
    {
        MIS_GPDATI += (GPIO_GROUP(id)<<2); 
        //printk("GPIO %d Read : [%08x] = %08x & %08x\n", id ,MIS_GPDATI,  rd_reg(MIS_GPDATI), GPIO_BIT);                        
        return (rd_reg(MIS_GPDATI) &  GPIO_BIT) ? 1 : 0;    
    }        
    
    return 0;                    
}



/*------------------------------------------------------------------
 * Func : venus_i2c_timer
 *
 * Desc : timer of venus i2c - this timer is used to stop a blocked xfer
 *
 * Parm : arg : handle of venus i2c xfer complete
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/
void venus_i2c_timer(unsigned long arg)
{             
    venus_i2c* p_this = (venus_i2c*) arg;
    unsigned char i = p_this->id;
    
    i2c_print("i2c_%d : time out\n", i);
    LOG_EVENT(EVENT_EXIT_TIMEOUT); 
    SET_IC_INTR_MASK(i, 0);
    SET_IC_ENABLE(i, 0);       
    complete(&p_this->xfer.complete);  // wake up complete
}



/*------------------------------------------------------------------
 * Func : venus_i2c_msater_write
 *
 * Desc : master write handler for venus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *        event  : INT event of venus i2c
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_i2c_msater_write(venus_i2c* p_this, unsigned int event, unsigned int tx_abort_source)
{                                     
#define TxComplete()              (p_this->xfer.tx_len >= p_this->xfer.tx_buff_len)            
    
    unsigned char i = p_this->id;
    
    while(!TxComplete() && NOT_TXFULL(i))
    {
        SET_IC_DATA_CMD(i, p_this->xfer.tx_buff[p_this->xfer.tx_len++]);
    }
    
    if (TxComplete())		    
    {
        SET_IC_INTR_MASK(i, GET_IC_INTR_MASK(i) & ~TX_EMPTY_BIT);     
    }
    
    if (event & TX_ABRT_BIT)
    {        
        p_this->xfer.ret = -ETXABORT;
        p_this->xfer.tx_abort_source = tx_abort_source;
    }        
    else if (event & STOP_DET_BIT)
    {
        p_this->xfer.ret = TxComplete() ? p_this->xfer.tx_len : -ECMDSPLIT;	
    }    
    
    if (p_this->xfer.ret)
    {        
        SET_IC_INTR_MASK(i, 0);
        SET_IC_ENABLE(i, 0);                 
        p_this->xfer.mode = I2C_IDEL;	// change to idle state        
	    complete(&p_this->xfer.complete);
    }          
        
#undef TxComplete
}



/*------------------------------------------------------------------
 * Func : venus_i2c_msater_read
 *
 * Desc : master read handler for venus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_i2c_msater_read(venus_i2c* p_this, unsigned int event, unsigned int tx_abort_source)
{        
#define TxComplete()        (p_this->xfer.tx_len >= p_this->xfer.rx_buff_len)          
#define RxComplete()        (p_this->xfer.rx_len >= p_this->xfer.rx_buff_len)                
    
    unsigned char i = p_this->id;
        
    // TX Thread          
    while(!TxComplete() && NOT_TXFULL(i))
    {
        SET_IC_DATA_CMD(i, READ_CMD);  // send read command to rx fifo
        p_this->xfer.tx_len++;
    }
            
    // RX Thread         
    while (!RxComplete() && NOT_RXEMPTY(i))      
    {
        p_this->xfer.rx_buff[p_this->xfer.rx_len++] = (unsigned char)(GET_IC_DATA_CMD(i) & 0xFF); 
    }        
    
    if (TxComplete())
    {
        SET_IC_INTR_MASK(i, GET_IC_INTR_MASK(i) & ~TX_EMPTY_BIT);     
    }
         
    if (event & TX_ABRT_BIT)
    {                
        p_this->xfer.ret = -ETXABORT;
        p_this->xfer.tx_abort_source = tx_abort_source;
    }                
    else if ((event & STOP_DET_BIT) || RxComplete())
    {
        SET_IC_INTR_MASK(i, GET_IC_INTR_MASK(i) & ~RX_FULL_BIT);
        
        p_this->xfer.ret = RxComplete() ? p_this->xfer.rx_len : -ECMDSPLIT;	
    }
    
    if (p_this->xfer.ret)
    {        
        SET_IC_INTR_MASK(i, 0);
        SET_IC_ENABLE(i, 0);         
        p_this->xfer.mode = I2C_IDEL;	// change to idle state        
	    complete(&p_this->xfer.complete);
    }
    
#undef TxComplete
#undef RxComplete 
}



/*------------------------------------------------------------------
 * Func : venus_i2c_msater_read
 *
 * Desc : master read handler for venus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_i2c_msater_random_read(venus_i2c* p_this, unsigned int event, unsigned int tx_abort_source)
{    
    
#define TxComplete()        (p_this->xfer.tx_len >= (p_this->xfer.rx_buff_len + p_this->xfer.tx_buff_len))    // it should add the same number of read command to tx fifo
#define RxComplete()        (p_this->xfer.rx_len >=  p_this->xfer.rx_buff_len)            
    
    unsigned char i = p_this->id;
        
    // TX Thread        
    while(!TxComplete() && NOT_TXFULL(i))
    {
        if (p_this->xfer.tx_len < p_this->xfer.tx_buff_len)
            SET_IC_DATA_CMD(i, p_this->xfer.tx_buff[p_this->xfer.tx_len]);
        else
            SET_IC_DATA_CMD(i, READ_CMD);  // send read command to rx fifo                        
            
        p_this->xfer.tx_len++;
    }
        
    // RX Thread
    while(!RxComplete() && NOT_RXEMPTY(i))        
    {
        p_this->xfer.rx_buff[p_this->xfer.rx_len++] = (unsigned char)(GET_IC_DATA_CMD(i) & 0xFF); 
    }        
    
    if TxComplete()
    {
        SET_IC_INTR_MASK(i, GET_IC_INTR_MASK(i) & ~TX_EMPTY_BIT);     
    }        

    if (event & TX_ABRT_BIT)
    {        
        p_this->xfer.ret = -ETXABORT;
        p_this->xfer.tx_abort_source = tx_abort_source;
    }           
    else if ((event & STOP_DET_BIT) || RxComplete())
    {
        SET_IC_INTR_MASK(i, GET_IC_INTR_MASK(i) & ~RX_FULL_BIT);  
        p_this->xfer.ret = RxComplete() ? p_this->xfer.rx_len : -ECMDSPLIT;	
    }    

    if (p_this->xfer.ret)
    {        
        SET_IC_INTR_MASK(i, 0);
        SET_IC_ENABLE(i, 0);         
        p_this->xfer.mode = I2C_IDEL;	// change to idle state        
	    complete(&p_this->xfer.complete);
    }          
    
#undef TxComplete
#undef RxComplete 
}




/*------------------------------------------------------------------
 * Func : venus_i2c_isr
 *
 * Desc : isr of venus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/
static 
irqreturn_t 
venus_i2c_isr(
    int                     this_irq, 
    void*                   dev_id, 
    struct pt_regs*         regs
    )
{        
    const unsigned long I2C_INT[] = {I2C0_INT, I2C1_INT};
    venus_i2c* p_this = (venus_i2c*) dev_id;
    unsigned char i = p_this->id;       
    unsigned long flags;
    
    LOCK_VENUS_I2C(&p_this->lock, flags);                   
               
    if (!(GET_MIS_ISR() & I2C_INT[i]))    // interrupt belongs to I2C
    {
        UNLOCK_VENUS_I2C(&p_this->lock, flags);
	    return IRQ_NONE;
    }
	    
    LOG_EVENT(EVENT_ENTER_ISR);      
	
    unsigned int event = GET_IC_INTR_STAT(i);    
    unsigned int tx_abrt_source = GET_IC_TX_ABRT_SOURCE(i);
    
	CLR_IC_INTR(i);	                    // clear interrupts of i2c_x
                                                        
    switch (p_this->xfer.mode)
    {
    case I2C_MASTER_WRITE:
        venus_i2c_msater_write(p_this, event, tx_abrt_source);
        break;
        
    case I2C_MASTER_READ:       
        venus_i2c_msater_read(p_this, event, tx_abrt_source);       
        break;
    
    case I2C_MASTER_RANDOM_READ:
        venus_i2c_msater_random_read(p_this, event, tx_abrt_source);
        break;        

    default:
        printk("Unexcepted Interrupt\n");    
        SET_IC_ENABLE(i, 0);                 
        
    }    
              
    mod_timer(&p_this->xfer.timer, jiffies + I2C_TIMEOUT_INTERVAL);        
        
    SET_MIS_ISR(I2C_INT[i]);   // clear I2C Interrupt Flag
    UNLOCK_VENUS_I2C(&p_this->lock, flags);
    return IRQ_HANDLED;
}


/*------------------------------------------------------------------
 * Func : venus_i2c_set_tar
 *
 * Desc : set tar of venus i2c 
 *
 * Parm : p_this : handle of venus i2c
 *        addr   : address of sar
 *        mode
  : mode of sar
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/
int 
venus_i2c_set_tar(
    venus_i2c*          p_this, 
    unsigned short      addr, 
    ADDR_MODE           mode
    )
{    
    unsigned char i = p_this->id;   
                       
    if (mode==ADDR_MODE_10BITS)
    {        
        if (addr > 0x3FF)                    
            return -EADDROVERRANGE;
        
        SET_IC_TAR(i, addr & 0x3FF);
        SET_IC_CON(i, (GET_IC_CON(i) & (~IC_10BITADDR_MASTER)) | IC_10BITADDR_MASTER);  
    }
    else      
    {   
        if (addr > 0x7F)        
            return -EADDROVERRANGE;
        
        SET_IC_TAR(i, addr & 0x7F);   
        SET_IC_CON(i, GET_IC_CON(i) & (~IC_10BITADDR_MASTER));           
    }
    
    p_this->tar      = addr;
    p_this->tar_mode = mode;
    
    return 0;                    
}  
  


/*------------------------------------------------------------------
 * Func : venus_i2c_set_sar
 *
 * Desc : set sar of venus i2c 
 *
 * Parm : p_this : handle of venus i2c
 *        addr   : address of sar
 *        mode
  : mode of sar
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/      
int 
venus_i2c_set_sar(
    venus_i2c*          p_this, 
    unsigned short      addr, 
    ADDR_MODE           mode
    )
{                
    unsigned char i = p_this->id;   
           
    if (mode==ADDR_MODE_10BITS)
    {
        SET_IC_SAR(i, p_this->sar & 0x3FF);
        SET_IC_CON(i, (GET_IC_CON(i) & (~IC_10BITADDR_SLAVE)) | IC_10BITADDR_SLAVE);  
    }
    else      
    {           
        SET_IC_SAR(i, p_this->sar & 0x7F);   
        SET_IC_CON(i, GET_IC_CON(i) & (~IC_10BITADDR_SLAVE));           
    }
    
    p_this->sar      = addr;
    p_this->sar_mode = mode;
    
    return 0;                    
}  


    
/*------------------------------------------------------------------
 * Func : venus_i2c_set_spd
 *
 * Desc : set speed of venus i2c 
 *
 * Parm : p_this : handle of venus i2c
 *        KHz    : operation speed of i2c
 *         
 * Retn : 0
 *------------------------------------------------------------------*/    
int venus_i2c_set_spd(venus_i2c* p_this, int KHz)
{                     
    unsigned char i = p_this->id;   
    unsigned int div_h = 0x7a;
    unsigned int div_l = 0x8f;
    
    if (KHz < 10 || KHz > 150)    
    {
        i2c_print("[I2C%d] warning, speed %d out of range, speed should between 10 ~ 150KHz\n", i, KHz);                
        return -1;
    }        
                     
    div_h = (div_h * 100)/KHz;
    div_l = (div_l * 100)/KHz;        
    
    if (div_h >= 0xFFFF || div_h==0 || 
        div_l >= 0xFFFF || div_l==0)
    {
        i2c_print("[I2C%d] fatal, set speed failed : divider divider out of range. div_h = %d, div_l = %d\n", i, div_h, div_l);        
        return -1;
    }            
    
    SET_IC_CON(i, (GET_IC_CON(i) & (~IC_SPEED)) | SPEED_SS);
    SET_IC_SS_SCL_HCNT(i, div_h);
    SET_IC_SS_SCL_LCNT(i, div_l);
    p_this->spd  = KHz;
    p_this->tick = 1000 / KHz;
    i2c_print("[I2C%d] i2c speed changed to %d KHz\n", i, p_this->spd);
    return 0;
}    



/*------------------------------------------------------------------
 * Func : venus_i2c_dump
 *
 * Desc : dump staus of venus i2c 
 *
 * Parm : p_this : handle of venus i2c
 *         
 * Retn : 0 for success
 *------------------------------------------------------------------*/    
int venus_i2c_dump(venus_i2c* p_this)
{   
    i2c_print("=========================\n"); 
    i2c_print("= VER : %s               \n", VERSION);
    i2c_print("=========================\n");
    i2c_print("= PHY : %d               \n", p_this->id);    
    i2c_print("= MODE: %s               \n", MODLE_NAME);
    i2c_print("= SPD : %d               \n", p_this->spd);
    i2c_print("= SAR : 0x%03x (%d bits) \n", p_this->sar, p_this->sar_mode);
    i2c_print("= TX FIFO DEPTH : %d     \n", p_this->tx_fifo_depth);
    i2c_print("= RX FIFO DEPTH : %d     \n", p_this->rx_fifo_depth);  
    i2c_print("= FIFO THRESHOLD: %d     \n", FIFO_THRESHOLD);

#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER           
    i2c_print("= BUS JAM RECORVER 3: ON  \n");     
#else    
    i2c_print("= BUS JAM RECORVER 3: OFF  \n");
#endif

#ifdef CONFIG_I2C_VENUS_NON_STOP_WRITE_XFER           
    i2c_print("= NON STOP WRITE : ON  \n");     
#else    
    i2c_print("= NON STOP WRITE : OFF  \n");
#endif

    i2c_print("= GPIO RW SUPPORT : ON \n");
    i2c_print("=========================\n");    
    return 0;
}




/*------------------------------------------------------------------
 * Func : venus_i2c_probe
 *
 * Desc : probe venus i2c
 *
 * Parm : p_this : handle of venus i2c
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/    
int venus_i2c_probe(venus_i2c* p_this)
{                      
    if (p_this->id >= MAX_I2C_CNT)  
        return -ENODEV;
     
#ifdef CONFIG_MARS_I2C_EN    

    // Jupiter
    if (is_jupiter_cpu())
    {
        // Jupiter has 2 deticated I2C bus, so we don't need to check the configuration
        if (p_this->id==0)
        {   
            // I2C0 : MUX PAD 10
            p_this->gpio_map.muxpad      = 0xb8000378;
            p_this->gpio_map.muxpad_mask =(0xF << 22);
            p_this->gpio_map.muxpad_gpio = 0x0;
            p_this->gpio_map.muxpad_i2c  =(0x5 << 22);                             
            p_this->gpio_map.scl         = 61;
            p_this->gpio_map.sda         = 62;                                    
        }                
        else
        {            
            // I2C1 : MUX PAD 10
            p_this->gpio_map.muxpad      = 0xb8000378;
            p_this->gpio_map.muxpad_mask =(0xF << 26);
            p_this->gpio_map.muxpad_gpio = 0x0;
            p_this->gpio_map.muxpad_i2c  =(0x5 << 26);                      
            p_this->gpio_map.scl         = 63;
            p_this->gpio_map.sda         = 64;                        
        }
        
        return 0;
    }        
        

    // MARS     
    if (p_this->id==0)
    {
        switch(GET_MUX_PAD0() & I2C0_LOC_MASK)
        {
        case I2C0_LOC_QFPA_BGA: 
        case I2C0_LOC_QFPB:
        
            if ((GET_MUX_PAD3() & I2C0_MASK)== I2C0_SDA_SCL) 
            {        
                p_this->gpio_map.muxpad      = MUX_PAD3;
                p_this->gpio_map.muxpad_mask = I2C0_MASK;
                p_this->gpio_map.muxpad_gpio = I2C0_GP4_GP5;
                p_this->gpio_map.muxpad_i2c  = I2C0_SDA_SCL;                                            
                p_this->gpio_map.scl         = 4;
                p_this->gpio_map.sda         = 5;
            }
            else
            {
                i2c_print("FATAL : I2C %d pins have been occupied for other application\n", p_this->id);
                return -ENODEV;
            }            
            break;
            
        default:
            i2c_print("FATAL : I2C %d does not exist\n", p_this->id);
            return -ENODEV;    
        }
        
    }
    else
    {           
        switch(GET_MUX_PAD0() & I2C1_LOC_MASK)
        {
        case I2C1_LOC_MUX_PCI: 
                    
            switch(GET_MUX_PAD5() & PCI_MUX1_MASK)
            {
            case PCI_MUX1_I2C1:                
                p_this->gpio_map.muxpad      = MUX_PAD5;
                p_this->gpio_map.muxpad_mask = PCI_MUX1_MASK;
                p_this->gpio_map.muxpad_gpio = PCI_MUX1_GPIO;
                p_this->gpio_map.muxpad_i2c  = PCI_MUX1_I2C1;                                       
                p_this->gpio_map.scl         = 40;
                p_this->gpio_map.sda         = 41;                  
                break;
                
            case PCI_MUX1_PCI:
                i2c_print("FATAL : I2C %d pins have been occupied by PCI\n", p_this->id);
                return -ENODEV;                
                                    
            case PCI_MUX1_GPIO:
                i2c_print("FATAL : I2C %d pins have been occupied by GPIO[40:41]\n", p_this->id);
                return -ENODEV;                
                
            default:
                i2c_print("FATAL : I2C %d pins have been reserved for other application\n", p_this->id);
                return -ENODEV;                                            
            }  
            break;   
                    
        case I2C1_LOC_MUX_UR0:
            
            switch(GET_MUX_PAD3() & I2C1_MASK)
            {
            case I2C1_SDA_SCL:                
                p_this->gpio_map.muxpad      = MUX_PAD3;                
                p_this->gpio_map.muxpad_mask = I2C1_MASK;
                p_this->gpio_map.muxpad_gpio = I2C1_GP0_GP1;    
                p_this->gpio_map.muxpad_i2c  = I2C1_SDA_SCL;                                
                p_this->gpio_map.scl         = 0;
                p_this->gpio_map.sda         = 1;                
                break;
                
            case I2C1_UART1:
                i2c_print("FATAL : I2C %d pins have been occupied by UART1\n", p_this->id);
                return -ENODEV;                
                                    
            case I2C1_GP0_GP1:
                i2c_print("FATAL : I2C %d pins have been occupied by GPIO[1:0]\n", p_this->id);
                return -ENODEV;                
                
            default:
                i2c_print("FATAL : I2C %d pins have been reserved for other application\n", p_this->id);
                return -ENODEV;                                            
            }            
            break;
            
        default:
            i2c_print("FATAL : I2C %d does not exist\n", p_this->id);
            return -ENODEV;    
        }                                                           
    }
    
#else

    // VENUS / NEPTUNE
    if ((GET_MIS_PSELL() & 0x00000f00) != 0x00000500) 
    {
        i2c_print("FATAL : GPIO 4/5 pins are not defined as I2C\n");
        return -ENODEV;
    }
    
    p_this->gpio_map.muxpad      = MIS_PSELL;                
    p_this->gpio_map.muxpad_mask = 0x00000f00;
    p_this->gpio_map.muxpad_gpio = 0x00000000;    
    p_this->gpio_map.muxpad_i2c  = 0x00000500;              
    p_this->gpio_map.scl         = 4;
    p_this->gpio_map.sda         = 5;
    
#endif    

    return 0;
}



/*------------------------------------------------------------------
 * Func : venus_i2c_init
 *
 * Desc : init venus i2c
 *
 * Parm : p_this : handle of venus i2c
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/    
int venus_i2c_init(venus_i2c* p_this)
{               
    unsigned char i = p_this->id;  
        
    if (venus_i2c_probe(p_this)<0) 
        return -ENODEV;    
         
    if (request_irq(p_this->irq, venus_i2c_isr, SA_INTERRUPT | SA_SHIRQ, "Venus I2C", p_this) < 0) 
    {
        i2c_print("FATAL : Request irq%d failed\n", p_this->irq);	    		
        return -ENODEV;
    }            
  
    p_this->rx_fifo_depth = ((GET_IC_COMP_PARAM_1(i) >>  8) & 0xFF)+1;
    p_this->tx_fifo_depth = ((GET_IC_COMP_PARAM_1(i) >> 16) & 0xFF)+1; 
    spin_lock_init(&p_this->lock);
        
    SET_IC_ENABLE(i, 0);
    SET_IC_INTR_MASK(i, 0);                // disable all interrupt      
    SET_IC_CON(i, IC_SLAVE_DISABLE | IC_RESTART_EN | SPEED_SS | IC_MASTER_MODE);  
    SET_IC_TX_TL(i, FIFO_THRESHOLD);
    SET_IC_RX_TL(i, p_this->rx_fifo_depth - FIFO_THRESHOLD);
     
    venus_i2c_set_spd(p_this, p_this->spd);
    venus_i2c_set_sar(p_this, p_this->sar, p_this->sar_mode);    

#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER         
    if (venus_i2c_bus_jam_detect(p_this))
    {                        
        i2c_print("I2C%d Bus Status Check.... Error... Try to Recrver\n",p_this->id);
        venus_i2c_bus_jam_recover_proc(p_this);                                        
    }
    else
        i2c_print("I2C%d Bus Status Check.... OK\n",p_this->id);
#endif
    
    venus_i2c_dump(p_this);
    return 0;
}    



/*------------------------------------------------------------------
 * Func : venus_i2c_uninit
 *
 * Desc : uninit venus i2c
 *
 * Parm : p_this : handle of venus i2c
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/    
int venus_i2c_uninit(venus_i2c* p_this)
{       
    unsigned char i = p_this->id;
    
    SET_IC_ENABLE(i, 0);
    SET_IC_INTR_MASK(i, 0);    
    free_irq(p_this->irq, p_this);     
    return 0;
}    
 
 
 
 
enum {
    I2C_MODE    = 0,
    GPIO_MODE   = 1
};



/*------------------------------------------------------------------
 * Func : venus_i2c_gpio_selection
 *
 * Desc : select i2c/GPIO mode
 *
 * Parm : p_this : handle of venus i2c
 *        mode : 0      : SDA / SCL
 *               others : GPIO
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void venus_i2c_gpio_selection(venus_i2c* p_this, unsigned char mode)
{        
    unsigned long val = rd_reg(p_this->gpio_map.muxpad);     
    
    val &= ~p_this->gpio_map.muxpad_mask;

    val |= (mode==GPIO_MODE) ? p_this->gpio_map.muxpad_gpio 
                             : p_this->gpio_map.muxpad_i2c;
    
    wr_reg(p_this->gpio_map.muxpad, val);    
    
    //printk("GPIO Selection: [%08x] = %08x & %08x\n", p_this->gpio_map.muxpad, rd_reg(p_this->gpio_map.muxpad), p_this->gpio_map.muxpad_mask);
}
    
        
    
/*------------------------------------------------------------------
 * Func : venus_i2c_suspend
 *
 * Desc : suspend venus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 for success
 *------------------------------------------------------------------*/    
int venus_i2c_suspend(venus_i2c* p_this)
{           
#ifdef GPIO_MODE_SUSPEND

    unsigned char sda = p_this->gpio_map.sda;
    unsigned char scl = p_this->gpio_map.scl;    

    i2c_print("[I2C%d] suspend\n", p_this->id);    
    
    while (p_this->xfer.mode!=I2C_IDEL)    
        msleep(1);

    venus_gpio_set_dir(sda, 0);
    venus_gpio_set_dir(scl, 0);   
    
    venus_gpio_set_ie(sda, 0);
    venus_gpio_set_ie(scl, 0);   
            
    venus_i2c_gpio_selection(p_this, GPIO_MODE); 
#endif

    return 0;
}


/*------------------------------------------------------------------
 * Func : venus_i2c_resume
 *
 * Desc : resumevenus i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 for success
 *------------------------------------------------------------------*/    
int venus_i2c_resume(venus_i2c* p_this)
{   
#ifdef GPIO_MODE_SUSPEND
    i2c_print("[I2C%d] resume\n", p_this->id);                    
    venus_i2c_gpio_selection(p_this, I2C_MODE);   
#endif 
    return 0;
}



/*------------------------------------------------------------------
 * Func : venus_i2c_reset_state
 *
 * Desc : reset internal state machine of i2c controller.
 *        
 *        This is a hack that used to reset the internal state machine 
 *        of venus i2c. In mars, there is no way to reset the internal
 *        state of venus I2C controller. However, we found out that we
 *        can use GPIO to generate a pseudo stop to reset it. 
 *
 *        First, we need to set the i2c bus to CPIO mode and pull low 
 *        SDA and pull high SCL, then changed the i2c bus to I2C mode. 
 *        Because SDA has a pull high resistor, so the i2c controller 
 *        will see SDA falling and rising when SCL is highw. It will be 
 *        looked like start & stop and the state of i2c controller will 
 *        be reset.
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void venus_i2c_reset_state(venus_i2c* p_this)
{    
    unsigned char sda = p_this->gpio_map.sda;
    unsigned char scl = p_this->gpio_map.scl;  
    int d = p_this->tick / 2;             
    
     // Disable GPIO Interrupt
    venus_gpio_set_ie(sda, 0);   
    venus_gpio_set_ie(scl, 0);

    // pull high SCL & pull low SDA            
    venus_gpio_write(sda, 0);
    venus_gpio_write(scl, 1);
    
    // mode : SCL : out, SDA : out
    venus_gpio_set_dir(sda, 1);
    venus_gpio_set_dir(scl, 1);      
    
    venus_i2c_gpio_selection(p_this, GPIO_MODE);
    
    udelay(d);    
        
    venus_i2c_gpio_selection(p_this, I2C_MODE);
    venus_gpio_set_dir(sda, 0);
    venus_gpio_set_dir(scl, 0); 
}


#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER 

/*------------------------------------------------------------------
 * Func : venus_i2c_bus_jam_recover
 *
 * Desc : recover i2c bus jam status 
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void venus_i2c_bus_jam_recover(venus_i2c* p_this)
{        
    unsigned char sda = p_this->gpio_map.sda;
    unsigned char scl = p_this->gpio_map.scl;    
    int i;
    int d = p_this->tick / 2;                    
    
    // Disable GPIO Interrupt
    venus_gpio_set_ie(sda, 0);   
    venus_gpio_set_ie(scl, 0);

    // pull low SCL & SDA            
    venus_gpio_write(sda, 0);    
    venus_gpio_write(scl, 0);      
    
    // mode : SCL : out, SDA : out
    venus_gpio_set_dir(sda, 1);
    venus_gpio_set_dir(scl, 1);                        
        
    venus_i2c_gpio_selection(p_this, GPIO_MODE);
    
    //Add Stop Condition
	udelay(10);
	venus_gpio_write(scl, 1);            // pull high SCL
	udelay(10);
	venus_gpio_write(sda, 1);            // pull high SDA                
	udelay(10);
    
    venus_gpio_set_dir(sda, 0);          // mode SDA : in    
        
    // Output Clock Modify Clock Output from 10 to 9
    for (i=0; i<9; i++)
    {
        venus_gpio_write(scl, 0);        // pull low SCL               
        udelay(d);
        venus_gpio_write(scl, 1);        // pull high SCL                     
        udelay(d);
    }
    
    venus_gpio_set_dir(scl, 0);          // mode SCL : in        

    venus_i2c_gpio_selection(p_this, I2C_MODE);
}



/*------------------------------------------------------------------
 * Func : venus_i2c_bus_jam_detect
 *
 * Desc : check if bus jam occurs
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 : bus not jammed, 1 : bus jammed 
 *------------------------------------------------------------------*/
int venus_i2c_bus_jam_detect(venus_i2c* p_this)
{               
    unsigned char sda = p_this->gpio_map.sda;
    unsigned char scl = p_this->gpio_map.scl;    
    int ret = 1;
    int i;                    
                                                            
    // GPIO Interrupt Disable                                
    venus_gpio_set_ie(sda, 0);
    venus_gpio_set_ie(scl, 0);
        
    // GPIO Dir=Input                    
    venus_gpio_set_dir(sda, 0);
    venus_gpio_set_dir(scl, 0);
    
    venus_i2c_gpio_selection(p_this, GPIO_MODE);
		
    for(i=0; i<30; i++)
    {
        if (venus_gpio_read(sda) &&  venus_gpio_read(scl))      // SDA && SCL == High               
        {
            ret = 0;
            break;
        }
        msleep(1);
    }
    
    if (ret)
    {        
        i2c_print("I2C %d Jamed, BUS Status: SDA=%d, SCL=%d\n", 
                p_this->id, venus_gpio_read(sda), venus_gpio_read(scl));
    }        
                
    venus_i2c_gpio_selection(p_this, I2C_MODE);
    
    return ret;    
}




/*------------------------------------------------------------------
 * Func : venus_i2c_bus_jam_recover
 *
 * Desc : recover i2c bus jam status 
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void venus_i2c_bus_jam_recover_proc(venus_i2c* p_this)
{
    int i = 0;
    
    do
    {   
        i2c_print("Do I2C%d Bus Recover %d\n",p_this->id, i);
    
        venus_i2c_bus_jam_recover(p_this);
        
        msleep(200);                
        
        if (venus_i2c_bus_jam_detect(p_this)==0)               
        {                 
            i2c_print("I2C%d Bus Recover successed\n",p_this->id);
            return ;
        }
            
    }while(i++ < 3);
    
    i2c_print("I2C%d Bus Recover failed\n",p_this->id);
}
#endif


/*------------------------------------------------------------------
 * Func : venus_i2c_start_xfer
 *
 * Desc : start xfer message
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 for success, others is failed
 *------------------------------------------------------------------*/
int venus_i2c_start_xfer(venus_i2c* p_this)
{        
    unsigned char i = p_this->id;    
    unsigned long flags;
    int ret;
    int mode = p_this->xfer.mode;

    LOG_EVENT(EVENT_START_XFER);
    
    LOCK_VENUS_I2C(&p_this->lock, flags);
            
    switch (p_this->xfer.mode)
    {
    case I2C_MASTER_WRITE:  
        SET_IC_INTR_MASK(i, TX_EMPTY_BIT | TX_ABRT_BIT | STOP_DET_BIT);
        break;
        
    case I2C_MASTER_READ:    
    case I2C_MASTER_RANDOM_READ:   
    
        if (GET_IC_RXFLR(i)) 
        {
            printk("WARNING, RX FIFO NOT EMPRY\n");
            while(GET_IC_RXFLR(i))  
                 GET_IC_DATA_CMD(i);            
        }
    
        SET_IC_INTR_MASK(i, RX_FULL_BIT | TX_EMPTY_BIT | TX_ABRT_BIT | STOP_DET_BIT);    
        break;
            
    default:
        UNLOCK_VENUS_I2C(&p_this->lock, flags);       
        LOG_EVENT(EVENT_STOP_XFER);        
        return -EILLEGALMSG;
    }                

#ifdef MINIMUM_DELAY_EN

    UNLOCK_VENUS_I2C(&p_this->lock, flags);
    
    if (jiffies <= p_this->time_stamp)
        udelay(1000);

    LOCK_VENUS_I2C(&p_this->lock, flags);
    
#endif 
                        
    p_this->xfer.timer.expires  = jiffies + I2C_TIMEOUT_INTERVAL;    
    add_timer(&p_this->xfer.timer);
                                    
    SET_IC_ENABLE(i, 1);                   // Start Xfer
    
    UNLOCK_VENUS_I2C(&p_this->lock, flags);       
    
    wait_for_completion(&p_this->xfer.complete);
    
    LOCK_VENUS_I2C(&p_this->lock, flags);  
    
    del_timer(&p_this->xfer.timer);
    
    if (p_this->xfer.mode != I2C_IDEL)
    {
        p_this->xfer.ret  = -ETIMEOUT;
        
#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER

        UNLOCK_VENUS_I2C(&p_this->lock, flags);       

        // Bus Jammed Recovery Procedure
        if (venus_i2c_bus_jam_detect(p_this))
        {
            printk("[I2C] WARNING, I2C Bus Jammed, Do Recorver\n");        
            venus_i2c_bus_jam_recover_proc(p_this);
            msleep(50);
        }
        
        printk("[I2C] Info, Reset I2C State\n");
        venus_i2c_reset_state(p_this);                    

        LOCK_VENUS_I2C(&p_this->lock, flags);  
#endif
    }
    else if (p_this->xfer.ret==-ECMDSPLIT)
    {
        switch(mode)
        {
        case I2C_MASTER_WRITE:          
            printk("WARNING, Write Cmd Split, tx : %d/%d\n", 
                    p_this->xfer.tx_len, p_this->xfer.tx_buff_len);                     
            break;
        
        case I2C_MASTER_READ:                
            printk("WARNING, Read Cmd Split, tx : %d/%d rx : %d/%d\n", 
                    p_this->xfer.tx_len, p_this->xfer.tx_buff_len,
                    p_this->xfer.rx_len, p_this->xfer.rx_buff_len);
            break;                    
            
        case I2C_MASTER_RANDOM_READ:              
            printk("WARNING, Read Cmd Split, tx : %d/%d rx : %d/%d\n", 
                    p_this->xfer.tx_len, p_this->xfer.tx_buff_len + p_this->xfer.rx_buff_len,
                    p_this->xfer.rx_len, p_this->xfer.rx_buff_len);
            break;                                        
        }        
    }

#ifdef MINIMUM_DELAY_EN            
    p_this->time_stamp = (unsigned long) jiffies;
#endif    

    ret = p_this->xfer.ret;
    
    UNLOCK_VENUS_I2C(&p_this->lock, flags);       

#ifndef MINIMUM_DELAY_EN
    udelay(1200);
#endif

    if (ret==-ECMDSPLIT)
    {
        if (venus_i2c_probe(p_this)<0)
            printk("WARNING, I2C %d no longer exists\n", p_this->id);
    }    
    
    LOG_EVENT(EVENT_STOP_XFER);            
    
    return ret;
}



/*------------------------------------------------------------------
 * Func : venus_g2c_do_start
 *
 * Desc : gpio i2c xfer - start phase
 *
 * Parm : p_this : handle of venus i2c  
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_g2c_do_start(venus_i2c* p_this)
{
    unsigned long sda = p_this->gpio_map.sda;
    unsigned long scl = p_this->gpio_map.scl; 
    
    switch(p_this->xfer.gpio_xfer_sub_state)
    {
    case 0: 
        venus_gpio_set_ie(sda, 0);
        venus_gpio_set_ie(scl, 0);
        venus_gpio_set_dir(sda, 0);          // SDA DIR = IN  
        venus_gpio_set_dir(scl, 0);          // SCL DIR = IN
        venus_gpio_write(sda, 0);            // SDA = L
        venus_gpio_write(scl, 0);            // SCL = L            
        venus_i2c_gpio_selection(p_this, GPIO_MODE);        
        p_this->xfer.gpio_xfer_sub_state++;
        break;
        
    case 1:
        if (venus_gpio_read(scl) && venus_gpio_read(sda))       // Wait SDA = SCL = H                              
            p_this->xfer.gpio_xfer_sub_state++;
            
        break;
        
    case 2:               
        venus_gpio_set_dir(sda, 1);             // SDA = L            
        p_this->xfer.gpio_xfer_sub_state++;
        break;
                 
    case 3:                        
        venus_gpio_set_dir(scl, 1);             // SCL = L
        p_this->xfer.gpio_xfer_state = G2C_ST_ADDR0;
        p_this->xfer.gpio_xfer_sub_state = 0;
        break;
    }    
}




/*------------------------------------------------------------------
 * Func : venus_g2c_do_address
 *
 * Desc : gpio i2c xfer - address phase
 *
 * Parm : p_this : handle of venus i2c  
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_g2c_do_address(venus_i2c* p_this)
{
    unsigned long sda = p_this->gpio_map.sda;
    unsigned long scl = p_this->gpio_map.scl; 
    unsigned char state = G2C_MINOR_STATE(p_this->xfer.gpio_xfer_state);
    int bit_index = 0;
    unsigned char addr;
    
    if (state <= 7)        
    {
        //--------------------------------------------------
        // ADDR Phase 0 ~ 7
        //--------------------------------------------------
                    
        switch(p_this->xfer.gpio_xfer_sub_state)
        {
        case 0:
            venus_gpio_set_dir(scl, 1);             // SCL = L            
            p_this->xfer.gpio_xfer_sub_state++;
            break;            
            
        case 1:           
            
            addr = p_this->tar << 1;
            
            if (p_this->xfer.mode == I2C_MASTER_READ)
                addr |= 1;
                
            bit_index = 7 - state;   

            if ((addr>>bit_index) & 0x1)
                venus_gpio_set_dir(sda, 0);         // SDA = H            
            else 
                venus_gpio_set_dir(sda, 1);         // SDA = L                        
            
            p_this->xfer.gpio_xfer_sub_state++;
            break;            
            
        case 2:
            
            addr = p_this->tar << 1;
            
            if (p_this->xfer.mode == I2C_MASTER_READ)
                addr |= 1;

            bit_index = 7 - state;

            if (((addr>>bit_index) & 0x1) && venus_gpio_read(sda)==0)
            {
                // lose of arbitraction 
                p_this->xfer.ret = -ETXABORT;
                p_this->xfer.gpio_xfer_state = G2C_ST_DONE;
                p_this->xfer.gpio_xfer_sub_state = 0;
            }
            else
            {
                venus_gpio_set_dir(scl, 0);             // SCL = H
                p_this->xfer.gpio_xfer_sub_state++;
            }
                            
            break;            
            
        case 3:

            if (venus_gpio_read(scl))               // Wait SCL = H
            {
                p_this->xfer.gpio_xfer_state++;                
                p_this->xfer.gpio_xfer_sub_state = 0;                                
            }
            break;
        }        
    }
    else if (state==8)
    {        
        //--------------------------------------------------
        // ADDR ACK & NACK
        //--------------------------------------------------        
        switch(p_this->xfer.gpio_xfer_sub_state)
        {
        case 0: 
            venus_gpio_set_dir(scl, 1);             // Pull low SCL                                     
            p_this->xfer.gpio_xfer_sub_state++;
            break;   
            
        case 1:            
            venus_gpio_set_dir(sda, 0);             // SDA = H     
            p_this->xfer.gpio_xfer_sub_state++;
            break;        
            
        case 2:     
            venus_gpio_set_dir(scl, 0);             // SCL = H
            p_this->xfer.gpio_xfer_sub_state++;
            break;                    
            
        case 3:            
            if (venus_gpio_read(scl))               // Wait SCL = H
            {
                if (venus_gpio_read(sda))
                {
                    p_this->xfer.ret = -ETXABORT;    
                    p_this->xfer.gpio_xfer_state = G2C_ST_STOP;     // NACK or no data to xfer
                }
                else
                {                    
                    p_this->xfer.gpio_xfer_state = G2C_ST_DATA0;    // ACK and still has data to xfer
                }
                    
                p_this->xfer.gpio_xfer_sub_state = 0;                                
            }                            
            break;
        }        
    }   
}




/*------------------------------------------------------------------
 * Func : venus_g2c_do_read
 *
 * Desc : gpio i2c xfer - read data phase
 *
 * Parm : p_this : handle of venus i2c  
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_g2c_do_read(venus_i2c* p_this)
{
    unsigned long sda = p_this->gpio_map.sda;
    unsigned long scl = p_this->gpio_map.scl; 
    unsigned char state = G2C_MINOR_STATE(p_this->xfer.gpio_xfer_state);
    int bit_index = 0;
    
    if (state < 8)
    {
        //--------------------------------------------------
        // DATA Phase 0 ~ 7
        //--------------------------------------------------
                    
        switch(p_this->xfer.gpio_xfer_sub_state)
        {
        case 0:
            venus_gpio_set_dir(scl, 1);             // SCL = L
            p_this->xfer.gpio_xfer_sub_state++;
            break;            
            
        case 1:            
            venus_gpio_set_dir(sda, 0);             // SDA = In            
            p_this->xfer.gpio_xfer_sub_state++;
            break;            
            
        case 2:            
            venus_gpio_set_dir(scl, 0);             // SCL = H
            p_this->xfer.gpio_xfer_sub_state++;
            break;            
            
        case 3:
            if (venus_gpio_read(scl))               // Wait SCL = H
            {                                
                if (venus_gpio_read(sda))
                {            
                    bit_index = 7 - state;                        
                    p_this->xfer.rx_buff[p_this->xfer.rx_len] |= (1<<bit_index);
                }
                
                p_this->xfer.gpio_xfer_state++;     // Next State
                p_this->xfer.gpio_xfer_sub_state = 0;
            }
            break;
        }        
    }                
    else
    {        
        //--------------------------------------------------
        // ACK & NACK
        //--------------------------------------------------
        
        switch(p_this->xfer.gpio_xfer_sub_state)
        {
        case 0: 
            venus_gpio_set_dir(scl, 1);             // SCL = L                        
            p_this->xfer.gpio_xfer_sub_state++;
            p_this->xfer.rx_len++;
            break;   
            
        case 1:                                    
            if (p_this->xfer.rx_len < p_this->xfer.rx_buff_len)
            {
                venus_gpio_set_dir(sda, 1);             // SDA = L  ACK
                //printk(KERN_DEBUG "rx = %d/%d ACK\n", p_this->xfer.rx_len, p_this->xfer.rx_buff_len);                
            }                
            else                
            {   
                venus_gpio_set_dir(sda, 0);             // SDA = H  NACK
                //printk(KERN_DEBUG "rx = %d/%d NACK\n", p_this->xfer.rx_len, p_this->xfer.rx_buff_len);
            }                
                            
            p_this->xfer.gpio_xfer_sub_state++;
            break;        

        case 2:     
            venus_gpio_set_dir(scl, 0);             // SCL = H
            p_this->xfer.gpio_xfer_sub_state++;
            break;                    
            
        case 3:

            if (venus_gpio_read(scl))               // Wait SCL = H
            {                                                                                
                if (p_this->xfer.rx_len < p_this->xfer.rx_buff_len)
                    p_this->xfer.gpio_xfer_state = G2C_ST_DATA0;    
                else
                    p_this->xfer.gpio_xfer_state = G2C_ST_STOP;     
                    
                p_this->xfer.gpio_xfer_sub_state = 0;
            }                            
            break; 
        }        
    }      
}    



/*------------------------------------------------------------------
 * Func : venus_g2c_do_write
 *
 * Desc : gpio i2c xfer - write data phase
 *
 * Parm : p_this : handle of venus i2c  
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_g2c_do_write(venus_i2c* p_this)
{
    unsigned long sda = p_this->gpio_map.sda;
    unsigned long scl = p_this->gpio_map.scl; 
    unsigned char state = G2C_MINOR_STATE(p_this->xfer.gpio_xfer_state);
    int bit_index = 0;
    
    if (state < 8)        
    {
        //--------------------------------------------------
        // DATA Phase 0 ~ 7
        //--------------------------------------------------
                    
        switch(p_this->xfer.gpio_xfer_sub_state)
        {
        case 0:
            venus_gpio_set_dir(scl, 1);             // SCL = L            
            p_this->xfer.gpio_xfer_sub_state++;
            break;            
            
        case 1:
            bit_index = 7 - state;   
            
            if ((p_this->xfer.tx_buff[p_this->xfer.tx_len]>>bit_index) & 0x1)
                venus_gpio_set_dir(sda, 0);         // SDA = H    
            else                
                venus_gpio_set_dir(sda, 1);         // SDA = L                        
            
            p_this->xfer.gpio_xfer_sub_state++;
            break;            
            
        case 2:
            bit_index = 7 - state;
            if (((p_this->xfer.tx_buff[p_this->xfer.tx_len]>>bit_index) & 0x1) && venus_gpio_read(sda)==0)
            {
                // lose of arbitraction 
                p_this->xfer.ret = -ETXABORT;
                p_this->xfer.gpio_xfer_state = G2C_ST_DONE;
                p_this->xfer.gpio_xfer_sub_state = 0;
            }                                
            else
            {                
                venus_gpio_set_dir(scl, 0);             // SCL = H
                p_this->xfer.gpio_xfer_sub_state++;
            }                
                            
            break;            
            
        case 3:
            if (venus_gpio_read(scl))               // Wait SCL = H
            {
                p_this->xfer.gpio_xfer_state++;     // Next State
                p_this->xfer.gpio_xfer_sub_state = 0;                                
            }
            break;
        }
    }                
    else
    {        
        //--------------------------------------------------
        // ACK & NACK
        //--------------------------------------------------
        
        switch(p_this->xfer.gpio_xfer_sub_state)
        {
        case 0: 
            venus_gpio_set_dir(scl, 1);             // Pull low SCL
            p_this->xfer.gpio_xfer_sub_state++;
            break;   
            
        case 1:            
            venus_gpio_set_dir(sda, 0);             // SDA = H
            p_this->xfer.gpio_xfer_sub_state++;
            break;        
            
        case 2:     
            venus_gpio_set_dir(scl, 0);             // Release SCL                         
            p_this->xfer.gpio_xfer_sub_state++;
            break;                    
            
        case 3:            
            if (venus_gpio_read(scl))               // Wait SCL = H
            {                    
                p_this->xfer.tx_len++;
                                                       
                if (venus_gpio_read(sda) || (p_this->xfer.tx_len >= p_this->xfer.tx_buff_len))
                {
                    if (venus_gpio_read(sda))
                        p_this->xfer.ret = -ETXABORT;    
                    p_this->xfer.gpio_xfer_state = G2C_ST_STOP;     // NACK or no data to xfer
                }
                else
                {                    
                    p_this->xfer.gpio_xfer_state = G2C_ST_DATA0;    // ACK and still has data to xfer
                }
                    
                p_this->xfer.gpio_xfer_sub_state = 0;                                
            }                            
            break; 
        }        
    }    
}   




/*------------------------------------------------------------------
 * Func : venus_g2c_do_stop
 *
 * Desc : Do STOP or Restart
 *
 * Parm : p_this : handle of venus i2c  
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_g2c_do_stop(venus_i2c* p_this)
{    
    unsigned long sda = p_this->gpio_map.sda;
    unsigned long scl = p_this->gpio_map.scl; 
        
    switch(p_this->xfer.gpio_xfer_sub_state)
    {
    case 0:            
        venus_gpio_set_dir(scl, 1);             // SCL = L        
        p_this->xfer.gpio_xfer_sub_state++;
        break; 
        
    case 1:            
        if ((p_this->xfer.flags & I2C_NO_STOP)==0 || p_this->xfer.ret < 0)
            venus_gpio_set_dir(sda, 1);         // SDA = L
        else
            venus_gpio_set_dir(sda, 0);         // SDA = H
        
        p_this->xfer.gpio_xfer_sub_state++;
        break; 
        
    case 2:            
        venus_gpio_set_dir(scl, 0);             // SCL = H
        p_this->xfer.gpio_xfer_sub_state++;
        break;
            
    case 3:
        if (venus_gpio_read(scl))               // wait SCL = H            
        {                                                
            venus_gpio_set_dir(sda, 0);         // SDA = H
            p_this->xfer.gpio_xfer_state = G2C_ST_DONE;
            p_this->xfer.gpio_xfer_sub_state = 0;            
        }                 
    }        
}




/*------------------------------------------------------------------
 * Func : venus_g2c_do_complete
 *
 * Desc : complete GPIO i2c transxfer
 *
 * Parm : p_this : handle of venus i2c  
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void venus_g2c_do_complete(venus_i2c* p_this)
{    
    unsigned long sda = p_this->gpio_map.sda;
    unsigned long scl = p_this->gpio_map.scl; 
    
    if (p_this->xfer.gpio_xfer_sub_state==0)
    {          
        venus_gpio_set_dir(sda, 0);            
        venus_gpio_set_dir(scl, 0);          
        venus_i2c_gpio_selection(p_this, I2C_MODE);
        p_this->xfer.gpio_xfer_sub_state++;     
        p_this->xfer.mode = I2C_IDEL;           
    }
}



/*------------------------------------------------------------------
 * Func : venus_g2c_isr
 *
 * Desc : isr of venus gpio i2c
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 
 *------------------------------------------------------------------*/
static 
irqreturn_t venus_g2c_isr(
    int                     this_irq, 
    void*                   dev_id, 
    struct pt_regs*         regs
    )
{            
    venus_i2c* p_this = (venus_i2c*) dev_id;        
    
    unsigned long flags;            
    
#if 0
    printk(KERN_DEBUG "ST = %d-%d:%d\n", 
        G2C_MAJOR_STATE(p_this->xfer.gpio_xfer_state),
        G2C_MINOR_STATE(p_this->xfer.gpio_xfer_state),
        p_this->xfer.gpio_xfer_sub_state);
#endif        
    
    LOCK_VENUS_I2C(&p_this->lock, flags);                                                                                                                   
    
#if 0    
    printk(KERN_DEBUG "p_this->xfer.mode=%d, jiffies = %lu,  timeout = %lu\n", 
            p_this->xfer.mode, 
            jiffies, 
            p_this->xfer.timeout);
#endif            
    
    if (p_this->xfer.mode != I2C_IDEL && time_after(jiffies,p_this->xfer.timeout))
    {                             
        p_this->xfer.ret = -ETIMEOUT;
        p_this->xfer.gpio_xfer_state = G2C_ST_DONE;
        p_this->xfer.gpio_xfer_sub_state = 0;
    }                                                                                 
    
    switch(G2C_MAJOR_STATE(p_this->xfer.gpio_xfer_state))
    {    
    case G2C_STATE_START: venus_g2c_do_start(p_this);      break;                
    case G2C_STATE_ADDR:  venus_g2c_do_address(p_this);    break; 
    case G2C_STATE_STOP:  venus_g2c_do_stop(p_this);       break;        
    case G2C_STATE_DONE:  venus_g2c_do_complete(p_this);   break;    
    case G2C_STATE_DATA:
        if (p_this->xfer.mode==I2C_MASTER_WRITE)
            venus_g2c_do_write(p_this);
        else            
            venus_g2c_do_read(p_this);
        break;                            
    }                
            
    UNLOCK_VENUS_I2C(&p_this->lock, flags);
    
    return IRQ_HANDLED;
}



/*------------------------------------------------------------------
 * Func : venus_g2c_start_xfer
 *
 * Desc : venus_g2c_start_xfer
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : N/A
 *
 * Note : this file using GPIO4/5 to out I2C protocol. where GP4 is SCLK
 *        GP5 is SDA 
 *------------------------------------------------------------------*/
int venus_g2c_start_xfer(
    venus_i2c*              p_this    
    )
{                  
    int d = p_this->tick>>2; 
    p_this->xfer.timeout = jiffies + (2 * HZ);
    p_this->xfer.gpio_wait_time = (G2C_WAIT_TIMEOUT * 1000);
    
    while(1)
    {
        venus_g2c_isr(7, (void*) p_this, 0);
        
        if (p_this->xfer.mode == I2C_IDEL)
            break;                
                    
        if (p_this->xfer.gpio_wait_time <= d)
        {
            // maximum run time = 3 msec                        
            p_this->xfer.gpio_wait_time = G2C_WAIT_TIMEOUT * 1000;
            msleep(1);
        }            
        else            
        {
            p_this->xfer.gpio_wait_time -= d;
            udelay(d);            
        }
    } 

#ifdef CONFIG_I2C_VENUS_BUS_JAM_RECOVER

    if (p_this->xfer.ret == -ETIMEOUT)
    {            
        if (venus_i2c_bus_jam_detect(p_this))
        {
            printk("[I2C] WARNING, I2C Bus Jammed, Do Recorver\n");        
            venus_i2c_bus_jam_recover_proc(p_this);
            msleep(50);
        }
        
        printk("[I2C] Info, Reset I2C State\n");        
        venus_i2c_reset_state(p_this);
    }

#endif

    return p_this->xfer.ret;
}



/*------------------------------------------------------------------
 * Func : venus_i2c_get_tx_abort_reason
 *
 * Desc : get reason of tx abort, this register will be clear when new message is loaded
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : tx about source
 *------------------------------------------------------------------*/    
unsigned int venus_i2c_get_tx_abort_reason(venus_i2c* p_this)
{               
    return p_this->xfer.tx_abort_source;    
}




/*------------------------------------------------------------------
 * Func : venus_i2c_load_message
 *
 * Desc : load a i2c message (just add this message to the queue)
 *
 * Parm : p_this : handle of venus i2c 
 *        
 *         
 * Retn : 0 for success, others is failed
 *------------------------------------------------------------------*/    
int venus_i2c_load_message(
    venus_i2c*              p_this,
    unsigned char           mode,
    unsigned char*          tx_buf, 
    unsigned short          tx_buf_len, 
    unsigned char*          rx_buf, 
    unsigned short          rx_buf_len,
    unsigned char           xfer_flags
    )
{           
    unsigned long flags;
    
    LOCK_VENUS_I2C(&p_this->lock, flags);     
    
    memset(&p_this->xfer, 0, sizeof(p_this->xfer));        
        
    p_this->xfer.mode           = mode;
    p_this->xfer.flags          = xfer_flags;
    p_this->xfer.tx_buff        = tx_buf;
    p_this->xfer.tx_buff_len    = tx_buf_len;
    p_this->xfer.tx_len         = 0;
    p_this->xfer.rx_buff        = rx_buf;
    p_this->xfer.rx_buff_len    = rx_buf_len;
    p_this->xfer.rx_len         = 0;
    
    if (rx_buf && rx_buf_len)
        memset(rx_buf, 0, rx_buf_len);
        
    p_this->xfer.gpio_xfer_state   = G2C_ST_START;
    p_this->xfer.gpio_xfer_sub_state = 0;    
    
    //reset timer
    init_timer(&p_this->xfer.timer);    
    p_this->xfer.timer.data     = (unsigned long) p_this;
    p_this->xfer.timer.expires  = jiffies + 4 * HZ;
    p_this->xfer.timer.function = venus_i2c_timer; 
    
    //reset timer
    init_completion(&p_this->xfer.complete);
    
    UNLOCK_VENUS_I2C(&p_this->lock, flags);     
    
    return 0;
}


/*------------------------------------------------------------------
 * Func : venus_i2c_read
 *
 * Desc : read data from sar
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 for success, others is failed
 *------------------------------------------------------------------*/
int venus_i2c_read(
    venus_i2c*              p_this, 
    unsigned char*          tx_buf, 
    unsigned short          tx_buf_len, 
    unsigned char*          rx_buf, 
    unsigned short          rx_buf_len
    )
{
    int retry = 2;
    int ret = 0; 
    
    while(retry > 0)
    {    
        venus_i2c_load_message(p_this, 
            (tx_buf_len) ? I2C_MASTER_RANDOM_READ : I2C_MASTER_READ,
            tx_buf, tx_buf_len, rx_buf, rx_buf_len, 0);
            
        ret = venus_i2c_start_xfer(p_this);                     
        if (ret!=-ETIMEOUT)
            break;        
            
        i2c_print("[I2C] read timeout detected, do retry\n");                
        retry--;  
    }
    
    return ret;
}    



/*------------------------------------------------------------------
 * Func : venus_i2c_write
 *
 * Desc : write data to sar
 *
 * Parm : p_this : handle of venus i2c 
 *        tx_buf : data to write
 *        tx_buf_len : number of bytes to write
 *        wait_stop  : wait for stop of not (extension)
 *         
 * Retn : 0 for success, others is failed
 *------------------------------------------------------------------*/
int venus_i2c_write(
    venus_i2c*              p_this, 
    unsigned char*          tx_buf, 
    unsigned short          tx_buf_len,
    unsigned char           wait_stop
    )
{
    int retry = 2;
    int ret = 0; 
    
    while(retry > 0)
    {  
        venus_i2c_load_message(p_this, I2C_MASTER_WRITE, 
            tx_buf, tx_buf_len, NULL, 0, (wait_stop) ? 0 : I2C_NO_STOP);                               
                               
#ifdef CONFIG_I2C_VENUS_NON_STOP_WRITE_XFER
    
        ret = (!wait_stop) ? venus_g2c_start_xfer(p_this)   // normal i2c can not support this mode, so we use GPIO mode to instead
                           : venus_i2c_start_xfer(p_this);
#else                                    
        ret = venus_i2c_start_xfer(p_this);
#endif        
      
        if (ret!=-ETIMEOUT)
            break;               
        
        i2c_print("[I2C] read timeout detected, do retry\n");                            
        retry--;                
    }
    
    return ret;    
}



/*------------------------------------------------------------------
 * Func : venus_g2c_write
 *
 * Desc : write data to sar - GPIO mode
 *
 * Parm : p_this : handle of venus i2c 
 *        tx_buf : data to write
 *        tx_buf_len : number of bytes to write
 *        wait_stop  : wait for stop of not (extension)
 *         
 * Retn : 0 for success, others is failed
 *------------------------------------------------------------------*/
int venus_g2c_write(
    venus_i2c*              p_this, 
    unsigned char*          tx_buf, 
    unsigned short          tx_buf_len,
    unsigned char           wait_stop
    )
{
    venus_i2c_load_message(p_this, I2C_MASTER_WRITE, 
                           tx_buf, tx_buf_len, NULL, 0, (wait_stop) ? 0 : I2C_NO_STOP);                               
                               
    return venus_g2c_start_xfer(p_this);           
}



/*------------------------------------------------------------------
 * Func : venus_g2c_read
 *
 * Desc : read data from sar - GPIO mode
 *
 * Parm : p_this : handle of venus i2c 
 *         
 * Retn : 0 for success, others is failed
 *------------------------------------------------------------------*/
int venus_g2c_read(
    venus_i2c*              p_this, 
    unsigned char*          tx_buf, 
    unsigned short          tx_buf_len, 
    unsigned char*          rx_buf, 
    unsigned short          rx_buf_len
    )
{
    int ret = 0;
    
    if (tx_buf && tx_buf_len)
    {        
        if ((ret = venus_g2c_write(p_this,  tx_buf, tx_buf_len, 0))<0)
            return ret;                
    }

    venus_i2c_load_message(p_this, I2C_MASTER_READ,
            NULL, 0, rx_buf, rx_buf_len, 0);

    return venus_g2c_start_xfer(p_this);
}    


static unsigned char venus_i2c_flags = 0;


/*------------------------------------------------------------------
 * Func : create_venus_i2c_handle
 *
 * Desc : create handle of venus i2c
 *
 * Parm : N/A
 *         
 * Retn : handle of venus i2c
 * 
 *------------------------------------------------------------------*/
venus_i2c*  
create_venus_i2c_handle(
    unsigned char       id,
    unsigned short      sar,
    ADDR_MODE           sar_mode,
    unsigned int        spd,
    unsigned int        irq
    )
{
    venus_i2c* hHandle;
    
    if (id >= MAX_I2C_CNT || (venus_i2c_flags>>id) & 0x01)
        return NULL;
        
    hHandle = kmalloc(sizeof(venus_i2c),GFP_KERNEL);        
    
    if (hHandle!= NULL)
    {
        hHandle->id           = id;
        hHandle->irq          = irq;
        hHandle->sar          = sar;               
        hHandle->sar_mode     = sar_mode;
        hHandle->spd          = spd;                
        hHandle->init         = venus_i2c_init;
        hHandle->uninit       = venus_i2c_uninit;
        hHandle->set_spd      = venus_i2c_set_spd;
        hHandle->set_tar      = venus_i2c_set_tar;
        hHandle->read         = venus_i2c_read;
        hHandle->write        = venus_i2c_write;                                           
        hHandle->gpio_read    = venus_g2c_read;
        hHandle->gpio_write   = venus_g2c_write;
        hHandle->dump         = venus_i2c_dump;      
        hHandle->suspend      = venus_i2c_suspend;        
        hHandle->resume       = venus_i2c_resume;  
        hHandle->get_tx_abort_reason = venus_i2c_get_tx_abort_reason;
        
        memset(&hHandle->xfer, 0, sizeof(venus_i2c_xfer));  
        
        venus_i2c_flags |= (0x01 << id);
    }
    
    return hHandle;
}   




/*------------------------------------------------------------------
 * Func : destroy_venus_i2c_handle
 *
 * Desc : destroy handle of venus i2c
 *
 * Parm : N/A
 *         
 * Retn : N/A 
 *------------------------------------------------------------------*/
void destroy_venus_i2c_handle(venus_i2c* hHandle)
{    
    if (hHandle==NULL)
        return;    
        
    hHandle->uninit(hHandle);
    venus_i2c_flags &= ~(0x01<<hHandle->id);    
    kfree(hHandle);             
}   

