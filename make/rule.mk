#
# Copyright © 2000-2017 JiuzhouTech, Inc.
# Copyright © 2017-x    yuchen  <yuchen@jiuzhoutech>.
# 
# Created by yuchen  <yuchen@jiuzhoutech>
# Date:    2017-03-09
#
# Compile option and make rule for build 
#
#

#=============CROSS COMPILE CHAINS==============================================================
#CFG_HI_ARM_TOOLCHAINS_NAME = arm-hisiv200-linux
CFG_HI_ARM_TOOLCHAINS_NAME = arm-histbv310-linux
AR=$(CFG_HI_ARM_TOOLCHAINS_NAME)-ar
AS=$(CFG_HI_ARM_TOOLCHAINS_NAME)-as
LD=$(CFG_HI_ARM_TOOLCHAINS_NAME)-ld
CPP=$(CFG_HI_ARM_TOOLCHAINS_NAME)-cpp
CC=$(CFG_HI_ARM_TOOLCHAINS_NAME)-gcc
NM=$(CFG_HI_ARM_TOOLCHAINS_NAME)-nm
STRIP=$(CFG_HI_ARM_TOOLCHAINS_NAME)-strip
OBJCOPY=$(CFG_HI_ARM_TOOLCHAINS_NAME)-objcopy
OBJDUMP=$(CFG_HI_ARM_TOOLCHAINS_NAME)-objdump
CFG_HI_BASE_ENV+=" AR AS LD CPP CC NM STRIP OBJCOPY OBJDUMP "

#=============INCLUDE LINK LD_LIBRARY===========================================================

INCLUDE = -I$(INC_DIR)
INCLUDE += -I$(SRC_DIR)
LIB_PATH = $(TOP_DIR)/install/lib
LIB_LINK = -L$(LIB_PATH)/share -L$(LIB_PATH)/share -L$(LIB_PATH)/extern 

INSTALL_PATH = $(TOP_DIR)/install
INSTALL_LIB_PATH = $(INSTALL_PATH)/lib
INSTALL_INC_PATH = $(INSTALL_PATH)/include

#==============LIB COMPILE OPTIONS==============================================================
CFLAGS :=

ifeq (${CFG_HI_TOOLCHAINS_NAME},arm-hisiv200-linux)
CFLAGS +=-march=armv7-a -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=vfpv3-d16
endif
ifeq (${CFG_HI_TOOLCHAINS_NAME},arm-histbv310-linux)
CFLAGS +=-march=armv7-a -mcpu=cortex-a53 -mfloat-abi=softfp -mfpu=vfpv3-d16
endif

CFLAGS +=  -g -O0 -Wall -fPIC -DUSE_AEC -D_GNU_SOURCE -D_XOPEN_SOURCE=600

CFLAGS+= -DCHIP_TYPE_${CFG_HI_CHIP_TYPE} -DSDK_VERSION=${CFG_HI_SDK_VERSION}

JZCFLAGS += -ffunction-sections -fdata-sections -Wl,--gc-sections -lpthread -lrt -ldl -lm -lstdc++


