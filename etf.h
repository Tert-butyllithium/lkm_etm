#ifndef _ETF_LANRAN_H
#define _ETF_LANRAN_H
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/device.h>

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
    writel_relaxed(TMC_CTL_CAPT_EN, drvdata->base + TMC_CTL);
}

void tmc_disable_hw(struct tmc_drvdata *drvdata)
{
    writel_relaxed(0x0, drvdata->base + TMC_CTL);
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

    writel_relaxed(TMC_MODE_HARDWARE_FIFO, drvdata->base + TMC_MODE);
    writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI,
                   drvdata->base + TMC_FFCR);
    writel_relaxed(0x0, drvdata->base + TMC_BUFWM);
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

#endif
