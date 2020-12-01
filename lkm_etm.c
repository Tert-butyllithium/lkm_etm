#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huana Liu");
MODULE_DESCRIPTION("A Linux kenrel module for tracing using ETM");
MODULE_VERSION("0.01");

#define DRVR_NAME "test_etm_register"
#define BASE_ETM_ADDR 0x22040000
#define BASE_PMU_ADDR 0x22030000
#define BASE_ETF_ADDR 0x20010000
#define BASE_FUNNEL_MAIN_ADDR 0x20040000
#define BASE_A72_FUNNEL_ADDR 0x220c0000

#define ADDR_SIZE 1024
#define _DEBUG_LANRAN

#include "funnel.h"
#include "etf.h"
#include "etm.h"

static void gao(void *args)
{
    char *mem = kmalloc(10, GFP_KERNEL);
    kfree(mem);
}

// uint32_t CSTF_OLD = 0x300;

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
    _default_addresses.etm_drvdata.config.cfg = 0x17;
    _default_addresses.etm_drvdata.config.syncfreq = 0x8;
    _default_addresses.etm_drvdata.config.ccctlr = 0x100;
    _default_addresses.etm_drvdata.trcid = 0x10;
    _default_addresses.etm_drvdata.config.addr_acc[0] = 0x5000;
    _default_addresses.etm_drvdata.config.addr_val[1] = 0xffffffff;
    _default_addresses.etm_drvdata.config.addr_acc[1] = 0x5000;
    _default_addresses.etm_drvdata.config.vinst_ctrl = 0xf0201;
}

char buffer[1024];
static void check_mem(void __iomem *base, char *path)
{
    mm_segment_t old_fs;
    int i = 0;
    struct file *file = filp_open(path, O_RDWR | O_APPEND | O_CREAT, 0644);
    if (IS_ERR(file))
    {
        printk(KERN_INFO "[ETM:]error occured while opening file %s, exiting...\n", path);
        return;
    }
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    for (; i < 1024; i += 4)
    {
        sprintf(buffer, "address 0x%x: 0x%x\n", i, ioread32(base + i));
        file->f_op->write(file, (char *)buffer, strlen(buffer), &file->f_pos);
    }

    set_fs(old_fs);
    filp_close(file, NULL);
    file = NULL;
}

static int __init lkm_etm_init(void)
{
    map_addresses();
    funnel_enable_hw(&_default_addresses.a72_funnel_base_addr, 0);
    funnel_enable_hw(&_default_addresses.main_funnel_base_addr, 0);
    tmc_etf_enable_hw(&_default_addresses.tmc_drvdata);
    init_config();
    etm4_enable_hw(&_default_addresses.etm_drvdata);

    check_mem(_default_addresses.etm_drvdata.base, "/sdcard/Download/mem_check/myetm.out");

    // do something
    smp_call_function_single(0, &gao, NULL, 1);

    // -------------
    etm4_disable_hw(&_default_addresses.etm_drvdata);
    tmc_etf_disable_hw(&_default_addresses.tmc_drvdata);
    tmc_eft_retrieve(&_default_addresses.tmc_drvdata);

    funnel_disable_hw(&_default_addresses.a72_funnel_base_addr, 0);
    funnel_disable_hw(&_default_addresses.main_funnel_base_addr, 0);

    unmap_address();
    return 0;
}

static void __exit lkm_etm_exit(void)
{
}
module_init(lkm_etm_init);
module_exit(lkm_etm_exit);