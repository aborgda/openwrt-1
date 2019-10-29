#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/bootmem.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>

#include <asm/reboot.h>
#include <asm/io.h>
#include <asm/prom.h>
#include <asm/setup.h>
#include <asm/bootinfo.h>
#include <asm/addrspace.h>
#include <asm/idle.h>

#include <linux/clk-provider.h>
#include <linux/clocksource.h>

#include "realtek_mem.h"

#define REALTEK_WATCHDOG_TIMER_REG	0x1C

const char *get_system_type(void)
{
#ifdef CONFIG_SOC_RTL8196E
	return "Realtek RTL8196E";
#endif

#ifdef CONFIG_SOC_RTL8197D
	return "Realtek RTL8197D";
#endif

#ifdef CONFIG_SOC_RTL8197F
	return "Realtek RTL8197F";
#endif
}

static inline void wait_instruction(void)
{
	__asm__(
	"       .set    push            \n"
	"       sleep                   \n"
	"       .set    pop             \n");
}

void realtek_machine_restart(char *command)
{
	/* Disable all interrupts */
	local_irq_disable();

	/* Use watchdog to reset the system */
	tc_w32(0x00, REALTEK_WATCHDOG_TIMER_REG);

	for (;;)
		wait_instruction();
}

void realtek_wait(void)
{
	if (!need_resched())
		wait_instruction();
	local_irq_enable();
}

void realtek_halt(void)
{
	while (1)
		wait_instruction();
}

void __init plat_mem_setup(void)
{
	void *dtb = NULL;

	_machine_restart = realtek_machine_restart;
	_machine_halt = realtek_halt;
	cpu_wait = realtek_wait;

	// Initialize DTB
	if (fw_passed_dtb)
		dtb = (void *)fw_passed_dtb;
	else if (__dtb_start != __dtb_end)
		dtb = (void *)__dtb_start;

	__dt_setup_arch(dtb);
}

__iomem void *_sys_membase;
__iomem void *_timer_membase;
__iomem void *_intc_membase;

__iomem void *of_mapmem(const char *node)
{
	struct device_node *np;
	struct resource res;

	np = of_find_compatible_node(NULL, NULL, node);
	if (!np)
		panic("Failed to find %s node", node);

	if (of_address_to_resource(np, 0, &res))
		panic("Failed to get resource for %s", node);

	return ioremap_nocache(res.start, resource_size(&res));
}

void __init device_tree_init(void)
{
	unflatten_and_copy_device_tree();

	_sys_membase = of_mapmem("realtek,rtl819x-sysc");
	if(!_sys_membase)
		panic("Failed to map memory for rtl819x-sysc");

	_timer_membase = of_mapmem("realtek,rtl819x-timer");
	if(!_timer_membase)
		panic("Failed to map memory for rtl819x-timer");

	_intc_membase = of_mapmem("realtek,rtl819x-intc");
	if(!_intc_membase)
		panic("Failed to map memory for rtl819x-intc");

	pr_info("BOOTSTRAP = %x %x %x %x\n", sr_r32(0x00), sr_r32(0x04), sr_r32(0x08), sr_r32(0x10));
}

void __init plat_time_init(void)
{
	of_clk_init(NULL);
	timer_probe();
}

