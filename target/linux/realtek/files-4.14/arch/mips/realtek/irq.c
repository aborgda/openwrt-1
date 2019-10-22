/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Copyright (C) 2019 Gaspare Bruno <gaspare@anlix.io>
 *
 * Realtek IRQ handler
 * This IRQ driver have the 8 common MIPS CPU irqs
 * RLX driver have an aditional 8 hardware irqs
 * TODO: Implement the RLX additional irqs
 */

#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>

#include <asm/irq_cpu.h>
#include <asm/mipsregs.h>

#include "irq.h"
#include "realtek_mem.h"

static u32 mips_chip_irqs;

#define REALTEK_INTC_IRQ_BASE 8

#define REALTEK_INTCTL_REG_MASK			0x00
#define REALTEK_INTCTL_REG_STATUS		0x04

static void realtek_soc_irq_unmask(struct irq_data *d)
{
	unsigned int irq = d->irq - REALTEK_INTC_IRQ_BASE;
	u32 t;

	t = ir_r32(REALTEK_INTCTL_REG_MASK);
	ir_w32(t | BIT(irq), REALTEK_INTCTL_REG_MASK);
}

static void realtek_soc_irq_mask(struct irq_data *d)
{
	unsigned int irq = d->irq - REALTEK_INTC_IRQ_BASE;
	u32 t;

	t = ir_r32(REALTEK_INTCTL_REG_MASK);
	ir_w32(t & ~BIT(irq), REALTEK_INTCTL_REG_MASK);
}

static struct irq_chip realtek_soc_irq_chip = {
	.name			= "SOC",
	.irq_unmask 	= realtek_soc_irq_unmask,
	.irq_mask 		= realtek_soc_irq_mask,
};

static void realtek_soc_irq_handler(struct irq_desc *desc)
{
	u32 pending;
	struct irq_domain *domain;

	pending = ir_r32(REALTEK_INTCTL_REG_MASK) &
			  ir_r32(REALTEK_INTCTL_REG_STATUS);

	if (pending & mips_chip_irqs) {
		/*
		 * interrupts routed to mips core found here
		 * clear these bits as they can't be handled here
		 */
		ir_w32(mips_chip_irqs, REALTEK_INTCTL_REG_STATUS);
		pending &= ~mips_chip_irqs;

		if (!pending)
		        return;
	}

	if (!pending) {
		spurious_interrupt();
		return;
	}

	domain = irq_desc_get_handler_data(desc);
	while (pending) {
		int bit = __ffs(pending);
		generic_handle_irq(irq_find_mapping(domain, REALTEK_INTC_IRQ_BASE + bit));
		pending &= ~BIT(bit);
	}
}

static int intc_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hw)
{
	irq_set_chip_and_handler(irq, &realtek_soc_irq_chip, handle_level_irq);

	return 0;
}

static const struct irq_domain_ops irq_domain_ops = {
	.xlate = irq_domain_xlate_onecell,
	.map = intc_map,
};

asmlinkage void plat_irq_dispatch(void)
{
	unsigned long pending;

	pending = read_c0_status() & read_c0_cause() & ST0_IM;

	if (pending & STATUSF_IP7)
		do_IRQ(REALTEK_IRQ_TIMER);

	else if (pending & STATUSF_IP2)
		do_IRQ(REALTEK_IRQ_GENERIC);

	else
		spurious_interrupt();
}

static int __init intc_of_init(struct device_node *node,
			       struct device_node *parent)
{
	struct irq_domain *domain;

	// Reset all interrupts
	ir_w32(0, REALTEK_INTCTL_REG_MASK);

	// Map Interrupts according to SoC
	mips_chip_irqs = realtek_soc_irq_init();

	domain = irq_domain_add_legacy(node, REALTEK_INTC_IRQ_COUNT,
			REALTEK_INTC_IRQ_BASE, 0, &irq_domain_ops, NULL);
	if (!domain)
		panic("Failed to add irqdomain");

	irq_set_chained_handler_and_data(REALTEK_IRQ_GENERIC, realtek_soc_irq_handler, domain);

	return 0;
}

static struct of_device_id __initdata of_irq_ids[] = {
	{ .compatible = "mti,cpu-interrupt-controller", .data = mips_cpu_irq_of_init },
	{ .compatible = "realtek,rtl819x-intc", .data = intc_of_init },
	{},
};

void __init arch_init_irq(void)
{
	of_irq_init(of_irq_ids);
}

