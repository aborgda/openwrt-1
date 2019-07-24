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

	return 0;
}

rtk_pre_upgrade () {
        echo "- install_ram_libs -"
        ramlib="$RAM_ROOT/lib"
        mkdir -p "$ramlib"
        cp /lib/ld-uClibc.so.0 $ramlib
        cp /lib/libc.so.0 $ramlib
        cp /lib/libgcc_s.so.1 $ramlib
        cp /lib/libubox.so $ramlib
	cp /lib/libcrypt.so.0 $ramlib
	cp /lib/libm.so.0 $ramlib
}

append sysupgrade_pre_upgrade rtk_pre_upgrade
