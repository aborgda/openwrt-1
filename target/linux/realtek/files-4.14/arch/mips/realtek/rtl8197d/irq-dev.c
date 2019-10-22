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

#include "../realtek_mem.h"
#include "../irq.h"

#define INTC_TC0   BIT(8)
#define INTC_UART0 BIT(12)

u32 realtek_soc_irq_init(void) 
{
	ir_w32((REALTEK_IRQ_TIMER << 0  | 
			      REALTEK_IRQ_UART0 << 16 ), 
	        0x8);
	        
	ir_w32((0), 
	        0xC);

	ir_w32((0), 
	     	0x10);

	ir_w32((0), 
	     	0x14);

	return INTC_TC0;
}
