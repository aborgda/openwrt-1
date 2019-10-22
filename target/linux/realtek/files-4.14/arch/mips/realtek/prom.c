#include <linux/init.h>
#include <asm/setup.h>

#ifdef CONFIG_SOC_RTL8197F
#define REALTEK_UART0_BASE	0xB8147000
#else
#define REALTEK_UART0_BASE	0xB8002000
#endif

void __init prom_init(void)
{
	setup_8250_early_printk_port((unsigned long)REALTEK_UART0_BASE, 2, 30000);
}

void __init prom_free_prom_memory(void)
{
}
