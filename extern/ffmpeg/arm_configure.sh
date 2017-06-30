
X264_PATH=$TOP_DIR/install
EXTRA_CFLAGS="-I$X264_PATH/include"
EXTRA_LDFLAGS="-L$X264_PATH/lib"

echo "EXTRA_CFLAGS='$EXTRA_CFLAGS'"
echo "EXTRA_LDFLAGS='$EXTRA_LDFLAGS'"

./configure \
    --arch=arm \
    --target-os=linux \
    --enable-cross-compile \
    --cross-prefix=arm-hisiv200-linux- \
    --cc=arm-hisiv200-linux-gcc \
    --prefix=`pwd`/_arm_install \
    --enable-static \
    --enable-shared \
    --enable-gpl \
    --enable-libx264 \
    --disable-yasm \
    --disable-encoders \
    --enable-encoder=libx264 \
    --enable-encoder=aac \
    --disable-decoders \
    --enable-decoder=h264 \
    --enable-decoder=aac \
    --enable-decoder=mpeg2video \
    --enable-decoder=mpegvideo \
    --enable-decoder=mp2 \
    --enable-decoder=mp3 \
    --disable-muxers \
    --enable-muxer=flv \
    --enable-muxer=mpegts \
    --disable-bsfs \
    --disable-filters \
    --enable-filter=aresample \
    --disable-devices \
    --disable-avdevice \
    --disable-hwaccels \
    --disable-demuxers \
    --enable-demuxer=aac \
    --enable-demuxer=h264 \
    --enable-demuxer=hls \
    --enable-demuxer=mpegts \
    --disable-protocols \
    --enable-protocol=file \
    --enable-protocol=rtmp \
    --enable-protocol=hls \
    --disable-parsers \
    --enable-parser=aac \
    --enable-parser=h264 \
    --enable-parser=mpegvideo \
    --extra-cflags=$EXTRA_CFLAGS \
    --extra-ldflags=$EXTRA_LDFLAGS \
    --disable-xlib \
    --disable-ffserver \
    --disable-ffplay \
    #--disable-ffprobe
    #--disable-ffmpeg \

    #--enable-small \
    #--enable-memalign-hack \
