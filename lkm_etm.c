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
#define BASE_FUNNEL_ADDR 0x20040000

#define ADDR_SIZE 1024
#define _DEBUG_LANRAN

uint32_t CSTF_OLD = 0x300;

static inline uint32_t system_read_helper(uint64_t base, uint64_t offset)
{
    uint32_t val = 0;
    void __iomem *virtual_addr = ioremap(base, offset + 8);
    val = ioread32(virtual_addr + offset);
    iounmap(virtual_addr);
    return val;
}

static inline void system_write_helper(uint64_t base, uint64_t offset, uint32_t new_val)
{
    void __iomem *virtual_addr = ioremap(base, offset + 8);
    iowrite32(new_val, virtual_addr + offset);
    iounmap(virtual_addr);
}

static inline uint32_t system_read_TRCVICTLR(void)
{
    return system_read_helper(BASE_ETM_ADDR, 0x080);
}

// control register, set 1 to enable
static inline uint32_t system_read_TRCPRGCTLR(void)
{
    return system_read_helper(BASE_ETM_ADDR, 0x004);
}

static inline uint32_t system_write_TRCPRGCTLR(void)
{
    return system_read_helper(BASE_ETM_ADDR, 0x004);
}

// funnel control register
// static inline uint32_t system_read_CSTF_CR(void)
// {
//     return system_read_helper(BASE_FUNNEL_ADDR, 0x0);
// }

static inline u_int32_t enable_CSTF_CR(void)
{
    uint32_t old_val;
    old_val = system_read_helper(BASE_FUNNEL_ADDR, 0x0);
    system_write_helper(BASE_FUNNEL_ADDR, 0x0, old_val | 1);
    CSTF_OLD = old_val;
#ifdef _DEBUG_LANRAN
    printk(KERN_INFO "[" DRVR_NAME "]"
                     "CSTF change: 0x%x -> 0x%x",
           old_val, old_val | 1);
#endif
    return old_val;
}

static inline void disable_CSTF_CR(void)
{

#ifdef _DEBUG_LANRAN
    uint32_t old_val;
    old_val = system_read_helper(BASE_FUNNEL_ADDR, 0x0);
#endif
    system_write_helper(BASE_FUNNEL_ADDR, 0x0, CSTF_OLD);
#ifdef _DEBUG_LANRAN
    printk(KERN_INFO "[" DRVR_NAME "]"
                     "CSTF change: 0x%x -> 0x%x",
           old_val, CSTF_OLD);
#endif
}

static inline uint32_t system_read_LAR(void)
{
    return system_read_helper(BASE_ETM_ADDR, 0xfb0);
}

static inline uint32_t system_read_OSLAR(void)
{
    return system_read_helper(BASE_ETM_ADDR, 0x300);
}

static inline void OS_UNLOCK(void)
{

    // uint32_t old_val = system_read_OSLAR();
    // system_write_helper(BASE_ETM_ADDR, 0x300, old_val & 0xfffffffe);

#ifdef _DEBUG_LANRAN
    // printk(KERN_INFO "[" DRVR_NAME "]"
    //                  "OSLAR change: 0x%x -> 0x%x",
    //        old_val, old_val & 0xfffffffe);
    printk(KERN_INFO "[" DRVR_NAME "]"
                     "LAR change: 0x%x -> 0x%x",
           system_read_LAR(), 0xc5acce55);
#endif
    system_write_helper(BASE_ETM_ADDR, 0xfb0, 0xc5acce55);
}

static inline void OS_LOCK(void)
{

    // uint32_t old_val = system_read_OSLAR();
    // system_write_helper(BASE_ETM_ADDR, 0x300, old_val | 1);

#ifdef _DEBUG_LANRAN
    // printk(KERN_INFO "[" DRVR_NAME "]"
    //                  "OSLAR change: 0x%x -> 0x%x",
    //        old_val, old_val | 1);
    printk(KERN_INFO "[" DRVR_NAME "]"
                     "LAR change: 0x%x -> 0x%x",
           system_read_LAR(), 0x0);
#endif
    system_write_helper(BASE_ETM_ADDR, 0xfb0, 0x0);
}

static int __init lkm_example_init(void)
{
    OS_UNLOCK();
    // uint32_t r = 0;
    printk(KERN_INFO "[" DRVR_NAME "]"
                     "Hello, World!\n");

    // r = system_read_OSLAR();
    enable_CSTF_CR();
    // printk(KERN_INFO "[" DRVR_NAME "]"
    //                  "trcprgctlr: 0x%0x\n",
    //        r);
    return 0;
}
static void __exit lkm_example_exit(void)
{

    disable_CSTF_CR();
    printk(KERN_INFO "[" DRVR_NAME "]"
                     "Goodbye, World!\n");
    OS_LOCK();
}
module_init(lkm_example_init);
module_exit(lkm_example_exit);