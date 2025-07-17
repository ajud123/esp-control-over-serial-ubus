include $(TOPDIR)/rules.mk

PKG_NAME:=esp-ubus-module
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/esp-ubus-module
	CATEGORY:=Utilities
	TITLE:=A program to control an ESP device over UBUS
	DEPENDS:=+libserialport +libubus +libubox +libblobmsg-json +libuci
endef

define Package/esp-ubus-module/description
	This is a library needed to communicate with the Tuya IoT cloud network
endef

define Package/esp-ubus-module/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/esp-ubus-module $(1)/usr/bin
	$(INSTALL_BIN) ./files/esp-ubus-module.init $(1)/etc/init.d/esp-ubus-module
endef

$(eval $(call BuildPackage,esp-ubus-module))