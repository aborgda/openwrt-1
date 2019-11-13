
#ifndef __MACH_RTL_OPENWRT__
#define __MACH_RTL_OPENWRT__

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include "bspchip.h"
#include "dev_leds_gpio.h"
#include "dev-gpio-buttons.h"

#define RTL819X_BUTTONS_POLL_INTERVAL   100 // orignal is 20 , fine-tune to 100
#define RTL819X_BUTTONS_DEBOUNCE_INTERVAL   3*RTL819X_BUTTONS_POLL_INTERVAL
#define RTL_MACHINE_NAME "Multilaser RE172 V1"

static struct gpio_led rtl819xd_leds_gpio[] __initdata = {
	{
                .name           = "rtl819x:green:wps",
                .gpio           = BSP_GPIO_PIN_A6,
                .active_low     = 1,
        },
        {
                .name           = "rtl819x:green:lan1",
                .gpio           = BSP_GPIO_PIN_B2,
                .active_low     = 1,
        },
        {
                .name           = "rtl819x:green:lan2",
                .gpio           = BSP_GPIO_PIN_B3,
                .active_low     = 1,
        },
        {
                .name           = "rtl819x:green:lan3",
                .gpio           = BSP_GPIO_PIN_B4,
                .active_low     = 1,
        },
        {
                .name           = "rtl819x:green:lan4",
                .gpio           = BSP_GPIO_PIN_B5,
                .active_low     = 1,
        },
        {
                .name           = "rtl819x:green:wan",
                .gpio           = BSP_GPIO_PIN_B6,
                .active_low     = 1,
        },
};

static struct gpio_keys_button rtl819xd_buttons[] __initdata = { 
	{
		.desc		= "reset",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = RTL819X_BUTTONS_DEBOUNCE_INTERVAL,
		.gpio		= BSP_GPIO_PIN_A5,
		.active_low	= 1,
	}
};

static struct mtd_partition rtl8196_parts1[] = {
       {name: "boot", offset: 0, size: 0x10000, mask_flags: MTD_WRITEABLE,},
       {name: "firmware", offset: 0x10000, size: 0x3E0000,},
};

#endif
