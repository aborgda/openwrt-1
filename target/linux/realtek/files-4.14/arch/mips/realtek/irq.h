/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Copyright (C) 2019 Gaspare Bruno <gaspare@anlix.io>
 *
 * 
 */

#ifndef __REALTEK_IRQ__
#define __REALTEK_IRQ__

#define REALTEK_INTC_IRQ_COUNT	32

#define REALTEK_IRQ_GENERIC		2   

#define REALTEK_IRQ_TIMER		7
#define REALTEK_IRQ_UART0		REALTEK_IRQ_GENERIC   

u32 realtek_soc_irq_init(void);

#endif