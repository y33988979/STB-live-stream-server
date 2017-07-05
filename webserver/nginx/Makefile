#
## Copyright © 2000-2017 JiuzhouTech, Inc.
# Copyright © 2017-x    yuchen  <yuchen@jiuzhoutech>.
# # 
# # Created by yuchen  <382347665@qq.com>
# # Date:    2017-06-09
# #
# # Makefile for nginx build 
# #

include $(MAKE_DIR)/rule.mk

NGINX = nginx
NGINX_DIR = nginx-1.10.1
INSTALL_DIR = $(NGINX_DIR)/_arm_install

ZLIB_SRC=zlib-1.2.5
PCRE_SRC=pcre-8.39
OPENSSL_SRC=openssl-1.0.2j
RTMP_MODULE_SRC=nginx-rtmp-module

#CONFIG_FILE=arm_configure_def.sh
CONFIG_FILE=arm_configure.sh

AT := -

.PHONY: prepare
TARGETS = prepare $(NGINX)

prepare:
	(cd extern; \
	test -d $(ZLIB_SRC) || tar xf $(ZLIB_SRC).tar.gz; \
	chmod 777 $(ZLIB_SRC)/configure; \
	test -d $(PCRE_SRC) || tar xf $(PCRE_SRC).tar.bz2; \
	test -d $(OPENSSL_SRC) || tar xf $(OPENSSL_SRC).tar.gz;)
	(cd addon;\
	test -d $(RTMP_MODULE_SRC) || unzip $(RTMP_MODULE_SRC)-master.zip;\
	test -d $(RTMP_MODULE_SRC) || mv $(RTMP_MODULE_SRC)-master $(RTMP_MODULE_SRC);)

$(NGINX): $(NGINX_DIR)/tar 
	cd $(NGINX_DIR) && make && make install

$(NGINX_DIR)/tar:
	$(AT)test -d $(NGINX_DIR) || (tar xf $(NGINX_DIR).tar.gz; \
	cp ./configs/$(CONFIG_FILE) $(NGINX_DIR)/arm_configure.sh; \
	cp ./patch/nginx/* -rf $(NGINX_DIR)/; \
	cp ./patch/patch.sh -rf $(NGINX_DIR)/; \
	cd $(NGINX_DIR); sh arm_configure.sh;)
	touch $@

.PHONY: all clean

all: $(TARGETS)
	echo "install nginx.."
	cp $(INSTALL_DIR)/* -rf $(INSTALL_PATH)

clean:
	$(AT)rm -rf extern/$(ZLIB_SRC)
	$(AT)rm -rf extern/$(PCRE_SRC)
	$(AT)rm -rf extern/$(OPENSSL_SRC)
	$(AT)rm -rf addon/$(RTMP_MODULE_SRC)
	#$(AT)rm -rf $(INSTALL_PATH)/sbin/nginx
	$(AT)rm -rf $(NGINX_DIR)


