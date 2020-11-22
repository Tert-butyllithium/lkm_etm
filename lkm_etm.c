#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huana Liu");
MODULE_DESCRIPTION("A Linux kenrel module for tracing using ETM");
MODULE_VERSION("0.01");

#define DRVR_NAME "test_etm_register"
#define BASE_ETM_ADDR 0x22040000
#define BASE_PMU_ADDR 0x22030000
#define BASE_FUNNEL_MAIN_ADDR 0x20040000
#define BASE_A72_FUNNEL_ADDR 0x220c0000

#define ADDR_SIZE 1024
#define _DEBUG_LANRAN

#include "funnel.h"

// uint32_t CSTF_OLD = 0x300;

struct memory_mapped_address
{
    struct funnel_drvdata a72_funnel_base_addr;
    struct funnel_drvdata main_funnel_base_addr;
} _default_addresses;

void map_addresses(void)
{
    _default_addresses.a72_funnel_base_addr.base = ioremap(BASE_A72_FUNNEL_ADDR, ADDR_SIZE);
    _default_addresses.main_funnel_base_addr.base = ioremap(BASE_FUNNEL_MAIN_ADDR, ADDR_SIZE);
}

void unmap_address(void)
{
    iounmap(_default_addresses.a72_funnel_base_addr.base);
    iounmap(_default_addresses.main_funnel_base_addr.base);
}

static int __init lkm_example_init(void)
{
    map_addresses();
    funnel_enable_hw(&_default_addresses.a72_funnel_base_addr, 0);
    funnel_enable_hw(&_default_addresses.main_funnel_base_addr, 0);
    return 0;
}

static void __exit lkm_example_exit(void)
{
    funnel_disable_hw(&_default_addresses.a72_funnel_base_addr, 0);
    funnel_disable_hw(&_default_addresses.main_funnel_base_addr, 0);
    unmap_address();
}
module_init(lkm_example_init);
module_exit(lkm_example_exit);