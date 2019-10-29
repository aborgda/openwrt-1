
#ifndef __REALTEK_MEMMAP__
#define __REALTEK_MEMMAP__

extern __iomem void *_sys_membase;
extern __iomem void *_timer_membase;
extern __iomem void *_intc_membase;

// System registers
#define REALTEK_SR_REG_ID			0x00
#define REALTEK_SR_REG_BOOTSTRAP	0x08
#define REALTEK_SR_BS_40MHZ			BIT(24) // Crystal clock at 40Mhz or 25Mhz


// Interrupt Registers and bits
#define REALTEK_IC_REG_MASK			0x00
#define REALTEK_IC_REG_STATUS		0x04
#define REALTEK_IC_REG_IRR0			0x08
#define REALTEK_IC_REG_IRR1			0x0C
#define REALTEK_IC_REG_IRR2			0x10
#define REALTEK_IC_REG_IRR3			0x14

// Timer Registers and bits
#define REALTEK_TC_REG_DATA0		0x00
#define REALTEK_TC_REG_DATA1		0x04
#define REALTEK_TC_REG_COUNT0		0x08
#define REALTEK_TC_REG_COUNT1		0x0c
#define REALTEK_TC_REG_CTRL			0x10
#define REALTEK_TC_CTRL_TC0_EN		BIT(31)	// Enable Timer0
#define REALTEK_TC_CTRL_TC0_MODE	BIT(30)	// 0 Counter, 1 Timer
#define REALTEK_TC_CTRL_TC1_EN		BIT(29)	// Enable Timer1
#define REALTEK_TC_CTRL_TC1_MODE	BIT(28) // 0 Counter, 1 Timer
#define REALTEK_TC_REG_IR			0x14
#define REALTEK_TC_IR_TC0_EN		BIT(31) // Enable Timer Interrupts
#define REALTEK_TC_IR_TC1_EN		BIT(30)
#define REALTEK_TC_IR_TC0_PENDING	BIT(29)
#define REALTEK_TC_IR_TC1_PENDING	BIT(28)
#define REALTEK_TC_REG_CLOCK_DIV	0x18	// Clock Divider N TimerClock=(BaseClock/N)


#define sr_w32(val, reg) __raw_writel(val, _sys_membase + reg)
#define sr_r32(reg)      __raw_readl(_sys_membase + reg)

#define tc_w32(val, reg) __raw_writel(val, _timer_membase + reg)
#define tc_r32(reg)		 __raw_readl(_timer_membase + reg)

#define ic_w32(val, reg) __raw_writel(val, _intc_membase + reg)
#define ic_r32(reg)      __raw_readl(_intc_membase + reg)

#endif