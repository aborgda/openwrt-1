
#ifndef __REALTEK_MEMMAP__
#define __REALTEK_MEMMAP__

extern __iomem void *_sys_membase;
extern __iomem void *_timer_membase;
extern __iomem void *_intc_membase;

#define sr_w32(val, reg) __raw_writel(val, _sys_membase + reg)
#define sr_r32(reg)      __raw_readl(_sys_membase + reg)

#define tr_w32(val, reg) __raw_writel(val, _timer_membase + reg)
#define tr_r32(reg)		 __raw_readl(_timer_membase + reg)

#define ir_w32(val, reg) __raw_writel(val, _intc_membase + reg)
#define ir_r32(reg)      __raw_readl(_intc_membase + reg)

#endif