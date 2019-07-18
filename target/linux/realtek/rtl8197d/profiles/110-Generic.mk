#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/GENERIC8197D
  NAME:=Generic RTL8197D
  PACKAGES:=-wpad-mini
endef

define Profile/GENERIC8197D/Description
        Realtek RTL8197D SOC
endef

$(eval $(call Profile,GENERIC8197D))
