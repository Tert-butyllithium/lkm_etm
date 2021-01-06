#ifndef _KERNEL_REPLACEMENT_LANRAN_H
#define _KERNEL_REPLACEMENT_LANRAN_H

// linux kenrel types
#define u8 unsigned char
#define u32 unsigned int
#define u64 unsigned long long
#define spinlock_t u8
#define local_t u8
#define bool u8
#define true 1
#define false 0
#define __iomem
#define __init
#define __exit
#define KERN_INFO 
#define printk ERROR
#define dev_info(dev, fmt, ...)						\
	ERROR(fmt, ##__VA_ARGS__)

//read and write
#define writel_relaxed(v, c) ((void)__raw_writel((u32)(v), (c)))
#define writeq_relaxed(v, c) ((void)__raw_writeq((u64)(v), (c)))
#define readl_relaxed(c) ({ u32 __r = (__raw_readl(c)); __r; })
#define readq_relaxed(c) ({ u64 __r = (__raw_readq(c)); __r; })

#define __raw_writel __raw_writel
static inline void __raw_writel(u32 val, volatile void __iomem *addr)
{
    asm volatile("str %w0, [%1]"
                 :
                 : "rZ"(val), "r"(addr));
}

#define __raw_writeq __raw_writeq
static inline void __raw_writeq(u64 val, volatile void __iomem *addr)
{
    asm volatile("str %x0, [%1]"
                 :
                 : "rZ"(val), "r"(addr));
}

#define __raw_readl __raw_readl
static inline u32 __raw_readl(const volatile void __iomem *addr)
{
    u32 val;
    asm volatile(
        "ldar %w0, [%1]"
        : "=r"(val)
        : "r"(addr));
    return val;
}

#define __raw_readq __raw_readq
static inline u64 __raw_readq(const volatile void __iomem *addr)
{
    u64 val;
    asm volatile("ldar %0, [%1]"
                 : "=r"(val)
                 : "r"(addr));
    return val;
}

// other things
#define EAGAIN 11
#define GFP_KERNEL 0

u32 get_trace_pid()
{
    // try to access register?
    // access register x0
}

void *kmalloc(u32 _,u32 __){
    return  0x0;
}

#endif