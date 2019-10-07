#
# Copyright (C) 2011 OpenWrt.org
#

. /lib/functions/system.sh
. /lib/realtek.sh

PART_NAME=firmware

platform_do_upgrade() {
	default_do_upgrade "$ARGV"
}

platform_check_image() {
	[ "$ARGC" -gt 1 ] && return 1
	return 0
}

rtk_pre_upgrade () {
	local board=$(board_name)

	case "$board" in
	gwr-300n-v1|\
	re172-v1)
		echo "- install_ram_libs -"
		ramlib="$RAM_ROOT/lib"
		mkdir -p "$ramlib"
		cp /lib/ld-uClibc.so.0 $ramlib
		cp /lib/libc.so.0 $ramlib
		cp /lib/libgcc_s.so.1 $ramlib
		cp /lib/libubox.so $ramlib
		cp /lib/libcrypt.so.0 $ramlib
		cp /lib/libm.so.0 $ramlib
		;;
	*)
		echo "- no libs to install -"
		;;
	esac
}

append sysupgrade_pre_upgrade rtk_pre_upgrade
