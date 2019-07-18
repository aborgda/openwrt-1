#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/DIR815D1
  NAME:=D-Link DIR-815 D1
  PACKAGES:=-wpad-mini
endef

define Profile/DIR815D1/Description
        D-Link DIR-815 version D1
endef

$(eval $(call Profile,DIR815D1))
