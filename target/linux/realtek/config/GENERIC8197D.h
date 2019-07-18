
#define RTL819X_BUTTONS_POLL_INTERVAL   100 // orignal is 20 , fine-tune to 100
#define RTL819X_BUTTONS_DEBOUNCE_INTERVAL   3*RTL819X_BUTTONS_POLL_INTERVAL
#define RTL_MACHINE_NAME "GENERIC"

static struct gpio_led rtl819xd_leds_gpio[] __initdata = {

	{
		.name		= "rtl819x:green:wps",
		.gpio		= BSP_GPIO_PIN_A6,
		.active_low	= 1,
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
	,
 	{
		.desc		= "wps",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = RTL819X_BUTTONS_DEBOUNCE_INTERVAL,
		.gpio		= BSP_GPIO_PIN_A3,
		.active_low	= 1,
	}

};

