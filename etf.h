#ifndef _ETF_LANRAN_H
#define _ETF_LANRAN_H
#define _ETF_RETRIEVED_IMPLEMENTED_LANRAN
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "coresight.h"

/* TMC_FFCR - 0x304 */
#define TMC_FFCR_FLUSHMAN_BIT 6
#define TMC_FFCR_EN_FMT BIT(0)
#define TMC_FFCR_EN_TI BIT(1)
#define TMC_FFCR_FON_FLIN BIT(4)
#define TMC_FFCR_FON_TRIG_EVT BIT(5)
#define TMC_FFCR_TRIGON_TRIGIN BIT(8)
#define TMC_FFCR_STOP_ON_FLUSH BIT(12)

#define TMC_FFSR 0x300
#define TMC_FFCR 0x304
#define TMC_PSCR 0x308
#define TMC_MODE 0x028
#define TMC_BUFWM 0x034
#define TMC_CTL 0x020

/* TMC_CTL - 0x020 */
#define TMC_CTL_CAPT_EN BIT(0)

#define TMC_STS 0x00c
#define TMC_STS_TMCREADY_BIT 2

enum tmc_mode
{
    TMC_MODE_CIRCULAR_BUFFER,
    TMC_MODE_SOFTWARE_FIFO,
    TMC_MODE_HARDWARE_FIFO,
};

/**
 * struct tmc_drvdata - specifics associated to an TMC component
 * @base:	memory mapped base address for this component.
 * @dev:	the device entity associated to this component.
 * @csdev:	component vitals needed by the framework.
 */
struct tmc_drvdata
{
    void __iomem *base;
    struct device *dev;
    struct coresight_device *csdev;
};

void tmc_wait_for_tmcready(struct tmc_drvdata *drvdata)
{
    /* Ensure formatter, unformatter and hardware fifo are empty */
    if (coresight_timeout(drvdata->base,
                          TMC_STS, TMC_STS_TMCREADY_BIT, 1))
    {
        dev_err(drvdata->dev,
                "timeout while waiting for TMC to be Ready\n");
    }
}

void tmc_enable_hw(struct tmc_drvdata *drvdata)
{
    // change to 0x1
    writel_relaxed(0x1, drvdata->base + TMC_CTL);
}

void tmc_disable_hw(struct tmc_drvdata *drvdata)
{
    u32 reg;
    writel_relaxed(0x0, drvdata->base + TMC_CTL);

    //Check if formatter stopped
    reg = readl_relaxed(drvdata->base + TMC_FFSR);
    while ((reg & 0x2) != 0x2)
    {
        reg = readl_relaxed(drvdata->base + TMC_FFSR);
        printk("[ETM:]: stopping FFSR....");
    }
}

void tmc_flush_and_stop(struct tmc_drvdata *drvdata)
{
    u32 ffcr;

    ffcr = readl_relaxed(drvdata->base + TMC_FFCR);
    ffcr |= TMC_FFCR_STOP_ON_FLUSH;
    writel_relaxed(ffcr, drvdata->base + TMC_FFCR);
    ffcr |= BIT(TMC_FFCR_FLUSHMAN_BIT);
    writel_relaxed(ffcr, drvdata->base + TMC_FFCR);
    /* Ensure flush completes */
    if (coresight_timeout(drvdata->base,
                          TMC_FFCR, TMC_FFCR_FLUSHMAN_BIT, 0))
    {
        dev_err(drvdata->dev,
                "timeout while waiting for completion of Manual Flush\n");
    }

    tmc_wait_for_tmcready(drvdata);
}

static void tmc_etf_enable_hw(struct tmc_drvdata *drvdata)
{
    CS_UNLOCK(drvdata->base);

    /* Wait for TMCSReady bit to be set */
    tmc_wait_for_tmcready(drvdata);

    writel_relaxed(TMC_MODE_CIRCULAR_BUFFER, drvdata->base + TMC_MODE);
    writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI,
                   drvdata->base + TMC_FFCR);
    writel_relaxed(0x0, drvdata->base + TMC_BUFWM);
    writel_relaxed(0x800, drvdata->base + 0x01c);
    tmc_enable_hw(drvdata);

    CS_LOCK(drvdata->base);
}

static void tmc_etf_disable_hw(struct tmc_drvdata *drvdata)
{
    CS_UNLOCK(drvdata->base);

    tmc_flush_and_stop(drvdata);
    tmc_disable_hw(drvdata);

    CS_LOCK(drvdata->base);
}

#ifdef _ETF_RETRIEVED_IMPLEMENTED_LANRAN
#define BUF_SIZE 0x10000
#define ETB_STATUS_REG 0x00c
#define MY_FILE "/sdcard/Download/trace_result"

static void tmc_eft_retrieve(struct tmc_drvdata *drvdata)
{
    static const u32 barrier_pkt[] = {0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x0};
    void __iomem *readbuff;
    unsigned int reg, status;
    bool lost = false;
    const unsigned int *barrier = barrier_pkt;
    static char tracebuf[BUF_SIZE + 10];
    static struct file *file = NULL;

    //clean up the trace buffer
    memset(tracebuf, 0, BUF_SIZE);
    char *bufp = tracebuf;

    unsigned int bufferlen = 0;
    int i;
    /*
	 * Get a hold of the status register and see if a wrap around
	 * has occurred.
	 */
    status = ioread32(drvdata->base + 0x00c);
    if ((status & 0x1) == 0x1)
    {
        printk("[ETM:] etf buffer is full\n");
        lost = true;
    }

    // get the etf buffer
    readbuff = (void *)(drvdata->base + 0x010);

    int readtimes = 0;

    int memwidth = BUF_SIZE / 4;
    // printk("[ETM: ] RRD: 0x%x", reg);
    for (i = 0; i < memwidth; i++)
    {
        reg = ioread32(readbuff);
        readtimes++;

        if (reg == 0xFFFFFFFF)
            break;

        if (lost && *barrier)
        {
            reg = *barrier;
            barrier++;
        }
        memcpy(bufp, &reg, 4);
        bufp += 4;
        bufferlen += 4;
    }

    printk(KERN_INFO "[ETM:] read to tracebuf from etf total %d times.\n", readtimes);
    //write tracebuf to file
    mm_segment_t old_fs;
    if (file == NULL)
        file = filp_open(MY_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);
    if (IS_ERR(file))
    {
        printk(KERN_INFO "[ETM:]error occured while opening file %s, exiting...\n", MY_FILE);
        return;
    }
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    file->f_op->write(file, (char *)tracebuf, bufferlen, &file->f_pos);
    set_fs(old_fs);
    filp_close(file, NULL);
    file = NULL;

    //disable trace
    reg = ioread32(drvdata->base + TMC_CTL);
    reg &= 0x0;
    iowrite32(reg, drvdata->base + TMC_CTL);

    return;
}

#endif

#endif
