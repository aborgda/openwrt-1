#
# Copyright (C) 2008-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mips
BOARD:=rlx
BOARDNAME:=Realtek
SUBTARGETS:=rtl8197d rtl8196e
FEATURES:=squashfs gpio
MAINTAINER:=Anlix <contato@anlix.io>

KERNEL_PATCHVER:=3.18

define Target/Description
	Build firmware images for Realtek based boards.
endef

include $(INCLUDE_DIR)/target.mk

$(eval $(call BuildTarget))
