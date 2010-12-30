#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include "scsi.h"
#include <scsi/scsi_host.h>
#include <linux/libata.h>
#include <asm/r4kcache.h>
#include <asm/r4kcache.h>
#include "scsi_priv.h"
#include "debug.h"
#include "sata_mars.h"
#include "sata_mars_cp.h"

#define DRV_NAME            "SATA_DRV"
#define DRV_VERSION         "0.7"
#define MARS_SATA_IRQ       2

#define MARS_SATA_REG_RANGE 0x100

static unsigned int PortStatus[SATA_PORT_COUNT] = {MARS_SATA_DEV_ONLINE, MARS_SATA_DEV_ONLINE};
static unsigned int mod_state=0;
static struct device *dev_addr=0;

static u8 mars_ata_check_err(struct ata_port *ap);
static u8 mars_ata_check_status(struct ata_port *ap);
static u8 mars_chk_online(struct ata_port *ap,u8 show);

static u32 mars_sata_scr_read (struct ata_port *ap, unsigned int sc_reg);

static int mars_init_sata (struct device *dev);
static int mars_remove(struct device *dev);
static int mars_sata_match(struct device *dev, struct device_driver *drv);
static int sata_offline_resume(struct device *dev);
static int mars_ATAPI_cmd(struct ata_port * ap,u8 atapi_op);

static void __mars_sata_phy_reset(struct ata_port *ap);
static void init_mdio (u8);
static void mars_sata_scr_write (struct ata_port *ap, unsigned int sc_reg, u32 val);
static void mars_ata_dma_reset(struct ata_port *ap);
static void mars_sata_phy_reset(struct ata_port *ap);
static void mdio_setting(u8, u16,u8);

static struct timer_list sata_timer ;
static struct work_struct mars_sata_work;

extern int ata_bus_probe(struct ata_port *ap);
extern int ata_qc_issue(struct ata_queued_cmd *qc);
extern unsigned int mars_ata_busy_sleep (struct ata_port *ap,unsigned long tmout_pat,unsigned long tmout);
extern void ata_qc_free(struct ata_queued_cmd *qc);
extern struct ata_queued_cmd *ata_qc_new_init(struct ata_port *ap,struct ata_device *dev);

struct bus_type sata_bus_type = {
    .name       = "SATA_BUS",
    .match      = mars_sata_match,

    .suspend    = generic_sata_suspend,
    //.resume     = generic_sata_resume,
    .resume     = sata_offline_resume,
};

void sata_sb1_release(struct device * dev)
{
    printk(KERN_INFO "/********** sata_module_release **********/\n");
    mod_state=0x00;
}

static struct device sata_sb1 = {
    .bus_id     = "SATA_DEV",
    .release    = sata_sb1_release,
    .bus        = &sata_bus_type,
};

static struct device_driver sata_driver = {
    .name           = DRV_NAME,
    .bus            = &sata_bus_type,
    .probe          = mars_init_sata,
    .remove         = mars_remove,
};

static Scsi_Host_Template mars_sht = {
    .module                 = THIS_MODULE,
    .name                   = DRV_NAME,
    .ioctl                  = ata_scsi_ioctl,
    .queuecommand           = ata_scsi_queuecmd,
    .eh_strategy_handler    = ata_scsi_error,
    .can_queue              = ATA_DEF_QUEUE,
    .this_id                = ATA_SHT_THIS_ID,
    .sg_tablesize           = ATA_MAX_PRD-1, /*mars limite is 255*/
    .max_sectors            = ATA_MAX_SECTORS,
    .cmd_per_lun            = ATA_SHT_CMD_PER_LUN,
    .emulated               = ATA_SHT_EMULATED,
    .use_clustering         = ATA_SHT_USE_CLUSTERING,
    .proc_name              = DRV_NAME,
    .dma_boundary           = ATA_DMA_BOUNDARY,
    .slave_configure        = ata_scsi_slave_config,
    .bios_param             = ata_std_bios_param,
    .ordered_flush          = 1,
};

static struct ata_port_operations mars_ops = {
    .port_disable   = ata_port_disable,
    .tf_load        = ata_tf_load,
    .tf_read        = ata_tf_read,
    .exec_command   = ata_exec_command,
    .dev_select     = ata_std_dev_select,
    .phy_reset      = mars_sata_phy_reset,
    .check_status   = mars_ata_check_status,
    .check_err      = mars_ata_check_err,
    .bmdma_setup    = mars_ata_dma_setup,
    .bmdma_start    = mars_ata_dma_start,
    .bmdma_stop     = mars_ata_dma_stop,
    .bmdma_status   = mars_ata_dma_status,
    .irq_clear      = mars_ata_dma_irq_clear,
    .qc_prep        = ata_qc_prep,
    .qc_issue       = ata_qc_issue_prot,
    .eng_timeout    = ata_eng_timeout,
    .irq_handler    = ata_interrupt,
    .scr_read       = mars_sata_scr_read,
    .scr_write      = mars_sata_scr_write,
    .port_start     = ata_port_start,
    .port_stop      = ata_port_stop,
    .host_stop      = ata_host_stop,
    .dma_reset      = mars_ata_dma_reset,
    .chk_online     = mars_chk_online,
    .exec_ATAPI_cmd = mars_ATAPI_cmd,
};

static struct ata_port_info mars_port_info = {
    .sht        = &mars_sht,
    .host_flags = ATA_FLAG_SATA | ATA_FLAG_SATA_RESET | ATA_FLAG_NO_LEGACY |
                  ATA_FLAG_MMIO | ATA_FLAG_PIO_DMA,
    .pio_mask   = 0x1f,
    .mwdma_mask = 0x7,
    .udma_mask  = 0x7f,
    .port_ops   = &mars_ops,
};


MODULE_AUTHOR("abevau");
MODULE_DESCRIPTION("host controller driver for Realtek mars-SATA controller");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);


void UR_INFO(unsigned int loop)
{
    unsigned int i;
    writel('{', (void __iomem *) (0xb801b200));
    for(i=0;i<loop;i++)
        writel('.', (void __iomem *) (0xb801b200));

    writel('}', (void __iomem *) (0xb801b200));
    writel('\n',(void __iomem *) (0xb801b200));
    writel('\r',(void __iomem *) (0xb801b200));

}

void UR_CHAR(unsigned char s){
    writel(s, (void __iomem *) (0xb801b200));
}

#define ur_char(s) writel('s', (void __iomem *) (0xb801b200));
int mars_qc_complete_noop(struct ata_queued_cmd *qc, u8 drv_stat);
static int mars_ATAPI_cmd(struct ata_port * ap,u8 atapi_op)
{
    struct ata_device *adev = &ap->device[0];
    struct ata_queued_cmd *qc;
    DECLARE_COMPLETION(wait);
    unsigned long flags;
    int rc;

    //printk("%s(%d)ap->active_tag:0x%x\n",__func__,__LINE__,ap->active_tag);
    if(ata_qc_from_tag(ap, ap->active_tag)){
        printk("%s(%d)ap active\n",__func__,__LINE__);
        return -1;
    }
    else
    {
        qc = ata_qc_new_init(ap, adev);
        if(qc == NULL){
            printk("%s(%d) get new qc fail\n",__func__,__LINE__);
            return -1;
        }
    }

    qc->complete_fn = mars_qc_complete_noop;
    qc->tf.command = ATA_CMD_PACKET;
    qc->tf.feature = 0;
    qc->tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE;
    qc->tf.protocol = ATA_PROT_ATAPI_NODATA;
    qc->tf.nsect = 0;
    qc->tf.device=(qc->tf.device&(~0x08))|(ap->port_no<<3);

    qc->waiting = &wait;

    memset(&qc->cdb, 0, ap->cdb_len);

    qc->cdb[0]=atapi_op;

    if(qc->cdb[0]==START_STOP)    //start_stop;
    {
        qc->cdb[1]=0x00;    //no imm bit
        qc->cdb[4]=0x02;    //eject disc
    }
    else if(qc->cdb[0]==ALLOW_MEDIUM_REMOVAL)   //prevent_allow
    {
        qc->cdb[4]=0x00;    //unlock
    }

    spin_lock_irqsave(&ap->host_set->lock, flags);
    rc = ata_qc_issue(qc);
    spin_unlock_irqrestore(&ap->host_set->lock, flags);

    if (rc){
        ata_qc_free(qc);
        return -1;
    }else{
        wait_for_completion(&wait);
    }
    return 0;
}


#ifdef CONFIG_MARS_PM
int generic_sata_suspend(struct device *dev, u32 state)
{
    struct ata_host_set *host_set = dev_get_drvdata(dev);
    struct ata_port *ap;

    //int err;
    int i;
    //int j;

    printk(KERN_INFO"generic_sata_suspend\n");

    del_timer(&sata_timer);

    for(i=0;i<host_set->n_ports;i++)
    {
        struct ata_device *adev;
        struct ata_queued_cmd *qc;
        unsigned long flags;
        int rc=0;
        unsigned int reginfo;
        DECLARE_COMPLETION(wait);

        ap = (struct ata_port *) host_set->ports[i];
        if (sata_dev_present(ap)){
            printk(KERN_WARNING"sata%u :device found on port%u; ATA status:0x%02x\n",
                                ap->id,ap->port_no,ata_chk_status(ap));
        }else{
            printk(KERN_WARNING"sata%u :no device found on port%u\n",
                                ap->id,ap->port_no);
            continue;
        }
        adev = &ap->device[0];
        if (ata_dev_present(adev))
        {
            reginfo=readl((void __iomem *) (SATA0_CDR7+ap->port_no*MARS_BANK_GAP))&0xff;
            printk(KERN_WARNING"sata%u :device found; ATA status : 0x%02x\n",ap->id,reginfo);
        }else{
            printk("%s(%d)\n",__func__,__LINE__);
            continue;
        }

        if (mars_ata_busy_sleep(ap, ATA_TMOUT_7, ATA_TMOUT_15)) {
            printk("%s(%d)\n",__func__,__LINE__);
            //ap->ops->port_disable(ap);
            break;
        }
        printk("%s(%d)\n",__func__,__LINE__);
        qc = ata_qc_new_init(ap, adev);
        BUG_ON(qc == NULL);
        qc->tf.command = ATA_CMD_STANDBYNOW1;

        qc->tf.feature = 0;
        qc->tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE;
        qc->tf.protocol = ATA_PROT_NODATA;
        qc->tf.nsect = 0;
        //qc->tf.device|=0x40;
        qc->tf.device=(qc->tf.device&(~0x08))|(ap->port_no<<3);

        qc->waiting = &wait;
        qc->complete_fn = mars_qc_complete_noop;

        spin_lock_irqsave(&ap->host_set->lock, flags);
        rc = ata_qc_issue(qc);
        spin_unlock_irqrestore(&ap->host_set->lock, flags);

        if (rc){
            printk("%s(%d)standby cmd fail\n",__func__,__LINE__);
            ata_qc_free(qc);
            continue;
        }else{
            printk("%s(%d)waiting finish\n",__func__,__LINE__);
            wait_for_completion(&wait);
            printk("%s(%d)standby cmd finish\n",__func__,__LINE__);
        }
        printk("%s(%d)\n",__func__,__LINE__);
        mdio_setting(SATA_PHY_RCR1, 0x129e,ap->port_no); //tune off SATA
    }
    printk("%s(%d)suspend finish\n",__func__,__LINE__);
    return 0;
}

int generic_sata_resume(struct device *dev)
{
    struct ata_host_set *host_set = dev_get_drvdata(dev);
    struct ata_port *ap;

    int i;

    printk(KERN_INFO"generic_sata_resume\n");

    for(i=0;i<host_set->n_ports;i++)
    {
        u32 sstatus;
        unsigned long timeout;
        //unsigned int reginfo;
        //struct ata_device *adev;

        ap = (struct ata_port *) host_set->ports[i];

        if(!ap)
            goto go_out;

        writel(TIMEOUT_VOL, (void __iomem *) (SATA0_TIMEOUT  +(ap->port_no*MARS_BANK_GAP)));
        writel(ENABLE_VOL, (void __iomem *) (SATA0_ENABLE +(ap->port_no*MARS_BANK_GAP)));

        //msleep(10);
        writel(SCR2_VOL|SCR2_ACT_COMMU, (void __iomem *) (SATA0_SCR2  +(ap->port_no*MARS_BANK_GAP)));
        init_mdio(ap->port_no);
        udelay(100);
        //writel(SCR2_VOL|SCR2_ACT_COMMU, (void __iomem *) (SATA0_SCR2  +(ap->port_no*MARS_BANK_GAP)));
        writel(SCR2_VOL, (void __iomem *) (SATA0_SCR2  +(ap->port_no*MARS_BANK_GAP)));

        timeout = jiffies + (HZ * 1);
        do {
            sstatus = scr_read(ap, SCR_STATUS);
            //printk(KERN_WARNING "sstatus:0x%x\n",sstatus);
            if ((sstatus & 0xf)==MARS_SATA_DEV_ONLINE || jiffies>timeout)
                break;
            msleep(200);
        } while (1);


        if(jiffies>timeout){
            printk(KERN_WARNING "sata%u: phy reset timeout!\n",ap->id);
            continue;
        } else {
            printk(KERN_WARNING "sata%u: phy is ready ",ap->id);
            printk(KERN_WARNING "COMRESET ok, SCRstatus = %X\n",sstatus);
        }

        if (mars_ata_busy_sleep(ap, ATA_TMOUT_BOOT_QUICK, ATA_TMOUT_BOOT)) {
            ap->ops->port_disable(ap);
        }

    }

 go_out:
    if(1){
        unsigned long j=jiffies;
        mod_timer(&sata_timer,j+2*HZ);
    }
    return 0;
}


#endif /*CONFIG_MARS_PM*/

int sata_offline_resume(struct device *dev)
{
    struct ata_host_set *host_set = dev_get_drvdata(dev);
    struct ata_port *ap;

    int i;

    printk(KERN_INFO"sata_offline_resume\n");
    for(i=0;i<host_set->n_ports;i++){
        host_set->ahost_hp_event &= ~(1<<i);
        ap = host_set->ports[i];
        PortStatus[i]=0x00;
        writel(TIMEOUT_VOL, (void __iomem *) (SATA0_TIMEOUT  +(ap->port_no*MARS_BANK_GAP)));
        writel(ENABLE_VOL, (void __iomem *) (SATA0_ENABLE +(ap->port_no*MARS_BANK_GAP)));
        //msleep(10);
        writel(SCR2_VOL|SCR2_ACT_COMMU, (void __iomem *) (SATA0_SCR2  +(ap->port_no*MARS_BANK_GAP)));
        init_mdio(ap->port_no);
        udelay(100);
        //writel(SCR2_VOL|SCR2_ACT_COMMU, (void __iomem *) (SATA0_SCR2  +(ap->port_no*MARS_BANK_GAP)));
        writel(SCR2_VOL, (void __iomem *) (SATA0_SCR2  +(ap->port_no*MARS_BANK_GAP)));
    }

    mod_timer(&sata_timer,jiffies+HZ/2);
    return 0;
}



/**
Tell if a marssata device structure has a matching mars sata device id structure
**/

static int mars_sata_match(struct device *dev, struct device_driver *drv)
{
    return 1;
}


static u32 mars_sata_scr_read (struct ata_port *ap, unsigned int sc_reg)
{
    marsinfo("mars_sata_scr_read\n");
    void __iomem *mmio = (void __iomem *) (ap->ioaddr.scr_addr + (sc_reg * 4));
    return readl(mmio);
}

static void mars_sata_scr_write (struct ata_port *ap, unsigned int sc_reg, u32 val)
{
    marsinfo("mars_sata_scr_write\n");
    void __iomem *mmio = (void __iomem *) (ap->ioaddr.scr_addr + (sc_reg * 4));
    writel(val, mmio);
}

static u8 mars_ata_check_status(struct ata_port *ap)
{
    u8 tf_status;
    marsinfo("mars_ata_check_status\n");
    void __iomem *mmio = (void __iomem *) ap->ioaddr.status_addr;
    tf_status=(u8)readl(mmio)&0xff;
    if(ap->flags & ATA_FLAG_HP_EVENT){
        printk("%s(%d)dev disconnect, return tf_status:0x51\n",__func__,__LINE__);
        printk("%s(%d)true tf_status:0x%x\n",__func__,__LINE__,tf_status);
        return 0x51;
    }
    return tf_status;
}

static u8 mars_ata_check_err(struct ata_port *ap)
{
    void __iomem *mmio = (void __iomem *) ap->ioaddr.error_addr;
    return (u8)readl(mmio)&0xff;
}

extern int MARS_CP_EN;
extern u32 CP_KEY[];
void mars_ata_dma_setup(struct ata_queued_cmd *qc)
{

    struct ata_port *ap = qc->ap;

    marsinfo("mars_ata_dma_setup\n");


#ifdef SHOW_PRD
    if(1){
        u16 i;
        u32 addr;
        struct scatterlist *sg = qc->sg;
        printk(KERN_WARNING"prd alloc, virt %p, dma %llx\n", ap->prd, (unsigned long long) ap->prd_dma);
        addr = (u32) sg_dma_address(sg);
        addr = cpu_to_le32(addr);
        printk(KERN_WARNING"n_elem=%d\n",qc->n_elem);

        for(i=0;i<qc->n_elem;i++){
            struct ata_prd *prdtmp;
            prdtmp=ap->prd+i;
            printk(KERN_WARNING"prdlist[%d]=addr:0x%08x len:0x%08x\n",i,prdtmp->addr,prdtmp->flags_len);
        }
    }

#endif

    if(MARS_CP_EN)
    {
        u32 reg_data;
        int port_gap=0;;

        marslolo("MARS_CP_EN A\n");
        marslolo("CP_KEY[0]=%x\n",CP_KEY[0]);
        marslolo("CP_KEY[1]=%x\n",CP_KEY[1]);

        writel(CP_KEY[0], (void __iomem *) (ACP_CP0_KEY0+0x10*ap->port_no));
        writel(CP_KEY[1], (void __iomem *) (ACP_CP0_KEY1+0x10*ap->port_no));

        reg_data=readl((void __iomem *) ACP_CTRL);
        if(ap->port_no==1)
        {
            port_gap=10;
        }
        reg_data=reg_data & (~(1<<(5+port_gap)) |(1<<port_gap)|(0x150<<port_gap));
        marslolo("cp reg-a=%x\n",reg_data);

        writel(reg_data, (void __iomem *) ACP_CTRL);
        marslolo("cp reg-b=%x\n",readl((void __iomem *) ACP_CTRL));

    }

    writel((u32)ap->prd_dma, (void __iomem *) (SATA0_PRDPTR+(ap->port_no * MARS_BANK_GAP )));
    ap->ops->exec_command(ap, &qc->tf);
}

void mars_ata_dma_start(struct ata_queued_cmd *qc)
{
    struct ata_port *ap = qc->ap;
    unsigned int rw = (qc->tf.flags & ATA_TFLAG_WRITE);
    u32 dmactl;

    marsinfo("mars_ata_dma_start\n");

    if(rw){
        writel((u32)0x01, (void __iomem *) (SATA0_DMACR+(ap->port_no * MARS_BANK_GAP )));   //alex add
    }else{
        writel((u32)0x02, (void __iomem *) (SATA0_DMACR+(ap->port_no * MARS_BANK_GAP )));   //alex add
    }

    //writel((u32)0x03, (void __iomem *) (SATA0_DMACR+(ap->port_no * MARS_BANK_GAP )));

#ifdef FORCE_SYNC
    mb();
    dma_cache_wback_inv((u32) ap->prd,ATA_PRD_TBL_SZ);
#endif

#ifdef CHG_BUF_LEN
    //printk(KERN_WARNING "sata%u: CHG_BUF_LEN\n",ap->id);
    void __iomem *mmio2 = (void __iomem *) (SATA0_DMAMISC+(ap->port_no * MARS_BANK_GAP));
    writel(readl(mmio2 ) & (~(0x0f<<12)|(0x0a<<12)), mmio2);
#endif

    void __iomem *mmio = (void __iomem *) (SATA0_DMACTRL+(ap->port_no * MARS_BANK_GAP));

#ifdef MARS_LOLO_DEBUG
    if(0){
        dmactl=readl(mmio);
        marslolo("chk DMACTRL:0x%x\n",dmactl);
        dmactl=0;
    }
#endif

    dmactl = (1<<21) | (0<<16) | ((qc->n_elem&0xff)<<8) | ((!rw<<3) | 0x01);

    if(MARS_CP_EN)
    {
        marslolo("MARS_CP_EN B\n");
        dmactl|=(1<<6);
    }
    marslolo("dmactl:0x%x\n",dmactl);

#ifdef FORCE_SYNC
    mb();
    dma_cache_wback_inv((u32) ap->prd,ATA_PRD_TBL_SZ);
#endif
    writel(dmactl, mmio);
}

u8 mars_ata_dma_status(struct ata_port *ap)
{
    marsinfo("mars_ata_dma_status\n");
    void __iomem *mmio = (void __iomem *) (SATA0_INTPR+(ap->port_no * MARS_BANK_GAP) );

    if(readl(mmio)&0x80000000)
        return ATA_DMA_INTR;

    return 0;

}

u8 mars_chk_online(struct ata_port *ap,u8 show)
{
    struct ata_device *adev;
    adev = &ap->device[0];
    if(adev->flags & ATA_DFLAG_RESET_ALERT)
        return 1;

    if(!sata_dev_present(ap)){
        udelay(100);
        if(!sata_dev_present(ap)){
            ap->flags |= ATA_FLAG_HP_EVENT;
            if(show==1)
                printk("port%d dev%d disconnect\n",ap->id,ap->port_no);
            return 0;
        }
    }
    return 1;
}


void mars_ata_dma_reset(struct ata_port *ap)
{
    unsigned int reginfo;
    int phy_bank=ap->port_no;
    unsigned int polltime;

    marsinfo("mars_ata_dma_reset\n");

    /*Mask SB1 sata*/
    asm ("sync");
    reginfo=readl((void __iomem *)SB1_MASK);

    polltime=0;
    writel(reginfo|(SB1_MASK_SATA0<<phy_bank),(void __iomem *)SB1_MASK);
    while(((readl((void __iomem *)SB1_MASK)>>16)&(SB1_MASK_SATA0<<phy_bank))
                !=(SB1_MASK_SATA0<<phy_bank))
    {
        udelay(100);
        if(polltime>50){  /*max waiting 5ms*/
            printk(KERN_WARNING "sata%u: SB1 check fail!\n",ap->id);
            break;
        }
        polltime++;
    }
    marslolo("polltime-c: %u\n",polltime);

    /* reset phy*/
    asm ("sync");
    reginfo=readl((void __iomem *)CRT_SOFT_RESET1);
    writel(reginfo&(~(SATA_RST_PHY0<<phy_bank)),(void __iomem *)CRT_SOFT_RESET1);

    /*reset SATA MAC clock*/
    asm ("sync");
    reginfo=readl((void __iomem *)CRT_CLOCK_ENABLE1);
    writel(reginfo&(~(SATA_PORT0_CLK_EN<<phy_bank)),(void __iomem *)CRT_CLOCK_ENABLE1);

    /*reset SATA MAC */
    asm ("sync");
    reginfo=readl((void __iomem *)CRT_SOFT_RESET1);
    writel(reginfo&(~(SATA_RST_MAC0<<phy_bank)),(void __iomem *)CRT_SOFT_RESET1);


    udelay(100);
    /*release SB1 Mask*/
    asm ("sync");
    reginfo=readl((void __iomem *)SB1_MASK);
    writel(reginfo&(~(SB1_MASK_SATA0<<phy_bank)),(void __iomem *)SB1_MASK);

    /*release SATA MAC*/
    asm ("sync");
    reginfo=readl((void __iomem *)CRT_SOFT_RESET1);
    writel(reginfo|(SATA_RST_MAC0<<phy_bank),(void __iomem *)CRT_SOFT_RESET1);

    /*release SATA MAC clock*/
    asm ("sync");
    reginfo=readl((void __iomem *)CRT_CLOCK_ENABLE1);
    writel(reginfo|(SATA_PORT0_CLK_EN<<phy_bank),(void __iomem *)CRT_CLOCK_ENABLE1);

    /*release SATA phy*/
    asm ("sync");
    reginfo=readl((void __iomem *)CRT_SOFT_RESET1);
    writel(reginfo|(SATA_RST_PHY0<<phy_bank),(void __iomem *)CRT_SOFT_RESET1);


    udelay(200);
    //printk("%s(%d)\n",__func__,__LINE__);
    asm ("sync");
    //writel(SCR2_VOL|SCR2_ACT_COMMU, (void __iomem *) (SATA0_SCR2  +(ap->port_no*MARS_BANK_GAP)));

    writel(SCR2_VOL,(void __iomem *) (SATA0_SCR2+phy_bank*MARS_BANK_GAP));
    asm ("sync");
    init_mdio(phy_bank);
    udelay(100);
    //writel(SCR2_VOL,(void __iomem *) (SATA0_SCR2+phy_bank*MARS_BANK_GAP));
    writel(SCR2_VOL|SCR2_ACT_COMMU, (void __iomem *) (SATA0_SCR2  +(ap->port_no*MARS_BANK_GAP)));
    udelay(100);
    writel(SCR2_VOL,(void __iomem *) (SATA0_SCR2+phy_bank*MARS_BANK_GAP));
}

void mars_ata_dma_stop(struct ata_queued_cmd *qc)
{

    struct ata_port *ap = qc->ap;
    void __iomem *mmio;
    unsigned int polltime;
    unsigned int reginfo;
    u8 drv_stat;

    marsinfo("sata%u: mars_ata_dma_stop\n",ap->id);

    drv_stat = ata_chk_status(ap);
    if(drv_stat & 0x81){
        printk(KERN_ERR "%s(%d) tf_stat:0x%x;tf_error:0x%x\n",__func__,__LINE__,drv_stat,ap->ops->check_err(ap));
    }


    writel((u32)0x00, (void __iomem *) (SATA0_DMACR+(ap->port_no * MARS_BANK_GAP )));   //alex add
    /*clear DMACTRL go bit */
    mmio = (void __iomem *)(SATA0_DMACTRL+(ap->port_no * MARS_BANK_GAP));

    if((readl(mmio)&0x08)==0){  //read memory(write data to device)

#ifdef POLLING_IPF
        if(1){
            void __iomem *mmio2;
            mmio2 = (void __iomem *)(SATA0_INTPR+(ap->port_no * MARS_BANK_GAP));
            polltime=0;
            //printk(KERN_WARNING "sata%u: INTPR-a:0x%08x\n",ap->id,readl(mmio2));
            while((readl(mmio2)|0x80000000)==0){
                mdelay(1);
                if(polltime>10){
                    printk(KERN_WARNING "sata%u: DMA write to HD error!\n",ap->id);
                    printk(KERN_WARNING "sata%u: INTPR:0x%08x\n",ap->id,readl(mmio2));
                    break;
                }
                polltime++;
            }
        }
#endif
        writel(0x00, mmio);
    }else{                      //write memory(read data from device)

        polltime=0;
        while((readl(mmio)&0x01)){
            mdelay(1);
            if(polltime>500){

                reginfo=readl((void __iomem *)(SATA0_INTPR+(ap->port_no * MARS_BANK_GAP)));
                /*
                printk(KERN_INFO "%s(%d) sata%u: INTPR:0x%08x\n",__func__,__LINE__,ap->id,reginfo);
                printk(KERN_INFO "DMAT=%d; NEWFP=%d; PMABORT=%d; ERR=%d; NEWBIST=%d; IPF=%d\n"
                                 ,(reginfo&0x01)
                                 ,(reginfo&0x02)>>1
                                 ,(reginfo&0x04)>>2
                                 ,(reginfo&0x08)>>3
                                 ,(reginfo&0x10)>>4
                                 ,(reginfo&0x80000000)>>31);
                */
                writel(0x00, mmio);
                printk(KERN_WARNING "sata%u: DMA read from device error !\n",ap->id);

#ifdef SHOW_IPF
        void __iomem *mmio3;
        mmio3 = (void __iomem *)(SATA0_INTPR+(ap->port_no * MARS_BANK_GAP));
        printk(KERN_WARNING "sata%u: INTPR-b:0x%08x\n",ap->id,readl(mmio3));
#endif
                break;
            }
            polltime++;
        }

    }
    writel(0x00, (void __iomem *) (SATA0_DMACR+(ap->port_no * MARS_BANK_GAP )));

    ata_altstatus(ap);
}

void mars_ata_dma_irq_clear(struct ata_port *ap)
{
    void __iomem *mmio;

    marsinfo("mars_ata_dma_irq_clear\n");

    /*clear interrupt */
    mmio = (void __iomem *)(SATA0_CDR7+(ap->port_no * MARS_BANK_GAP));
    readl(mmio);
    mmio = (void __iomem *)(SATA0_INTPR+(ap->port_no * MARS_BANK_GAP));
    readl(mmio);
    writel(readl(mmio), mmio);


}

static void mars_ata_std_ports(struct ata_ioports *ioaddr)
{
    marsinfo("mars_ata_std_ports\n");

    ioaddr->data_addr       = ioaddr->cmd_addr + (ATA_REG_DATA<<2);
    ioaddr->error_addr      = ioaddr->cmd_addr + (ATA_REG_ERR<<2);
    ioaddr->feature_addr    = ioaddr->cmd_addr + (ATA_REG_FEATURE<<2);
    ioaddr->nsect_addr      = ioaddr->cmd_addr + (ATA_REG_NSECT<<2);
    ioaddr->lbal_addr       = ioaddr->cmd_addr + (ATA_REG_LBAL<<2);
    ioaddr->lbam_addr       = ioaddr->cmd_addr + (ATA_REG_LBAM<<2);
    ioaddr->lbah_addr       = ioaddr->cmd_addr + (ATA_REG_LBAH<<2);
    ioaddr->device_addr     = ioaddr->cmd_addr + (ATA_REG_DEVICE<<2);
    ioaddr->status_addr     = ioaddr->cmd_addr + (ATA_REG_STATUS<<2);
    ioaddr->command_addr    = ioaddr->cmd_addr + (ATA_REG_CMD<<2);
}



static void mdio_setting(u8 address, u16 data,u8 bank)
{
    udelay(50);
    void __iomem *mmio = (void __iomem *) SATA0_MDIOCTRL+(bank*MARS_BANK_GAP);
    writel((data<<16) | (address<<8) | 0x01, mmio);
    //while(!(readl((void __iomem *) SATA0_MDIOCTRL)&0x00000010));
    udelay(50);
}

/*
static u16 mdio_getting(u16 address,u8 bank)
{
    marsinfo("mdio_getting\n");
    void __iomem *mmio = (void __iomem *) (SATA0_MDIOCTRL+(bank*MARS_BANK_GAP));
    writel(address<<8, mmio);
    //while(!(readl((void __iomem *) SATA0_MDIOCTRL)&0x00000010));
    udelay(50);
    return (u16)(readl(mmio)>>16);

}
*/
static void init_mdio (u8 bank)
{

    mdio_setting(SATA_PHY_PCR, 0x3088,bank);

    mdio_setting(SATA_PHY_RCR0, 0x7c65,bank);

    mdio_setting(SATA_PHY_RCR2, 0xc2b8,bank);

        mdio_setting(SATA_PHY_TCR0, 0xa23c,bank);

    mdio_setting(SATA_PHY_TCR1, 0x2800,bank);

    mdio_setting(SATA_PHY_BPCR, 0x6d00,bank);

    mdio_setting(0x1e, 0x2572,bank);

    mdio_setting(0x1f, 0x4186,bank);

    mdio_setting(SATA_PHY_RCR1, 0x12de,bank);

    mdio_setting(SATA_PHY_RCR0, 0x3c65,bank);
    udelay(50);
    mdio_setting(SATA_PHY_RCR0, 0x7c65,bank);

}

/**
 *  __sata_phy_reset - Wake/reset a low-level SATA PHY
 *  @ap: SATA port associated with target SATA PHY.
 *
 *  This function issues commands to standard SATA Sxxx
 *  PHY registers, to wake up the phy (and device), and
 *  clear any reset condition.
 *
 *  LOCKING:
 *  PCI/etc. bus probe sem.
 *
 */
void __mars_sata_phy_reset(struct ata_port *ap)
{
    u32 sstatus;
    unsigned int phy_bank=ap->port_no;
    unsigned long timeout;
    unsigned long rtyconut=0;
    //struct Scsi_Host *sh;

    marsinfo("__mars_sata_phy_reset\n");

    ap->host->bus_type[0] = 's';
    ap->host->bus_type[1] = 'a';
    ap->host->bus_type[2] = 't';
    ap->host->bus_type[3] = 'a';

    ap->host->port_structure[0]=0x31+ap->port_no;

    if (ap->flags & ATA_FLAG_SATA_RESET) {
extra_try:
        printk("sata dma reset\n");
        ap->ops->dma_reset(ap);
    }
    else
    {
hd_rty:
        printk("sata phy reset\n");
        writel(SCR2_VOL|SCR2_ACT_COMMU, (void __iomem *) (SATA0_SCR2  +(phy_bank*MARS_BANK_GAP)));
        msleep(200);
        writel(SCR2_VOL, (void __iomem *) (SATA0_SCR2  +(phy_bank*MARS_BANK_GAP)));
        msleep(200);
    }

    writel(TIMEOUT_VOL, (void __iomem *) (SATA0_TIMEOUT  +(phy_bank*MARS_BANK_GAP)));
    writel(ENABLE_VOL, (void __iomem *) (SATA0_ENABLE+(phy_bank*MARS_BANK_GAP)));


    /* waiting phy become ready, if necessary */
    marslolo("wait phy ready\n");
    timeout = jiffies + (HZ * (1+(rtyconut&0x0f)));
    do {
        sstatus = scr_read(ap, SCR_STATUS);
        if ((sstatus & 0xf)==MARS_SATA_DEV_ONLINE || time_after(jiffies, timeout))
            break;
        msleep(200);
    } while (1);


    if(jiffies>timeout)
    {
        if((rtyconut&0xf)<2)
    	{
            rtyconut++;
            printk(KERN_WARNING "sata%u: reset phy again!\n",ap->id);
            goto hd_rty;
        }
        if(ap->id==1 && (rtyconut&0xf)<5)
        {
            rtyconut++;
            printk(KERN_WARNING "sata%u: reset phy extra!\n",ap->id);
            goto hd_rty;
        }
        printk(KERN_WARNING "sata%u: phy reset timeout!\n",ap->id);
        sstatus = scr_read(ap, SCR_STATUS);
        printk(KERN_WARNING "sata%u: no device found (phy stat %08x)\n",
                ap->id, sstatus);

        ap->ops->port_disable(ap);
        PortStatus[phy_bank] &= ~MARS_SATA_DEV_ONLINE;
    }
    else
    {
        marslolo("phy is ready\n");
        printk(KERN_WARNING "sata%u: occupy port%d, COMRESET ok, sstatus = %X, SCRstatus = %x\n",
                ap->id,ap->port_no,sstatus, scr_read(ap, SCR_STATUS));
        ata_port_probe(ap);
    }

    if (ap->flags & ATA_FLAG_PORT_DISABLED)
        return;

reset_taskfile:
    /*check taskfile status*/
    printk("%s(%d)check taskfile state\n",__func__,__LINE__);
    if (mars_ata_busy_sleep(ap, ATA_TMOUT_BOOT_QUICK, ATA_TMOUT_BOOT)) {

        if(ata_chk_status(ap) & 0x01){
            printk("%s(%d)alert SRST\n",__func__,__LINE__);
            writel(0x0e, (void __iomem *) (SATA0_CLR0  +(phy_bank*MARS_BANK_GAP)));
            mdelay(200);
            printk("%s(%d)clear SRST \n",__func__,__LINE__);
            writel(0x00, (void __iomem *) (SATA0_CLR0  +(phy_bank*MARS_BANK_GAP)));
        }

        rtyconut=(rtyconut+0x10)&0xf0;
        if(rtyconut<0x30)
        {
            //printk(KERN_WARNING "sata%u: HDisk status not ready,reset phy again.\n",ap->id);
            //goto hd_rty;
            printk(KERN_WARNING "sata%u: HDisk status not ready.\n",ap->id);
            goto reset_taskfile;
        }
        if(ap->id==1 && rtyconut<0x50)
        {
            printk(KERN_WARNING "sata%u:HDisk status not ready! reset port%u phy extra!\n",
                                ap->id,ap->port_no);
            goto extra_try;
        }
        ap->ops->port_disable(ap);
        //printk(KERN_WARNING "sata%u:detect fail!\n",__func__,__LINE__);
        return;
    }
    ap->cbl = ATA_CBL_SATA;

}

/**
 *  sata_phy_reset - Reset SATA bus.
 *  @ap: SATA port associated with target SATA PHY.
 *
 *  This function resets the SATA bus, and then probes
 *  the bus for devices.
 *
 *  LOCKING:
 *  PCI/etc. bus probe sem.
 *
 */
void mars_sata_phy_reset(struct ata_port *ap)
{
    marsinfo("mars_sata_phy_reset\n");
    __mars_sata_phy_reset(ap);
    if (ap->flags & ATA_FLAG_PORT_DISABLED)
        return;
    ata_bus_reset(ap);
    ap->flags&=~ATA_FLAG_SATA_RESET;

}

static
void scan_sata_timer_fn(unsigned long data)
{
    struct device *dev = (struct device *) data;
    struct ata_host_set *host_set = dev_get_drvdata(dev);
    struct ata_port *ap;
    struct ata_device *adev;

    unsigned long j=jiffies;
    unsigned int reginfo1;
    unsigned int reginfo2;
    unsigned int i;
    //unsigned long flags;

    if (host_set->ahost_hp_event)
        goto end_proc;

    for(i=0; i<host_set->n_ports; i++)
    {
        ap = host_set->ports[i];
        adev = &ap->device[0];

        reginfo1=readl((void __iomem *)(SATA0_SCR0+i*MARS_BANK_GAP));
        reginfo2=readl((void __iomem *)(SATA0_SCR1+i*MARS_BANK_GAP));

        if ((reginfo1&0xf) != PortStatus[i] )
        {
            host_set->ahost_hp_event |= (1<<i);

            PortStatus[i] = reginfo1&0xf;
            adev->adev_hp_state=ADEV_HP_START;
        }
        else if(reginfo2&SCR1_ERR_C)
        {
            PortStatus[i] = reginfo1&0xf;
            printk(KERN_WARNING "sata port%u: unrecovered communication error\n ",i);
            writel(SCR2_VOL|SCR2_ACT_COMMU, (void __iomem *) (SATA0_SCR2  +(i*MARS_BANK_GAP)));
            writel(SCR2_VOL, (void __iomem *) (SATA0_SCR2  +(i*MARS_BANK_GAP)));
        }
        //printk(KERN_INFO "sata port%u: SCR0=0x%08x, SCR1=0x%08x\n ",i,reginfo1,reginfo2);
    }

    if (host_set->ahost_hp_event){
        schedule_work(&mars_sata_work);
    }

end_proc:
    mod_timer(&sata_timer,j+HZ/2);
}

static
void mars_sata_work_fn(void* ptr)
{
    int i;
    struct device *dev = (struct device *) ptr;
    struct ata_host_set *host_set = dev_get_drvdata(dev);
    struct ata_port *ap;
    struct ata_device *adev;
    //struct scsi_device *sdev;
    struct scsi_cmnd *scmd;
    unsigned long flags;
    struct ata_queued_cmd *qc;

    down_interruptible(&host_set->sata_sem);
    printk(KERN_INFO "mars_sata_work_fn\n");

    for(i=0; i<host_set->n_ports; i++)
    {
        struct scsi_device *sdev;

        if (!((host_set->ahost_hp_event >> i) & 0x01))   // not changed
            continue;

        ap = host_set->ports[i];
        adev = &ap->device[0];
        qc = ata_qc_from_tag(ap, ap->active_tag);

        if(!ap)
            BUG_ON(ap == NULL);

        switch(adev->adev_hp_state)
        {
            printk("%s(%d): adev_hp_state:%u\n",__func__,__LINE__,adev->adev_hp_state);
        case ADEV_HP_IDLE:
            break;

        case ADEV_HP_START:

            if (PortStatus[i] == MARS_SATA_DEV_ONLINE)
            {   /*plugin event ****/
                if(adev->flags & ATA_DFLAG_DEV_ONLINE){
                    printk("%s(%d)host is exist\n",__func__,__LINE__);
                    goto finish_out;
                }

                adev->flags |= ATA_DFLAG_DEV_PLUGING;
                //ata_port_probe(ap);
                ap->flags &= ~ATA_FLAG_HP_EVENT;

                printk(KERN_WARNING"device plug into sata port%u\n",ap->port_no);

                //ap->ops->dma_reset(ap);
                //msleep(3000);

                /*
                if (mars_ata_busy_sleep(ap, ATA_TMOUT_3, ATA_TMOUT_5)) {
                	printk(KERN_WARNING"port%u first detect fail\n",ap->port_no);
                    ap->ops->dma_reset(ap);
                }
                */

                if(ata_bus_probe(ap)){
                    goto finish_out;
                }

                scsi_scan_host(ap->host);
                adev->flags |= ATA_DFLAG_DEV_ONLINE;
                adev->adev_hp_state=ADEV_HP_END;
                goto finish_out;
            }   /*plugin event &&&*/
            else
            {   /*plugout event ****/

                adev->flags |= ATA_DFLAG_DEV_PLUGING;
                printk(KERN_WARNING"device removed from sata port%u\n",ap->port_no);
                if(qc){
                    printk("%s(%d): cmd Q persisting!!\n",__func__,__LINE__);
                    spin_lock_irqsave(&ap->host_set->lock, flags);
                    qc = ata_qc_from_tag(ap, ap->active_tag);
                    if(!qc){
                        printk("%s(%d) go back non qc path\n",__func__,__LINE__);
                        spin_unlock_irqrestore(&ap->host_set->lock, flags);
                        goto non_qc_path;
                    }
                    if( (qc->tf.protocol ==ATA_PROT_DMA) || (qc->tf.protocol==ATA_PROT_ATAPI_DMA)){
                        printk("%s(%d) DMA cmd\n",__func__,__LINE__);
                        scmd=ap->qcmd->scsicmd;
                        if (scmd->eh_timeout.function){
                            printk("%s(%d)eh_timeout.function exist\n",__func__,__LINE__);
                            mod_timer(&scmd->eh_timeout,jiffies);
                        }else{
                            printk("%s(%d)eh_timeout.function disapper!!\n",__func__,__LINE__);
                        }
                        ap->ops->dma_reset(ap);
                    }else if((qc->tf.protocol==ATA_PROT_PIO) ||(qc->tf.protocol==ATA_PROT_ATAPI)){
                        printk("%s(%d): PIO cmd \n",__func__,__LINE__);
                    }else{
                        printk("%s(%d): Non DATA cmd \n",__func__,__LINE__);
                    }
                    adev->adev_hp_state=ADEV_HP_WAIT_Q;

                    //spin_lock_irqsave(&ap->host_set->lock, flags);
                    list_for_each_entry(sdev, &ap->host->__devices, siblings)
                        scsi_device_set_state(sdev, SDEV_OFFLINE);
                    //spin_unlock_irqrestore(&ap->host_set->lock, flags);

                    schedule_delayed_work(&mars_sata_work,HZ>>6);
                    spin_unlock_irqrestore(&ap->host_set->lock, flags);
                    break;
                }else{
non_qc_path:
                    printk("%s(%d): no cmd Q\n",__func__,__LINE__);
                    spin_lock_irqsave(&ap->host_set->lock, flags);
                    list_for_each_entry(sdev, &ap->host->__devices, siblings)
                        scsi_device_set_state(sdev, SDEV_OFFLINE);
                    spin_unlock_irqrestore(&ap->host_set->lock, flags);
                    adev->adev_hp_state=ADEV_HP_END;
                    scsi_forget_host(ap->host);
                    adev->flags &= ~ATA_DFLAG_DEV_ONLINE;
                    ap->ops->port_disable(ap);
                    goto finish_out;
                }

            }   /*plugout event &&&*/


        case ADEV_HP_WAIT_Q:

            if(qc){
                schedule_delayed_work(&mars_sata_work,HZ>>6);
                printk("%s(%d) wait Q again..\n",__func__,__LINE__);
                break;
            }else{
                scsi_forget_host(ap->host);
                adev->flags &= ~ATA_DFLAG_DEV_ONLINE;
                ap->ops->port_disable(ap);
                writel(0x00, (void __iomem *) (SATA0_ENABLE+(ap->port_no*MARS_BANK_GAP)));
                printk("%s(%d) finish Q, \n",__func__,__LINE__);
            }

        case ADEV_HP_END:
finish_out:
            printk("%s(%d) hot plug end\n",__func__,__LINE__);
            ap->flags &=~ATA_FLAG_HP_EVENT; 
            host_set->ahost_hp_event &= ~(1<<i); // clear flags
            ap->device->flags &= ~ATA_DFLAG_DEV_PLUGING;
            adev->adev_hp_state=ADEV_HP_IDLE;
            break;

        default:
            printk("%s(%d) wrong atate flow\n",__func__,__LINE__);
        }   //&&switch(adev->adev_hp_state)&&

    }   //&&for(i=0; i<host_set->n_ports; i++)&&
    up(&host_set->sata_sem);
}

struct ata_probe_ent * ata_probe_ent_alloc(struct device *dev, struct ata_port_info *port);
static int mars_init_sata (struct device *dev)
{
    unsigned int i;
    unsigned int reginfo;
    unsigned int phy_bank_cnt=SATA_PORT_COUNT;
    struct ata_probe_ent *probe_ent;

    marsinfo("mars_init_sata\n");

    probe_ent = (struct ata_probe_ent *)ata_probe_ent_alloc(dev, &mars_port_info);

    if (!probe_ent){
        printk(KERN_ERR "SATA memory request fail.\n");
        return -ENOMEM;
    }

    /* if you want to use only one port, the port 0 must be used.*/
    reginfo=readl((void __iomem *)CRT_CLOCK_ENABLE1);
    if((reginfo&SATA_PORT0_CLK_EN)==0){
        kfree(probe_ent);
        printk(KERN_ERR "SATA phy clock are disabled.\n");
        //goto err_out;
        return -ENODEV;
    }

    /* check sata clock for one port enable project.*/
    if((reginfo&SATA_PORT1_CLK_EN)==0)
        phy_bank_cnt=1;

    probe_ent->n_ports      = phy_bank_cnt;
    probe_ent->irq          = 2 + MARS_SATA_IRQ;
    probe_ent->irq_flags    = SA_SHIRQ;

    if (!request_mem_region(MARS_SATA0_BASE, MARS_SATA_REG_RANGE*2, DRV_NAME))
    {
        kfree(probe_ent);
        printk(KERN_ERR "SATA I/F resource already be used.\n");
        //goto err_out;
        return -EBUSY;
    }

    for(i=0; i < probe_ent->n_ports;i++)
    {
        probe_ent->port[i].cmd_addr         = (unsigned long) SATA0_CDR0+(i*MARS_BANK_GAP);
        probe_ent->port[i].altstatus_addr   = (unsigned long) SATA0_CLR0+(i*MARS_BANK_GAP);
        probe_ent->port[i].ctl_addr         = (unsigned long) SATA0_CLR0+(i*MARS_BANK_GAP);
        probe_ent->port[i].scr_addr         = (unsigned long) SATA0_SCR0+(i*MARS_BANK_GAP);

        mars_ata_std_ports(&probe_ent->port[i]);
    }


    if(1)
    {
        unsigned long j=jiffies;

        init_timer(&sata_timer);
        sata_timer.data=(unsigned long)dev;

        sata_timer.expires=j+2*HZ;
        sata_timer.function=(void (*)(unsigned long))scan_sata_timer_fn;
        //add_timer(&sata_timer);

        INIT_WORK(&mars_sata_work, mars_sata_work_fn, dev);
        dev_addr=dev;
    }

    printk("%s(%d)dev_addr:%p\n",__func__,__LINE__,dev_addr);
    if(!ata_device_add(probe_ent)){
        kfree(probe_ent);
        printk(KERN_ERR "SATA probe fail.\n");
        return -ENODEV;
    }
    add_timer(&sata_timer);
    kfree(probe_ent);
    return 0;
}

static int mars_remove(struct device *dev)

{
    struct ata_host_set *host_set = dev_get_drvdata(dev);
    struct ata_port *ap;
    u32 i;

    printk(KERN_INFO"%s(%d)\n",__func__,__LINE__);

    del_timer(&sata_timer);
    cancel_delayed_work(&mars_sata_work);
    flush_scheduled_work();

    release_mem_region(MARS_SATA0_BASE, MARS_SATA_REG_RANGE*2);

    free_irq(host_set->irq, host_set);

    for(i=0; i<host_set->n_ports; i++){
        ap = host_set->ports[i];
        ata_scsi_release(ap->host);
        scsi_remove_host(ap->host);
        scsi_host_put(ap->host);
    }

    if(host_set->ops->host_stop)
        host_set->ops->host_stop(host_set);

    kfree(host_set);

    dev_set_drvdata(dev, NULL);
    return 0;
}

static ssize_t
show_offline_dev_field(struct bus_type *bus, char *buf)
{
    unsigned int i;
    struct ata_host_set *host_set = dev_get_drvdata(dev_addr);
    struct ata_port *ap;
    struct ata_device *adev;

    del_timer(&sata_timer);
    flush_scheduled_work();

    printk(KERN_ERR "%s(%d)dev_addr=%p;\n",__func__,__LINE__,dev_addr);

    for(i=0; i<host_set->n_ports; i++){
        host_set->ahost_hp_event|=(0x01<<i);
        ap = host_set->ports[i];
        adev=&ap->device[0];
        if (ata_dev_present(adev)){
            printk(KERN_WARNING"sata%u :device%u found\n",ap->id,ap->port_no);
        }else{
            printk(KERN_WARNING"sata%u :device%u not found\n",ap->id,ap->port_no);
        }
        PortStatus[i]=0;
        adev=&ap->device[0];
        adev->adev_hp_state=ADEV_HP_START;
    }
    mars_sata_work_fn(dev_addr);
    return sprintf(buf, "sata device have removed.\n");
}

static ssize_t
store_offline_dev_field(struct bus_type *bus, const char *buf, size_t count)
{
    struct ata_host_set *host_set = dev_get_drvdata(dev_addr);
    struct ata_port *ap;
    struct ata_device *adev;
    unsigned int off_port_no;

    sscanf (buf, "%d\n", &off_port_no);
    if(off_port_no==1||off_port_no==2)
        printk("port %d will be stop!\n",off_port_no);
    else{
        printk("port %d not exist!\n",off_port_no);
        return count;
    }

    del_timer(&sata_timer);
    flush_scheduled_work();

    off_port_no--;
    host_set->ahost_hp_event|=(0x01<<off_port_no);
    ap = host_set->ports[off_port_no];
    adev=&ap->device[0];

    if(adev->flags &ATA_DFLAG_DEV_ONLINE){

        if (ata_dev_present(adev)){
            printk(KERN_WARNING"sata%u :device%u found\n",ap->id,ap->port_no);
        }else{
            printk(KERN_WARNING"sata%u :device%u not found\n",ap->id,ap->port_no);
        }
        PortStatus[off_port_no]=0;
        adev=&ap->device[0];
        adev->adev_hp_state=ADEV_HP_START;

        mars_sata_work_fn(dev_addr);
        PortStatus[off_port_no]=MARS_SATA_DEV_ONLINE;
    }else{
        printk("port %d is offline!\n",off_port_no+1);
    }
    mod_timer(&sata_timer,jiffies+HZ/2);

    return count;
}
static BUS_ATTR(offline_dev, S_IRUGO | S_IWUSR, show_offline_dev_field, store_offline_dev_field);

static ssize_t
show_scan_dev_field(struct bus_type *bus, char *buf)
{
    printk(KERN_INFO"show_scan_dev_field\n");
    sata_offline_resume(dev_addr);
    return sprintf(buf, "sata device is scaning...\n");
}
static BUS_ATTR(scan_dev, S_IRUGO | S_IWUSR, show_scan_dev_field, NULL);

static ssize_t
show_wait_insmod_state_field(struct bus_type *bus, char *buf)
{
    unsigned int loop=60;
    while(mod_state==0 && loop )
    {
        printk("sata modules is not ready yet!!!\n");
        loop--;
        msleep(500);
    }
    if(mod_state)
        printk("sata module is ready for service.\n");
    else
        printk("sata module insmod fail !!!\n");

    return sprintf(buf, "%x\n",mod_state);
}
static BUS_ATTR(wait_insmod_state, S_IRUGO | S_IWUSR, show_wait_insmod_state_field, NULL);

u8 tray_state=0xff;
u8 tray_port=0xff;
static ssize_t
show_trayout_imm_field(struct bus_type *bus, char *buf)
{
    struct ata_host_set *host_set = dev_get_drvdata(dev_addr);
    struct ata_port *ap;
    unsigned int port_no;
    //struct ata_device *adev;

    if(tray_port>1){
        printk("please echo port first!\n");
        goto out_state;
    }

    ap = host_set->ports[tray_port];

    if((ap->flags & ATA_FLAG_EJT_ING)||(ap->flags&ATA_FLAG_PRE_EJT))
    {
        return sprintf(buf, "%x\n",tray_state);
    }

 out_state:
    tray_state=0xff;
    return sprintf(buf, "%x\n",tray_state);
}

static ssize_t
store_trayout_imm_field(struct bus_type *bus, const char *buf, size_t count)
{
    struct ata_host_set *host_set = dev_get_drvdata(dev_addr);
    struct ata_port *ap;
    unsigned int port_no;
    struct ata_device *adev;

    sscanf (buf, "%d\n", &port_no);

    if(port_no==1||port_no==2){
        port_no--;
        tray_port=port_no;
        ap = host_set->ports[port_no];
        adev=&ap->device[0];

        if((ap->flags & ATA_FLAG_EJT_ING)==0 && (adev->class == ATA_DEV_ATAPI) )
        {
            printk("port %d will be trayout!\n",port_no+1);
        ap->flags|= ATA_FLAG_PRE_EJT;
            tray_state=0xff & ~(0x01 << (4*port_no));
    }else{
            printk("unacceptable state\n");
        }
    }else{
        printk("port %d not exist!\n",port_no);
        return count;
    }
    return count;
}
static BUS_ATTR(trayout_dev, S_IRUGO | S_IWUSR, show_trayout_imm_field, store_trayout_imm_field);

static ssize_t
show_re_scan_field(struct bus_type *bus, char *buf)
{
    //struct ata_host_set *host_set = dev_get_drvdata(dev_addr);
    //struct ata_port *ap;
    //struct ata_device *adev;
    return sprintf(buf, "null attritube\n");
}

static ssize_t
store_re_scan_field(struct bus_type *bus, const char *buf, size_t count)
{
    struct ata_host_set *host_set = dev_get_drvdata(dev_addr);
    struct ata_port *ap;
    struct ata_device *adev;
    unsigned int off_port_no;
    unsigned int off_port_no_org;
    u8 i;

    sscanf (buf, "%d\n", &off_port_no);

    off_port_no_org=off_port_no;
    if(off_port_no==1||off_port_no==2){
        printk("port %d will be stop!\n",off_port_no);
        i=1;
        off_port_no--;
    }else if(off_port_no==0xff){
        printk("all port will be stop!\n");
        i=2;
        off_port_no=0;
    }else{
        printk("port %d not exist!\n",off_port_no);
        return count;
    }

    del_timer(&sata_timer);
    flush_scheduled_work();

    while(i){
        ap = host_set->ports[off_port_no];
        adev=&ap->device[0];

        if(adev->flags &ATA_DFLAG_DEV_ONLINE && !(ap->flags & ATA_FLAG_HP_EVENT)){

            host_set->ahost_hp_event|=(0x01<<off_port_no);
            PortStatus[off_port_no]=0;
            adev=&ap->device[0];
            adev->adev_hp_state=ADEV_HP_START;
            ap->flags |= ATA_FLAG_HP_EVENT;
        }else{

            if(ap->flags & ATA_FLAG_HP_EVENT)
                printk("port %d iATA_FLAG_HP_EVENT alert!\n",off_port_no);
            else
                printk("port %d is offline!\n",off_port_no);
        }
        i--;
        off_port_no++;
    }

    mars_sata_work_fn(dev_addr);
    msleep(100);
    printk("host_set->ahost_hp_event:0x%x\n",host_set->ahost_hp_event);
    if(off_port_no_org==0xff){
        PortStatus[0]=0;
        PortStatus[1]=0;
        printk("port %d is offline!\n",off_port_no);
    }else{
        PortStatus[off_port_no_org-1]=0;
    }

    mod_timer(&sata_timer,jiffies+HZ/2);    //remain another port active

    return count;
}
static BUS_ATTR(re_scan, S_IRUGO | S_IWUSR, show_re_scan_field, store_re_scan_field);

static int __init mars_init(void)
{
    int error;

    printk(KERN_INFO"sata driver initial...2009/12/30 14:50\n");
    error = bus_register(&sata_bus_type);
    if (error)
        goto err_bus_fail;
    else {
        bus_create_file(&sata_bus_type, &bus_attr_scan_dev);
        bus_create_file(&sata_bus_type, &bus_attr_offline_dev);
        bus_create_file(&sata_bus_type, &bus_attr_wait_insmod_state);
        bus_create_file(&sata_bus_type, &bus_attr_trayout_dev);
        bus_create_file(&sata_bus_type, &bus_attr_re_scan);

        error = device_register(&sata_sb1);
        if (error)
            goto err_device_fail;
        else{
            error = driver_register(&sata_driver);
            if (error)
                goto err_driver_fail;
        }
    }
    goto finish_out;

err_driver_fail:
    printk(KERN_INFO"sata driver insert fail.\n");
    driver_unregister(&sata_driver);

err_device_fail:
    printk(KERN_INFO"sata device insert fail.\n");
    device_unregister(&sata_sb1);
    bus_remove_file(&sata_bus_type, &bus_attr_scan_dev);
    bus_remove_file(&sata_bus_type, &bus_attr_offline_dev);
    bus_remove_file(&sata_bus_type, &bus_attr_wait_insmod_state);
    bus_remove_file(&sata_bus_type, &bus_attr_trayout_dev);
    bus_remove_file(&sata_bus_type, &bus_attr_re_scan);

err_bus_fail:
    printk(KERN_INFO"sata bus insert fail.\n");
    bus_unregister(&sata_bus_type);

finish_out:
    mod_state=0x1;
    return error;

}

static void __exit mars_exit(void)
{
    printk(KERN_INFO"%s(%d)\n",__func__,__LINE__);
    driver_unregister(&sata_driver);
    device_unregister(&sata_sb1);
    bus_unregister(&sata_bus_type);

}

module_init(mars_init);
module_exit(mars_exit);

