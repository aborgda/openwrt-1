/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Copyright (C) 2019 Gaspare Bruno <gaspare@anlix.io>
 *
 * RTL 8197d SoC irq specific definitions
 *
 */

#include <linux/io.h>
#include <linux/bitops.h>

#include "realtek_mem.h"
#include "irq.h"

#define INTC_TC0   BIT(8)
#define INTC_UART0 BIT(12)

u32 realtek_soc_irq_init(void) 
{
	// IRR0
	ic_w32((0), 
		REALTEK_IC_REG_IRR0);

	// IRR1
	ic_w32((REALTEK_IRQ_TIMER << 0  | 
			REALTEK_IRQ_UART0 << 16 ), 
		REALTEK_IC_REG_IRR1);

	// IRR2
	ic_w32((0), 
		REALTEK_IC_REG_IRR2);

	// IRR3
	ic_w32((0), 
		REALTEK_IC_REG_IRR3);

	return INTC_TC0;
}
