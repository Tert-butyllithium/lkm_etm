#ifndef _ETM_LANRAN_H
#define _ETM_LANRAN_H

#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/coresight.h>
#include <linux/coresight-pmu.h>
#include <linux/pm_wakeup.h>
#include <linux/amba/bus.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/perf_event.h>
#include <linux/pm_runtime.h>
#include <asm/sections.h>
#include <asm/local.h>

#include "coresight.h"

#define TRCOSLAR 0x300

static int boot_enable;

/* The number of ETMv4 currently registered */
static int etm4_count;
static struct etmv4_drvdata *etmdrvdata[NR_CPUS];
static void etm4_set_default_config(struct etmv4_config *config);
static int etm4_set_event_filters(struct etmv4_drvdata *drvdata,
                                  struct perf_event *event);

static enum cpuhp_state hp_online;

struct etmv4_drvdata
{
    void __iomem *base;
    struct device *dev;
    struct coresight_device *csdev;
    spinlock_t spinlock;
    local_t mode;
    int cpu;
    u8 arch;
    u8 nr_pe;
    u8 nr_pe_cmp;
    u8 nr_addr_cmp;
    u8 nr_cntr;
    u8 nr_ext_inp;
    u8 numcidc;
    u8 numvmidc;
    u8 nrseqstate;
    u8 nr_event;
    u8 nr_resource;
    u8 nr_ss_cmp;
    u8 trcid;
    u8 trcid_size;
    u8 ts_size;
    u8 ctxid_size;
    u8 vmid_size;
    u8 ccsize;
    u8 ccitmin;
    u8 s_ex_level;
    u8 ns_ex_level;
    u8 q_support;
    bool sticky_enable;
    bool boot_enable;
    bool os_unlock;
    bool instrp0;
    bool trcbb;
    bool trccond;
    bool retstack;
    bool trccci;
    bool trc_error;
    bool syncpr;
    bool stallctl;
    bool sysstall;
    bool nooverflow;
    bool atbtrig;
    bool lpoverride;
    // struct etmv4_config		config;
};

static void etm4_os_unlock(struct etmv4_drvdata *drvdata)
{
    /* Writing any value to ETMOSLAR unlocks the trace registers */
    writel_relaxed(0x0, drvdata->base + TRCOSLAR);
    drvdata->os_unlock = true;
    isb();
}

// static int etm4_cpu_id(struct coresight_device *csdev)
// {
//     struct etmv4_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

//     return drvdata->cpu;
// }

// static int etm4_trace_id(struct coresight_device *csdev)
// {
//     struct etmv4_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

//     return drvdata->trcid;
// }

static void etm4_disable_hw(void *info)
{
    u32 control;
    struct etmv4_drvdata *drvdata = info;

    CS_UNLOCK(drvdata->base);

    /* power can be removed from the trace unit now */
    control = readl_relaxed(drvdata->base + TRCPDCR);
    control &= ~TRCPDCR_PU;
    writel_relaxed(control, drvdata->base + TRCPDCR);

    control = readl_relaxed(drvdata->base + TRCPRGCTLR);

    /* EN, bit[0] Trace unit enable bit */
    control &= ~0x1;

    /* make sure everything completes before disabling */
    mb();
    isb();
    writel_relaxed(control, drvdata->base + TRCPRGCTLR);

    CS_LOCK(drvdata->base);

    dev_dbg(drvdata->dev, "cpu: %d disable smp call done\n", drvdata->cpu);
}

static void etm4_enable_hw(void *info)
{
    int i;
    struct etmv4_drvdata *drvdata = info;
    struct etmv4_config *config = &drvdata->config;

    CS_UNLOCK(drvdata->base);

    etm4_os_unlock(drvdata);

    /* Disable the trace unit before programming trace registers */
    writel_relaxed(0, drvdata->base + TRCPRGCTLR);

    /* wait for TRCSTATR.IDLE to go up */
    if (coresight_timeout(drvdata->base, TRCSTATR, TRCSTATR_IDLE_BIT, 1))
        dev_err(drvdata->dev,
                "timeout while waiting for Idle Trace Status\n");

    writel_relaxed(config->pe_sel, drvdata->base + TRCPROCSELR);
    writel_relaxed(config->cfg, drvdata->base + TRCCONFIGR);
    /* nothing specific implemented */
    writel_relaxed(0x0, drvdata->base + TRCAUXCTLR);
    writel_relaxed(config->eventctrl0, drvdata->base + TRCEVENTCTL0R);
    writel_relaxed(config->eventctrl1, drvdata->base + TRCEVENTCTL1R);
    writel_relaxed(config->stall_ctrl, drvdata->base + TRCSTALLCTLR);
    writel_relaxed(config->ts_ctrl, drvdata->base + TRCTSCTLR);
    writel_relaxed(config->syncfreq, drvdata->base + TRCSYNCPR);
    writel_relaxed(config->ccctlr, drvdata->base + TRCCCCTLR);
    writel_relaxed(config->bb_ctrl, drvdata->base + TRCBBCTLR);
    writel_relaxed(drvdata->trcid, drvdata->base + TRCTRACEIDR);
    writel_relaxed(config->vinst_ctrl, drvdata->base + TRCVICTLR);
    writel_relaxed(config->viiectlr, drvdata->base + TRCVIIECTLR);
    writel_relaxed(config->vissctlr,
                   drvdata->base + TRCVISSCTLR);
    writel_relaxed(config->vipcssctlr,
                   drvdata->base + TRCVIPCSSCTLR);
    for (i = 0; i < drvdata->nrseqstate - 1; i++)
        writel_relaxed(config->seq_ctrl[i],
                       drvdata->base + TRCSEQEVRn(i));
    writel_relaxed(config->seq_rst, drvdata->base + TRCSEQRSTEVR);
    writel_relaxed(config->seq_state, drvdata->base + TRCSEQSTR);
    writel_relaxed(config->ext_inp, drvdata->base + TRCEXTINSELR);
    for (i = 0; i < drvdata->nr_cntr; i++)
    {
        writel_relaxed(config->cntrldvr[i],
                       drvdata->base + TRCCNTRLDVRn(i));
        writel_relaxed(config->cntr_ctrl[i],
                       drvdata->base + TRCCNTCTLRn(i));
        writel_relaxed(config->cntr_val[i],
                       drvdata->base + TRCCNTVRn(i));
    }

    /* Resource selector pair 0 is always implemented and reserved */
    for (i = 0; i < drvdata->nr_resource * 2; i++)
        writel_relaxed(config->res_ctrl[i],
                       drvdata->base + TRCRSCTLRn(i));

    for (i = 0; i < drvdata->nr_ss_cmp; i++)
    {
        writel_relaxed(config->ss_ctrl[i],
                       drvdata->base + TRCSSCCRn(i));
        writel_relaxed(config->ss_status[i],
                       drvdata->base + TRCSSCSRn(i));
        writel_relaxed(config->ss_pe_cmp[i],
                       drvdata->base + TRCSSPCICRn(i));
    }
    for (i = 0; i < drvdata->nr_addr_cmp; i++)
    {
        writeq_relaxed(config->addr_val[i],
                       drvdata->base + TRCACVRn(i));
        writeq_relaxed(config->addr_acc[i],
                       drvdata->base + TRCACATRn(i));
    }
    for (i = 0; i < drvdata->numcidc; i++)
        writeq_relaxed(config->ctxid_pid[i],
                       drvdata->base + TRCCIDCVRn(i));
    writel_relaxed(config->ctxid_mask0, drvdata->base + TRCCIDCCTLR0);
    writel_relaxed(config->ctxid_mask1, drvdata->base + TRCCIDCCTLR1);

    for (i = 0; i < drvdata->numvmidc; i++)
        writeq_relaxed(config->vmid_val[i],
                       drvdata->base + TRCVMIDCVRn(i));
    writel_relaxed(config->vmid_mask0, drvdata->base + TRCVMIDCCTLR0);
    writel_relaxed(config->vmid_mask1, drvdata->base + TRCVMIDCCTLR1);

    /*
	 * Request to keep the trace unit powered and also
	 * emulation of powerdown
	 */
    writel_relaxed(readl_relaxed(drvdata->base + TRCPDCR) | TRCPDCR_PU,
                   drvdata->base + TRCPDCR);

    /* Enable the trace unit */
    writel_relaxed(1, drvdata->base + TRCPRGCTLR);

    /* wait for TRCSTATR.IDLE to go back down to '0' */
    if (coresight_timeout(drvdata->base, TRCSTATR, TRCSTATR_IDLE_BIT, 0))
        dev_err(drvdata->dev,
                "timeout while waiting for Idle Trace Status\n");

    CS_LOCK(drvdata->base);

    dev_dbg(drvdata->dev, "cpu: %d enable smp call done\n", drvdata->cpu);
}

#endif
