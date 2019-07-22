# 
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Build/tbs_dlink
  $(STAGING_DIR_HOST)/bin/tbs_dlink $@
endef

define Device/Default
  PROFILES = Default $$(DEV_PROFILE)
endef

define Device/DIR815D1
  DEV_PROFILE := DIR815D1
  IMAGE_SIZE := 7936k
  DEVICE_TITLE := DIR 815 D1
  KERNEL := kernel-bin | lzma
  IMAGES := factory.bin
  IMAGE/factory.bin := append-kernel | pad-to 1900544 | append-rootfs | pad-rootfs | tbs_dlink -b
endef
TARGET_DEVICES += DIR815D1

define Image/Build/Initramfs
        $(call Image/Build/Profile/$(PROFILE),initramfs)
endef

$(eval $(call BuildImage))
