/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2019 - Gaspare Bruno <gaspare@anlix.io>
 *
 *   Realtek RTL8196E and RTL8197D have two clocks of 28 bits
 */

#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/reset.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include "realtek_mem.h"

#define REALTEK_TC_REG_DATA0		0x00
#define REALTEK_TC_REG_CTRL			0x10
#define REALTEK_TC_REG_IR			0x14
#define REALTEK_TC_REG_CLOCK_DIV	0x18

#define REALTEK_TC_IR_TC0_EN		BIT(31)
#define REALTEK_TC_IR_TC0_PENDING	BIT(29)

#define REALTEK_TC_CTRL_TC0_EN		BIT(31)	// Enable Timer 0
#define REALTEK_TC_CTRL_TC0_MODE	BIT(30)	// 0 Counter, 1 Timer
#define REALTEK_TC_CTRL_TC1_EN		BIT(29)	// Enable Timer 1
#define REALTEK_TC_CTRL_TC1_MODE	BIT(28) // 0 Counter, 1 Timer

// Only the 28 higher bits are valid in the timer register counter
#define RTLADJ_TICK(x)  (x<<4) 

static int rtl819x_set_state_shutdown(struct clock_event_device *cd)
{
	u32 val;

	// Disable Timer 
	val = tr_r32(REALTEK_TC_REG_CTRL);
	val &= ~(REALTEK_TC_CTRL_TC0_EN);
	tr_w32(val, REALTEK_TC_REG_CTRL);

	// Disable Interrupts
	val = tr_r32(REALTEK_TC_REG_IR);
	val &= ~REALTEK_TC_IR_TC0_EN;
	tr_w32(val, REALTEK_TC_REG_IR);
	return 0;
}

static int rtl819x_set_state_oneshot(struct clock_event_device *cd)
{
	u32 val;

	// Disable Timer and Set as Counter (It will be auto enabled)
	val = tr_r32(REALTEK_TC_REG_CTRL);
	val &= ~(REALTEK_TC_CTRL_TC0_EN | REALTEK_TC_CTRL_TC0_MODE);
	tr_w32(val, REALTEK_TC_REG_CTRL);

	// Enable Interrupts
	val = tr_r32(REALTEK_TC_REG_IR);
	val |= REALTEK_TC_IR_TC0_EN | REALTEK_TC_IR_TC0_PENDING;
	tr_w32(val, REALTEK_TC_REG_IR);
	return 0;
}

static int rtl819x_timer_set_next_event(unsigned long delta, struct clock_event_device *evt)
{
	u32 val;

	// Disable Timer
	val = tr_r32(REALTEK_TC_REG_CTRL);
	val &= ~REALTEK_TC_CTRL_TC0_EN;
	tr_w32(val, REALTEK_TC_REG_CTRL);

	tr_w32(RTLADJ_TICK(delta), REALTEK_TC_REG_DATA0);

	// Reenable Timer
	val |= REALTEK_TC_CTRL_TC0_EN;
	tr_w32(val, REALTEK_TC_REG_CTRL);

	return 0;
}

static irqreturn_t rtl819x_timer_interrupt(int irq, void *dev_id)
{
	u32 tc0_irs;
	struct clock_event_device *cd = dev_id;

	/* TC0 interrupt acknowledge */
	tc0_irs = tr_r32(REALTEK_TC_REG_IR);
	tc0_irs |= REALTEK_TC_IR_TC0_PENDING;
	tr_w32(tc0_irs, REALTEK_TC_REG_IR);

	cd->event_handler(cd);

	return IRQ_HANDLED;
}

static struct clock_event_device rtl819x_clockevent = {
	.rating			= 100,
	.features		= CLOCK_EVT_FEAT_ONESHOT,
	.set_next_event = rtl819x_timer_set_next_event,
	.set_state_oneshot = rtl819x_set_state_oneshot,
	.set_state_shutdown = rtl819x_set_state_shutdown,
};

static struct irqaction rtl819x_timer_irqaction = {
	.handler = rtl819x_timer_interrupt,
	.flags = IRQF_TIMER,
	.dev_id = &rtl819x_clockevent,
};

static int __init rtl819x_timer_init(struct device_node *np)
{
	unsigned long timer_rate;
	u32 div_fac;

	rtl819x_clockevent.name = np->name;
	rtl819x_timer_irqaction.name = np->name;

	rtl819x_clockevent.irq = irq_of_parse_and_map(np, 0);
	rtl819x_clockevent.cpumask = cpumask_of(0);

	timer_rate = 200000000/8000; // 25Mhz
	div_fac = 8000;

	// Higher 16 bits are the divider factor
	tr_w32(div_fac<<16, REALTEK_TC_REG_CLOCK_DIV);

	clockevents_config_and_register(&rtl819x_clockevent, timer_rate, 0x300, 0x7fffffff);

	setup_irq(rtl819x_clockevent.irq, &rtl819x_timer_irqaction);

	pr_info("%s: running - mult: %d, shift: %d, IRQ: %d\n",
		np->name, rtl819x_clockevent.mult, rtl819x_clockevent.shift, rtl819x_clockevent.irq);

	return 0;
}

TIMER_OF_DECLARE(rtl819x_timer, "realtek,rtl819x-timer", rtl819x_timer_init);
