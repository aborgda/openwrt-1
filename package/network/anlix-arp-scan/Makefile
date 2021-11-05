#
# Copyright (C) 2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=anlix-arp-scan
PKG_VERSION:=1.9.7
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/arp-scan-$(PKG_VERSION)
PKG_SOURCE_URL:=https://codeload.github.com/royhills/arp-scan/tar.gz/$(PKG_VERSION)?
PKG_HASH:=e03c36e4933c655bd0e4a841272554a347cd0136faf42c4a6564059e0761c039

PKG_MAINTAINER:=Sergey Urushkin <urusha.v1.0@gmail.com>
PKG_LICENSE:=GPL-3.0

PKG_FIXUP:=autoreconf

PKG_BUILD_DEPENDS:=libpcap
PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/anlix-arp-scan
  SECTION:=net
  CATEGORY:=Network
  TITLE:=ARP scanner
  URL:=https://github.com/royhills/arp-scan
  PKG_MAINTAINER:=Sergey Urushkin <urusha.v1.0@gmail.com>
endef

CONFIGURE_VARS += \
        LIBS="$(STAGING_DIR)/usr/lib/libpcap.a"

define Package/anlix-arp-scan/description
    ARP scanner
endef

define Package/anlix-arp-scan/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/arp-scan $(1)/usr/bin/
endef

$(eval $(call BuildPackage,anlix-arp-scan))
