#
## Copyright © 2000-2017 JiuzhouTech, Inc.
# Copyright © 2017-x    yuchen  <yuchen@jiuzhoutech>.
# # 
# # Created by yuchen  <382347665@qq.com>
# # Date:    2017-06-09
# #
# # Makefile for extern library build 
# #

include $(MAKE_DIR)/rule.mk

LIBX264 = last_x264
LIBX264_DIR = x264-snapshot-20170614-2245
INSTALL_DIR = $(LIBX264_DIR)/_arm_install

TARGETS = libx264

libx264: $(LIBX264_DIR)/tar
	cd $(LIBX264_DIR) && make && make install

$(LIBX264_DIR)/tar:
	$(AT)test -d $(LIBX264_DIR) || (tar xf $(LIBX264).tar.bz2; \
	cp ./arm_configure.sh $(LIBX264_DIR) ; \
	cd $(LIBX264_DIR); sh arm_configure.sh $(INSTALL_DIR);)
	#touch $@

.PHONY: all clean

all: $(TARGETS)
	echo "install libx264.."
	cp $(INSTALL_DIR)/include/* -rf $(INSTALL_INC_PATH)
	cp $(INSTALL_DIR)/lib/* -rf $(INSTALL_LIB_PATH)
	

clean:
	$(AT)rm -rf $(INSTALL_DIR)/lib/libx264*
	$(AT)rm -rf $(LIBX264_DIR)


