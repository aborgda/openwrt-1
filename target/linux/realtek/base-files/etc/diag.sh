#!/bin/sh
# Copyright (C) 2010-2013 OpenWrt.org

. /lib/functions.sh
. /lib/functions/leds.sh

get_status_led() {
	board=$(board_name)

	case $board in
	actionrg1200-v1)
		status_led="actionrg1200:blue:sys"
		;;
	actionrf1200-v1)
		status_led="actionrf1200:blue:sys"
		;;
	# Breaks device switch. Still needs fix before using it
	# re708-v1)
	# 	status_led="re708:green:wps"
	# 	;;
	# gwr1200ac-v1)
	# 	status_led="gwr1200ac:green:wps"
	# 	;;
	esac
}

set_state() {
	get_status_led $1

	case "$1" in
	preinit)
		status_led_blink_preinit
		;;
	failsafe)
		status_led_blink_failsafe
		;;
	upgrade | \
	preinit_regular)
		status_led_blink_preinit_regular
		;;
	done)
		status_led_on
		;;
	esac
}
