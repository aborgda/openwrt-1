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

