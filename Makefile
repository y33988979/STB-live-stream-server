#
# Copyright © 2000-2017 JiuzhouTech, Inc.
# Copyright © 2017-x    yuchen  <yuchen@jiuzhoutech>.
# 
# Created by yuchen  <382347665@qq.com>
# Date:    2017-03-09
#
# top Makefile for build all targets
#
#

include $(MAKE_DIR)/rule.mk


#==============OUT TARTGET DEFINE==============================================================
TARGET = nginx 

#==============MAKE COMMAND====================================================================

all: prepare extern
	echo "make done!"

test: $(LIB_TARGET)
	$(CC) $(CFLAGS) $(JZCFLAGS) $(INCLUDE) $(LIB_TARGET) main.c $< -o $@

#====================================================
#        extern library and open source lib
#====================================================
.PHONY: extern extern_clean
extern:
	make -C extern all
extern_clean:
	make -C extern clean

prepare:
	$(AT)-mkdir -p $(INSTALL_INC_PATH)
	$(AT)-mkdir -p $(INSTALL_LIB_PATH)

$(BIN_TARGET): 

install: $(LIB_TARGET) 
	$(AT)-cp $(TOP_DIR)/test $(INSTALL_PATH)
	$(AT)-cp $(SRC_DIR)/libcas.a $(INSTALL_LIB_PATH)
	$(AT)-cp $(SRC_DIR)/*.h $(INSTALL_INC_PATH)

clean: extern_clean
	$(AT)-rm $(INSTALL_INC_PATH)/* -rfv
	$(AT)-rm $(INSTALL_LIB_PATH)/* -rfv
