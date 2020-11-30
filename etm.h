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
#include <linux/pm_wakeup.h>
#include <linux/amba/bus.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/perf_event.h>
#include <linux/pm_runtime.h>
#include <asm/sections.h>
#include <asm/local.h>

#include "coresight.h"

/*
 * Device registers:
 * 0x000 - 0x2FC: Trace		registers
 * 0x300 - 0x314: Management	registers
 * 0x318 - 0xEFC: Trace		registers
 * 0xF00: Management		registers
 * 0xFA0 - 0xFA4: Trace		registers
 * 0xFA8 - 0xFFC: Management	registers
 */
/* Trace registers (0x000-0x2FC) */
/* Main control and configuration registers */
#define TRCPRGCTLR 0x004
#define TRCPROCSELR 0x008
#define TRCSTATR 0x00C
#define TRCCONFIGR 0x010
#define TRCAUXCTLR 0x018
#define TRCEVENTCTL0R 0x020
#define TRCEVENTCTL1R 0x024
#define TRCSTALLCTLR 0x02C
#define TRCTSCTLR 0x030
#define TRCSYNCPR 0x034
#define TRCCCCTLR 0x038
#define TRCBBCTLR 0x03C
#define TRCTRACEIDR 0x040
#define TRCQCTLR 0x044
/* Filtering control registers */
#define TRCVICTLR 0x080
#define TRCVIIECTLR 0x084
#define TRCVISSCTLR 0x088
#define TRCVIPCSSCTLR 0x08C
#define TRCVDCTLR 0x0A0
#define TRCVDSACCTLR 0x0A4
#define TRCVDARCCTLR 0x0A8
/* Derived resources registers */
#define TRCSEQEVRn(n) (0x100 + (n * 4))
#define TRCSEQRSTEVR 0x118
#define TRCSEQSTR 0x11C
#define TRCEXTINSELR 0x120
#define TRCCNTRLDVRn(n) (0x140 + (n * 4))
#define TRCCNTCTLRn(n) (0x150 + (n * 4))
#define TRCCNTVRn(n) (0x160 + (n * 4))
/* ID registers */
#define TRCIDR8 0x180
#define TRCIDR9 0x184
#define TRCIDR10 0x188
#define TRCIDR11 0x18C
#define TRCIDR12 0x190
#define TRCIDR13 0x194
#define TRCIMSPEC0 0x1C0
#define TRCIMSPECn(n) (0x1C0 + (n * 4))
#define TRCIDR0 0x1E0
#define TRCIDR1 0x1E4
#define TRCIDR2 0x1E8
#define TRCIDR3 0x1EC
#define TRCIDR4 0x1F0
#define TRCIDR5 0x1F4
#define TRCIDR6 0x1F8
#define TRCIDR7 0x1FC
/* Resource selection registers */
#define TRCRSCTLRn(n) (0x200 + (n * 4))
/* Single-shot comparator registers */
#define TRCSSCCRn(n) (0x280 + (n * 4))
#define TRCSSCSRn(n) (0x2A0 + (n * 4))
#define TRCSSPCICRn(n) (0x2C0 + (n * 4))
/* Management registers (0x300-0x314) */
#define TRCOSLAR 0x300
#define TRCOSLSR 0x304
#define TRCPDCR 0x310
#define TRCPDSR 0x314
/* Trace registers (0x318-0xEFC) */
/* Comparator registers */
#define TRCACVRn(n) (0x400 + (n * 8))
#define TRCACATRn(n) (0x480 + (n * 8))
#define TRCDVCVRn(n) (0x500 + (n * 16))
#define TRCDVCMRn(n) (0x580 + (n * 16))
#define TRCCIDCVRn(n) (0x600 + (n * 8))
#define TRCVMIDCVRn(n) (0x640 + (n * 8))
#define TRCCIDCCTLR0 0x680
#define TRCCIDCCTLR1 0x684
#define TRCVMIDCCTLR0 0x688
#define TRCVMIDCCTLR1 0x68C
/* Management register (0xF00) */
/* Integration control registers */
#define TRCITCTRL 0xF00
/* Trace registers (0xFA0-0xFA4) */
/* Claim tag registers */
#define TRCCLAIMSET 0xFA0
#define TRCCLAIMCLR 0xFA4
/* Management registers (0xFA8-0xFFC) */
#define TRCDEVAFF0 0xFA8
#define TRCDEVAFF1 0xFAC
#define TRCLAR 0xFB0
#define TRCLSR 0xFB4
#define TRCAUTHSTATUS 0xFB8
#define TRCDEVARCH 0xFBC
#define TRCDEVID 0xFC8
#define TRCDEVTYPE 0xFCC
#define TRCPIDR4 0xFD0
#define TRCPIDR5 0xFD4
#define TRCPIDR6 0xFD8
#define TRCPIDR7 0xFDC
#define TRCPIDR0 0xFE0
#define TRCPIDR1 0xFE4
#define TRCPIDR2 0xFE8
#define TRCPIDR3 0xFEC
#define TRCCIDR0 0xFF0
#define TRCCIDR1 0xFF4
#define TRCCIDR2 0xFF8
#define TRCCIDR3 0xFFC

/* ETMv4 resources */
#define ETM_MAX_NR_PE 8
#define ETMv4_MAX_CNTR 4
#define ETM_MAX_SEQ_STATES 4
#define ETM_MAX_EXT_INP_SEL 4
#define ETM_MAX_EXT_INP 256
#define ETM_MAX_EXT_OUT 4
#define ETM_MAX_SINGLE_ADDR_CMP 16
#define ETM_MAX_ADDR_RANGE_CMP (ETM_MAX_SINGLE_ADDR_CMP / 2)
#define ETM_MAX_DATA_VAL_CMP 8
#define ETMv4_MAX_CTXID_CMP 8
#define ETM_MAX_VMID_CMP 8
#define ETM_MAX_PE_CMP 8
#define ETM_MAX_RES_SEL 16
#define ETM_MAX_SS_CMP 8

#define ETM_ARCH_V4 0x40
#define ETMv4_SYNC_MASK 0x1F
#define ETM_CYC_THRESHOLD_MASK 0xFFF
#define ETM_CYC_THRESHOLD_DEFAULT 0x100
#define ETMv4_EVENT_MASK 0xFF
#define ETM_CNTR_MAX_VAL 0xFFFF
#define ETM_TRACEID_MASK 0x3f

/* PowerDown Control Register bits */
#define TRCPDCR_PU BIT(3)

#define TRCOSLAR 0x300
#define TRCSTATR_IDLE_BIT 0

// static int boot_enable;

/* The number of ETMv4 currently registered */
// static int etm4_count;
// static struct etmv4_drvdata *etmdrvdata[NR_CPUS];
// static void etm4_set_default_config(struct etmv4_config *config);
// static int etm4_set_event_filters(struct etmv4_drvdata *drvdata,
//                                   struct perf_event *event);

// static enum cpuhp_state hp_online;

/**
 * struct etmv4_config - configuration information related to an ETMv4
 * @mode:	Controls various modes supported by this ETM.
 * @pe_sel:	Controls which PE to trace.
 * @cfg:	Controls the tracing options.
 * @eventctrl0: Controls the tracing of arbitrary events.
 * @eventctrl1: Controls the behavior of the events that @event_ctrl0 selects.
 * @stallctl:	If functionality that prevents trace unit buffer overflows
 *		is available.
 * @ts_ctrl:	Controls the insertion of global timestamps in the
 *		trace streams.
 * @syncfreq:	Controls how often trace synchronization requests occur.
 *		the TRCCCCTLR register.
 * @ccctlr:	Sets the threshold value for cycle counting.
 * @vinst_ctrl:	Controls instruction trace filtering.
 * @viiectlr:	Set or read, the address range comparators.
 * @vissctlr:	Set, or read, the single address comparators that control the
 *		ViewInst start-stop logic.
 * @vipcssctlr:	Set, or read, which PE comparator inputs can control the
 *		ViewInst start-stop logic.
 * @seq_idx:	Sequencor index selector.
 * @seq_ctrl:	Control for the sequencer state transition control register.
 * @seq_rst:	Moves the sequencer to state 0 when a programmed event occurs.
 * @seq_state:	Set, or read the sequencer state.
 * @cntr_idx:	Counter index seletor.
 * @cntrldvr:	Sets or returns the reload count value for a counter.
 * @cntr_ctrl:	Controls the operation of a counter.
 * @cntr_val:	Sets or returns the value for a counter.
 * @res_idx:	Resource index selector.
 * @res_ctrl:	Controls the selection of the resources in the trace unit.
 * @ss_ctrl:	Controls the corresponding single-shot comparator resource.
 * @ss_status:	The status of the corresponding single-shot comparator.
 * @ss_pe_cmp:	Selects the PE comparator inputs for Single-shot control.
 * @addr_idx:	Address comparator index selector.
 * @addr_val:	Value for address comparator.
 * @addr_acc:	Address comparator access type.
 * @addr_type:	Current status of the comparator register.
 * @ctxid_idx:	Context ID index selector.
 * @ctxid_pid:	Value of the context ID comparator.
 * @ctxid_vpid:	Virtual PID seen by users if PID namespace is enabled, otherwise
 *		the same value of ctxid_pid.
 * @ctxid_mask0:Context ID comparator mask for comparator 0-3.
 * @ctxid_mask1:Context ID comparator mask for comparator 4-7.
 * @vmid_idx:	VM ID index selector.
 * @vmid_val:	Value of the VM ID comparator.
 * @vmid_mask0:	VM ID comparator mask for comparator 0-3.
 * @vmid_mask1:	VM ID comparator mask for comparator 4-7.
 * @ext_inp:	External input selection.
 */
struct etmv4_config
{
    u32 mode;
    u32 pe_sel;
    u32 cfg;
    u32 eventctrl0;
    u32 eventctrl1;
    u32 stall_ctrl;
    u32 ts_ctrl;
    u32 syncfreq;
    u32 ccctlr;
    u32 bb_ctrl;
    u32 vinst_ctrl;
    u32 viiectlr;
    u32 vissctlr;
    u32 vipcssctlr;
    u8 seq_idx;
    u32 seq_ctrl[ETM_MAX_SEQ_STATES];
    u32 seq_rst;
    u32 seq_state;
    u8 cntr_idx;
    u32 cntrldvr[ETMv4_MAX_CNTR];
    u32 cntr_ctrl[ETMv4_MAX_CNTR];
    u32 cntr_val[ETMv4_MAX_CNTR];
    u8 res_idx;
    u32 res_ctrl[ETM_MAX_RES_SEL];
    u32 ss_ctrl[ETM_MAX_SS_CMP];
    u32 ss_status[ETM_MAX_SS_CMP];
    u32 ss_pe_cmp[ETM_MAX_SS_CMP];
    u8 addr_idx;
    u64 addr_val[ETM_MAX_SINGLE_ADDR_CMP];
    u64 addr_acc[ETM_MAX_SINGLE_ADDR_CMP];
    u8 addr_type[ETM_MAX_SINGLE_ADDR_CMP];
    u8 ctxid_idx;
    u64 ctxid_pid[ETMv4_MAX_CTXID_CMP];
    u64 ctxid_vpid[ETMv4_MAX_CTXID_CMP];
    u32 ctxid_mask0;
    u32 ctxid_mask1;
    u8 vmid_idx;
    u64 vmid_val[ETM_MAX_VMID_CMP];
    u32 vmid_mask0;
    u32 vmid_mask1;
    u32 ext_inp;
};

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
    struct etmv4_config config;
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
    u32 control, reg;
    struct etmv4_drvdata *drvdata = (struct etmv4_drvdata *)info;

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

    reg = readl_relaxed(drvdata->base + 0x00C);
    while ((reg & 0x1) != 0x1)
    {
        printk(KERN_INFO "[ETM:] Waiting for disable to be idle state\n");
        reg = readl_relaxed(drvdata->base + 0x00C);
    }

    CS_LOCK(drvdata->base);

    dev_info(NULL, "cpu: %d disable smp call done\n", drvdata->cpu);
}

static void etm4_enable_hw(void *info)
{
    int i;
    struct etmv4_drvdata *drvdata = (struct etmv4_drvdata *)info;
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
    // val | 0x17
    // writel_relaxed(config->cfg, drvdata->base + TRCCONFIGR);
    writel_relaxed(readl_relaxed(drvdata->base + TRCCONFIGR) | 0x17, drvdata->base + TRCCONFIGR);
    /* nothing specific implemented */
    writel_relaxed(0x0, drvdata->base + TRCAUXCTLR);
    writel_relaxed(config->eventctrl0, drvdata->base + TRCEVENTCTL0R);
    writel_relaxed(config->eventctrl1, drvdata->base + TRCEVENTCTL1R);
    writel_relaxed(config->stall_ctrl, drvdata->base + TRCSTALLCTLR);
    writel_relaxed(config->ts_ctrl, drvdata->base + TRCTSCTLR);
    // 0x8
    writel_relaxed(config->syncfreq, drvdata->base + TRCSYNCPR);
    // 0x100
    writel_relaxed(config->ccctlr, drvdata->base + TRCCCCTLR);
    writel_relaxed(config->bb_ctrl, drvdata->base + TRCBBCTLR);
    //  ????  traceid: (CORESIGHT_ETM_PMU_SEED + (CPU_NUMBER * 2)): 0x10 + 0*2
    writel_relaxed(drvdata->trcid, drvdata->base + TRCTRACEIDR);
    // vinst_ctrl
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
    // start and stop
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
    // writel_relaxed(readl_relaxed(drvdata->base + TRCPDSR) | 0x1, drvdata->base + TRCPDSR);
    readl_relaxed(drvdata->base + TRCPDSR);
    /* Enable the trace unit */
    writel_relaxed(0x1, drvdata->base + TRCPRGCTLR);

    /* wait for TRCSTATR.IDLE to go back down to '0' */
    if (coresight_timeout(drvdata->base, TRCSTATR, TRCSTATR_IDLE_BIT, 0))
        dev_err(drvdata->dev,
                "timeout while waiting for Idle Trace Status\n");

    CS_LOCK(drvdata->base);

    dev_info(NULL, "cpu: %d enable smp call done\n", drvdata->cpu);
}

#endif
