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

FFMPEG = ffmpeg
FFMPEG_DIR = ffmpeg-3.3.1
INSTALL_DIR = $(FFMPEG_DIR)/_arm_install

TARGETS = $(FFMPEG)

$(FFMPEG): $(FFMPEG_DIR)/tar
	cd $(FFMPEG_DIR) && make && make install

$(FFMPEG_DIR)/tar:
	$(AT)test -d $(FFMPEG_DIR) || (tar xf $(FFMPEG_DIR).tar.bz2; \
	cp ./arm_configure.sh $(FFMPEG_DIR) ; \
	cd $(FFMPEG_DIR); sh arm_configure.sh;)
	touch $@

.PHONY: all clean

all: $(TARGETS)
	echo "install ffmpeg.."
	cp $(INSTALL_DIR)/include/* -rf $(INSTALL_INC_PATH)
	cp $(INSTALL_DIR)/lib/* -rf $(INSTALL_LIB_PATH)
	cp $(INSTALL_DIR)/bin -rf $(INSTALL_PATH)

clean:
	$(AT)rm -rf $(INSTALL_DIR)/lib/libav*
	$(AT)rm -rf $(INSTALL_DIR)/lib/libpostproc
	$(AT)rm -rf $(INSTALL_DIR)/lib/libswresample
	$(AT)rm -rf $(INSTALL_DIR)/lib/libswscale
	$(AT)rm -rf $(INSTALL_DIR)/include/libav*
	$(AT)rm -rf $(INSTALL_DIR)/include/libpostproc
	$(AT)rm -rf $(INSTALL_DIR)/include/libswresample
	$(AT)rm -rf $(INSTALL_DIR)/include/libswscale
	$(AT)rm -rf $(INSTALL_DIR)/bin/ff*
	$(AT)rm -rf $(FFMPEG_DIR)


