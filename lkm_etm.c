#define BASE_ETM_ADDR 0x22040000
#define BASE_PMU_ADDR 0x22030000
#define BASE_ETF_ADDR 0x20010000
#define BASE_FUNNEL_MAIN_ADDR 0x20040000
#define BASE_A72_FUNNEL_ADDR 0x220c0000

#define ADDR_SIZE 1024
#define BUF_SIZE 0x10001
// #define _DEBUG_LANRAN

#include "funnel.h"
#include "etf.h"
#include "etm.h"

struct memory_mapped_address
{
    struct funnel_drvdata a72_funnel_base_addr;
    struct funnel_drvdata main_funnel_base_addr;
    struct tmc_drvdata tmc_drvdata;
    struct etmv4_drvdata etm_drvdata;
} _default_addresses;

void map_addresses(void)
{
    _default_addresses.a72_funnel_base_addr.base = ioremap(BASE_A72_FUNNEL_ADDR, ADDR_SIZE);
    _default_addresses.main_funnel_base_addr.base = ioremap(BASE_FUNNEL_MAIN_ADDR, ADDR_SIZE);
    _default_addresses.tmc_drvdata.base = ioremap(BASE_ETF_ADDR, ADDR_SIZE);
    _default_addresses.etm_drvdata.base = ioremap(BASE_ETM_ADDR, ADDR_SIZE);
}

void unmap_address(void)
{
    iounmap(_default_addresses.a72_funnel_base_addr.base);
    iounmap(_default_addresses.main_funnel_base_addr.base);
    iounmap(_default_addresses.tmc_drvdata.base);
    iounmap(_default_addresses.etm_drvdata.base);
}

void init_config(void)
{
    u32 pid;
    _default_addresses.etm_drvdata.config.cfg = 0x17;
    _default_addresses.etm_drvdata.config.syncfreq = 0xC;
    _default_addresses.etm_drvdata.config.ccctlr = 0x100;
    _default_addresses.etm_drvdata.config.viiectlr = 0x1;
    _default_addresses.etm_drvdata.trcid = 0x10;
    _default_addresses.etm_drvdata.config.addr_val[0] = 0x0;
    _default_addresses.etm_drvdata.config.addr_acc[0] = 0x6B04;
    _default_addresses.etm_drvdata.config.addr_val[1] = ~0x0;
    _default_addresses.etm_drvdata.config.addr_acc[1] = 0x6B04;
    _default_addresses.etm_drvdata.nr_addr_cmp = 2;

    pid = get_trace_pid();
    // process id
    printk("[ETM:] pid: %u\n", pid);
    _default_addresses.etm_drvdata.config.ctxid_pid[0] = pid;
    _default_addresses.etm_drvdata.numcidc = 1;
    _default_addresses.etm_drvdata.config.vinst_ctrl = 0xf0201;

    // etf relavant
    _default_addresses.tmc_drvdata.trigger_cntr = 0x1000;
    _default_addresses.tmc_drvdata.buf = kmalloc(BUF_SIZE, GFP_KERNEL);
    _default_addresses.tmc_drvdata.memwidth = BUF_SIZE / 4;
}

static int __init lkm_etm_init(void)
{
    map_addresses();
    init_config();

    funnel_enable_hw(&_default_addresses.a72_funnel_base_addr, 0);
    funnel_enable_hw(&_default_addresses.main_funnel_base_addr, 0);
    tmc_etb_enable_hw(&_default_addresses.tmc_drvdata);

    etm4_enable_hw(&_default_addresses.etm_drvdata);

    // check_mem(_default_addresses.etm_drvdata.base, "/sdcard/Download/mem_check/myetm.out");

    return 0;
}

static void __exit lkm_etm_exit(void)
{
    etm4_disable_hw(&_default_addresses.etm_drvdata);
    tmc_etb_disable_hw(&_default_addresses.tmc_drvdata);
    // save_to_file(&_default_addresses.tmc_drvdata);

    funnel_disable_hw(&_default_addresses.a72_funnel_base_addr, 0);
    funnel_disable_hw(&_default_addresses.main_funnel_base_addr, 0);

    kfree(_default_addresses.tmc_drvdata.buf);
    unmap_address();
}
module_init(lkm_etm_init);
module_exit(lkm_etm_exit);