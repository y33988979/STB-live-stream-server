#!/bin/sh

LIB_PATH=_arm_install/lib
LIB_SHARE=_arm_install/lib/share
mkdir -p $LIB_SHARE

cp $LIB_PATH/libavcodec.so.57.89.100 $LIB_SHARE
cp $LIB_PATH/libavfilter.so.6.82.100 $LIB_SHARE
cp $LIB_PATH/libavformat.so.57.71.100 $LIB_SHARE
cp $LIB_PATH/libavutil.so.55.58.100 $LIB_SHARE
cp $LIB_PATH/libpostproc.so.54.5.100 $LIB_SHARE
cp $LIB_PATH/libswresample.so.2.7.100 $LIB_SHARE
cp $LIB_PATH/libswscale.so.4.6.100 $LIB_SHARE

du -skh $LIB_SHARE
echo "copy lib to \"$LIB_SHARE\" done!"
