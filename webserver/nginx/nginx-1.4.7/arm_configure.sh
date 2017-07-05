#!/bin/sh

export NGX_SYSTEM=linux
#export NGX_RELEASE=3.2-XT5
export NGX_MACHINE=arm

CC_PATH=/opt/hisi-linux/x86-arm/arm-hisiv200-linux/target/bin/arm-hisiv200-linux-gcc
ZLIB_PATH=`pwd`/../extern/zlib-1.2.5
PCRE_PATH=`pwd`/../extern/pcre-8.39
OPENSSL_PATH=`pwd`/../extern/openssl-1.0.2j

./configure \
    --prefix=`pwd`/_arm_install \
    --with-pcre \
    --user=root \
    --group=root \
    --with-cc=`which arm-hisiv200-linux-gcc` \
    --with-cpp=`which arm-hisiv200-linux-g++` \
    --without-http_rewrite_module \
    --with-zlib=$ZLIB_PATH \
    --with-pcre=$PCRE_PATH \
    --with-openssl=$OPENSSL_PATH \
    --add-module=`pwd`/../addon/nginx-rtmp-module \
    #--without-http_gzip_module \
    #--builddir=`pwd`/_build 

./patch.sh rtmp


