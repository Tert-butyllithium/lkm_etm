#ifndef _ETF_LANRAN_H
#define _ETF_LANRAN_H
#define _ETF_RETRIEVED_IMPLEMENTED_LANRAN
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

#include "coresight.h"

/* TMC_FFCR - 0x304 */
#define TMC_FFCR_FLUSHMAN_BIT 6
#define TMC_FFCR_EN_FMT BIT(0)
#define TMC_FFCR_EN_TI BIT(1)
#define TMC_FFCR_FON_FLIN BIT(4)
#define TMC_FFCR_FON_TRIG_EVT BIT(5)
#define TMC_FFCR_TRIGON_TRIGIN BIT(8)
#define TMC_FFCR_STOP_ON_FLUSH BIT(12)

#define TMC_RSZ 0x004
#define TMC_STS 0x00c
#define TMC_RRD 0x010
#define TMC_RRP 0x014
#define TMC_RWP 0x018
#define TMC_TRG 0x01c
#define TMC_CTL 0x020
#define TMC_RWD 0x024
#define TMC_MODE 0x028
#define TMC_LBUFLEVEL 0x02c
#define TMC_CBUFLEVEL 0x030
#define TMC_BUFWM 0x034
#define TMC_RRPHI 0x038
#define TMC_RWPHI 0x03c
#define TMC_AXICTL 0x110
#define TMC_DBALO 0x118
#define TMC_DBAHI 0x11c
#define TMC_FFSR 0x300
#define TMC_FFCR 0x304
#define TMC_PSCR 0x308
#define TMC_ITMISCOP0 0xee0
#define TMC_ITTRFLIN 0xee8
#define TMC_ITATBDATA0 0xeec
#define TMC_ITATBCTR2 0xef0
#define TMC_ITATBCTR1 0xef4
#define TMC_ITATBCTR0 0xef8

#define TMC_FFSR 0x300
#define TMC_FFCR 0x304
#define TMC_PSCR 0x308
#define TMC_MODE 0x028
#define TMC_BUFWM 0x034

/* TMC_CTL - 0x020 */
#define TMC_CTL_CAPT_EN BIT(0)

#define TMC_STS 0x00c
#define TMC_STS_TMCREADY_BIT 2
#define TMC_STS_FULL BIT(0)

static const u32 barrier_pkt[] = {0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x0};

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
 * @trigger_cntr: amount of words to store after a trigger.
 */
struct tmc_drvdata
{
    void __iomem *base;
    struct device *dev;
    struct coresight_device *csdev;
    char* buf;
    u32 len;
    u32 memwidth;
    u32 trigger_cntr;
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



static void tmc_etb_dump_hw(struct tmc_drvdata *drvdata)
{
	bool lost = false;
	char *bufp;
	const u32 *barrier;
	u32 read_data, status;
	int i;

	/*
	 * Get a hold of the status register and see if a wrap around
	 * has occurred.
	 */
	status = readl_relaxed(drvdata->base + TMC_STS);
	if (status & TMC_STS_FULL)
		lost = true;

	bufp = drvdata->buf;
    // printk("[ETM:] bufp: %p",&bufp);
	drvdata->len = 0;
	barrier = barrier_pkt;
	while (1) {
		for (i = 0; i < drvdata->memwidth; i++) {
			read_data = readl_relaxed(drvdata->base + TMC_RRD);
			if (read_data == 0xFFFFFFFF)
				return;

			if (lost && *barrier) {
				read_data = *barrier;
				barrier++;
			}
            printk("[ETM:] read buffer %d: %08x",i,read_data);
			memcpy(bufp, &read_data, 4);
			bufp += 4;
			drvdata->len += 4;
		}
	}
}



static void tmc_etb_enable_hw(struct tmc_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);

	/* Wait for TMCSReady bit to be set */
	tmc_wait_for_tmcready(drvdata);

	writel_relaxed(TMC_MODE_CIRCULAR_BUFFER, drvdata->base + TMC_MODE);
	writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI |
		       TMC_FFCR_FON_FLIN | TMC_FFCR_FON_TRIG_EVT |
		       TMC_FFCR_TRIGON_TRIGIN,
		       drvdata->base + TMC_FFCR);

	writel_relaxed(drvdata->trigger_cntr, drvdata->base + TMC_TRG);
	tmc_enable_hw(drvdata);

	CS_LOCK(drvdata->base);
}

static void tmc_etb_disable_hw(struct tmc_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);

	tmc_flush_and_stop(drvdata);
	/*
	 * When operating in sysFS mode the content of the buffer needs to be
	 * read before the TMC is disabled.
	 */
	// if (drvdata->mode == CS_MODE_SYSFS)
    printk("[ETM:] get buf contents");
	tmc_etb_dump_hw(drvdata);
	tmc_disable_hw(drvdata);

	CS_LOCK(drvdata->base);
}


#define MY_FILE "/home/root/trace_result/trace1.out"

static void save_to_file(struct tmc_drvdata *drvdata)
{
    mm_segment_t old_fs;
    static struct file *file = NULL;

    printk("saving to file");
    if (file == NULL)
        file = filp_open(MY_FILE, O_RDWR | O_CREAT, S_IRWXU);
    if (IS_ERR(file))
    {
        printk(KERN_INFO "[ETM:]error occured while opening file %s, exiting...\n", MY_FILE);
        return;
    }
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    kernel_write(file, drvdata->buf, drvdata->len, &file->f_pos);
    set_fs(old_fs);
    filp_close(file, NULL);
    file = NULL;

    return;
}

#endif
