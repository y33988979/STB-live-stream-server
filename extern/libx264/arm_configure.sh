./configure \
    --prefix=`pwd`/_arm_install \
    --host=arm-hisiv200-linux \
    --cross-prefix=arm-hisiv200-linux- \
    --disable-asm \
    --disable-cli \
    --enable-pic \
    --enable-static \
    --enable-shared
