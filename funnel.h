#ifndef _FUNNEL_LANRAN_H
#define _FUNNEL_LANRAN_H

#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/io.h>

#define CORESIGHT_UNLOCK 0xc5acce55
#define CORESIGHT_LAR 0xfb0

#define FUNNEL_FUNCTL 0x000
#define FUNNEL_PRICTL 0x004

#define FUNNEL_HOLDTIME_MASK 0xf00
#define FUNNEL_HOLDTIME_SHFT 0x8
#define FUNNEL_HOLDTIME (0x7 << FUNNEL_HOLDTIME_SHFT)

struct funnel_drvdata
{
    void __iomem *base;
    struct device *dev;
    struct clk *atclk;
    struct coresight_device *csdev;
    unsigned long priority;
};

#ifdef _DEBUG_LANRAN
u32 __tmp;
#endif

static inline void CS_UNLOCK(void __iomem *addr)
{
    do
    {
#ifdef _DEBUG_LANRAN
        __tmp = readl_relaxed(addr + CORESIGHT_LAR);
        printk(KERN_INFO "[" DRVR_NAME "]"
                         "LAR: 0x%x --> 0x%x",
               __tmp, CORESIGHT_UNLOCK);
#endif
        writel_relaxed(CORESIGHT_UNLOCK, addr + CORESIGHT_LAR);
        /* Make sure everyone has seen this */
        mb();
    } while (0);
}

static inline void CS_LOCK(void __iomem *addr)
{
    do
    {
        /* Wait for things to settle */
        mb();
#ifdef _DEBUG_LANRAN
        __tmp = readl_relaxed(addr + CORESIGHT_LAR);
        printk(KERN_INFO "[" DRVR_NAME "]"
                         "LAR: 0x%x --> 0x%x",
               __tmp, 0x0);
#endif
        writel_relaxed(0x0, addr + CORESIGHT_LAR);
    } while (0);
}

static void funnel_enable_hw(struct funnel_drvdata *drvdata, int port)
{
    u32 functl;

    CS_UNLOCK(drvdata->base);

    functl = readl_relaxed(drvdata->base + FUNNEL_FUNCTL);
#ifdef _DEBUG_LANRAN
    _tmp = functl;
#endif
    functl &= ~FUNNEL_HOLDTIME_MASK;
    functl |= FUNNEL_HOLDTIME;
    functl |= (1 << port);
#ifdef _DEBUG_LANRAN
    printk(KERN_INFO "[" DRVR_NAME "]"
                     "enable: CSTF: 0x%x --> 0x%x",
           __tmp, functl);
#endif
    writel_relaxed(functl, drvdata->base + FUNNEL_FUNCTL);
    // writel_relaxed(drvdata->priority, drvdata->base + FUNNEL_PRICTL);

    CS_LOCK(drvdata->base);
}

static void funnel_disable_hw(struct funnel_drvdata *drvdata, int inport)
{
    u32 functl;

    CS_UNLOCK(drvdata->base);

    functl = readl_relaxed(drvdata->base + FUNNEL_FUNCTL);
#ifdef _DEBUG_LANRAN
    _tmp = functl;
#endif
    functl &= ~(1 << inport);
#ifdef _DEBUG_LANRAN
    printk(KERN_INFO "[" DRVR_NAME "]"
                     "disable: CSTF: 0x%x --> 0x%x",
           __tmp, functl);
#endif
    writel_relaxed(functl, drvdata->base + FUNNEL_FUNCTL);

    CS_LOCK(drvdata->base);
}
#endif