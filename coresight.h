#ifndef _CORESIGHT_LANRAN_H
#define _CORESIGHT_LANRAN_H

#include <linux/types.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/io.h>

#define CORESIGHT_LAR 0xfb0
#define CORESIGHT_LSR 0xfb4
#define CORESIGHT_UNLOCK 0xc5acce55

#define TIMEOUT_US 100
/**
 * coresight_timeout - loop until a bit has changed to a specific state.
 * @addr: base address of the area of interest.
 * @offset: address of a register, starting from @addr.
 * @position: the position of the bit of interest.
 * @value: the value the bit should have.
 *
 * Return: 0 as soon as the bit has taken the desired state or -EAGAIN if
 * TIMEOUT_US has elapsed, which ever happens first.
 */

int coresight_timeout(void __iomem *addr, u32 offset, int position, int value)
{
    int i;
    u32 val;

    for (i = TIMEOUT_US; i > 0; i--)
    {
        val = __raw_readl(addr + offset);
        /* waiting on the bit to go from 0 to 1 */
        if (value)
        {
            if (val & BIT(position))
                return 0;
            /* waiting on the bit to go from 1 to 0 */
        }
        else
        {
            if (!(val & BIT(position)))
                return 0;
        }

        /*
		 * Delay is arbitrary - the specification doesn't say how long
		 * we are expected to wait.  Extra check required to make sure
		 * we don't wait needlessly on the last iteration.
		 */
        if (i - 1)
            udelay(1);
    }

    return -EAGAIN;
}

static inline void CS_UNLOCK(void __iomem *addr)
{
    do
    {
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
        writel_relaxed(0x0, addr + CORESIGHT_LAR);
    } while (0);
}

#endif