#ifndef _ETB_LANRAN_H
#define _ETB_LANRAN_H
#include <asm/local.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/pm_runtime.h>
#include <linux/seq_file.h>
#include <linux/coresight.h>
#include <linux/amba/bus.h>
#include <linux/clk.h>
#include <linux/circ_buf.h>
#include <linux/mm.h>
#include <linux/perf_event.h>

#include <asm/local.h>

#include "coresight.h"

#define ETB_RAM_DEPTH_REG 0x004
#define ETB_STATUS_REG 0x00c
#define ETB_RAM_READ_DATA_REG 0x010
#define ETB_RAM_READ_POINTER 0x014
#define ETB_RAM_WRITE_POINTER 0x018
#define ETB_TRG 0x01c
#define ETB_CTL_REG 0x020
#define ETB_RWD_REG 0x024
#define ETB_FFSR 0x300
#define ETB_FFCR 0x304
#define ETB_ITMISCOP0 0xee0
#define ETB_ITTRFLINACK 0xee4
#define ETB_ITTRFLIN 0xee8
#define ETB_ITATBDATA0 0xeeC
#define ETB_ITATBCTR2 0xef0
#define ETB_ITATBCTR1 0xef4
#define ETB_ITATBCTR0 0xef8

/* register description */
/* STS - 0x00C */
#define ETB_STATUS_RAM_FULL BIT(0)
/* CTL - 0x020 */
#define ETB_CTL_CAPT_EN BIT(0)
/* FFCR - 0x304 */
#define ETB_FFCR_EN_FTC BIT(0)
#define ETB_FFCR_FON_MAN BIT(6)
#define ETB_FFCR_STOP_FI BIT(12)
#define ETB_FFCR_STOP_TRIGGER BIT(13)

#define ETB_FFCR_BIT 6
#define ETB_FFSR_BIT 1
#define ETB_FRAME_SIZE_WORDS 4

/**
 * struct etb_drvdata - specifics associated to an ETB component
 * @base:	memory mapped base address for this component.
 * @dev:	the device entity associated to this component.
 * @atclk:	optional clock for the core parts of the ETB.
 * @csdev:	component vitals needed by the framework.
 * @miscdev:	specifics to handle "/dev/xyz.etb" entry.
 * @spinlock:	only one at a time pls.
 * @reading:	synchronise user space access to etb buffer.
 * @mode:	this ETB is being used.
 * @buf:	area of memory where ETB buffer content gets sent.
 * @buffer_depth: size of @buf.
 * @trigger_cntr: amount of words to store after a trigger.
 */

struct etb_drvdata
{
    void __iomem *base;
    struct device *dev;
    struct clk *atclk;
    struct coresight_device *csdev;
    struct miscdevice miscdev;
    spinlock_t spinlock;
    local_t reading;
    local_t mode;
    u8 *buf;
    u32 buffer_depth;
    u32 trigger_cntr;
};

// TraceCaptEn
static void etb_enable_hw(struct etb_drvdata *drvdata)
{
    int i;
    u32 depth;

    CS_UNLOCK(drvdata->base);

    depth = drvdata->buffer_depth;
    /* reset write RAM pointer address */
    writel_relaxed(0x0, drvdata->base + ETB_RAM_WRITE_POINTER);
    /* clear entire RAM buffer */
    for (i = 0; i < depth; i++)
        writel_relaxed(0x0, drvdata->base + ETB_RWD_REG);

    /* reset write RAM pointer address */
    writel_relaxed(0x0, drvdata->base + ETB_RAM_WRITE_POINTER);
    /* reset read RAM pointer address */
    writel_relaxed(0x0, drvdata->base + ETB_RAM_READ_POINTER);

    writel_relaxed(drvdata->trigger_cntr, drvdata->base + ETB_TRG);
    writel_relaxed(ETB_FFCR_EN_FTC | ETB_FFCR_STOP_TRIGGER,
                   drvdata->base + ETB_FFCR);
    /* ETB trace capture enable */
    writel_relaxed(ETB_CTL_CAPT_EN, drvdata->base + ETB_CTL_REG);

    CS_LOCK(drvdata->base);
}

static void etb_disable_hw(struct etb_drvdata *drvdata)
{
    u32 ffcr;

    CS_UNLOCK(drvdata->base);

    ffcr = readl_relaxed(drvdata->base + ETB_FFCR);
    /* stop formatter when a stop has completed */
    ffcr |= ETB_FFCR_STOP_FI;
    writel_relaxed(ffcr, drvdata->base + ETB_FFCR);
    /* manually generate a flush of the system */
    ffcr |= ETB_FFCR_FON_MAN;
    writel_relaxed(ffcr, drvdata->base + ETB_FFCR);

    if (coresight_timeout(drvdata->base, ETB_FFCR, ETB_FFCR_BIT, 0))
    {
        dev_err(drvdata->dev,
                "timeout while waiting for completion of Manual Flush\n");
    }

    /* disable trace capture */
    writel_relaxed(0x0, drvdata->base + ETB_CTL_REG);

    if (coresight_timeout(drvdata->base, ETB_FFSR, ETB_FFSR_BIT, 1))
    {
        dev_err(drvdata->dev,
                "timeout while waiting for Formatter to Stop\n");
    }

    CS_LOCK(drvdata->base);
}

#endif