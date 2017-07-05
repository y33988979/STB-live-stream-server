#!/bin/sh

## check params for add patch
echo "add module: \c"
for arg in $@
do
    echo "$arg \c"
    if [ $arg = "zlib" ]; then
        have_zlib=yes
    elif [ $arg = "pcre" ]; then
        have_pcre=yes
    elif [ $arg = "openssl" ]; then
        have_openssl=yes
    elif [ $arg = "rtmp" ]; then
        have_zlib=yes
        have_pcre=yes
        have_openssl=yes
        have_rtmp=yes
    fi
done

echo ""
if test -n "$have_pcre" || test -n "$have_zlib" ; then
    echo "found patch"
fi

## put a patch for cross-compile
cp objs/Makefile objs/Makefile.bak
echo "backup objs/Makefile -> objs/Makefile.bak"
echo "---------------------------------------------------------------------------------------------"
if [ -n "$have_pcre" ]; then
echo -e "\033[032m [ychen]: modify Makefile for support (pcre) cross-complie on arm !!!\033[0m"
echo -e "\033[032m sed -i 's/--disable-shared/--disable-shared --host=arm-hisiv200-linux/g' objs/Makefile\033[0m"
sed -i 's/--disable-shared/--disable-shared --host=arm-hisiv200-linux/g' objs/Makefile
fi
if [ -n "$have_zlib" ]; then
echo -e "\033[032m [ychen]: modify Makefile for support (zlib) cross-complie on arm !!!\033[0m"
echo "Add variable for zlib: \033[032mAR=$cross_prefix-ar RANLIB=$cross_prefix-ranlib \033[0m"
sed -i '/configure\s\\/i\AR=$(cross_prefix)-ar RANLIB=$(cross_prefix)-ranlib \\' objs/Makefile
fi
if [ -n "$have_openssl" ]; then
echo "---------------------------------------------------------------------------------------------"
cross_prefix=arm-hisiv200-linux
openssl_line=`cat objs/Makefile | grep -n no-shared | awk -F ':' '{print $1}'`

## Add complie_cross_prefix variable for openssl
echo "Add variable for openssl: \033[032mcross_prefix=$cross_prefix \033[0m"
sed -i '/LINK =/a\\n#add by ychen for openssl\ncross_prefix=arm-hisiv200-linux\nAR=$(cross_prefix)-ar\nRANLIB=$(cross_prefix)-ranlib\nCROSS_COMPILE=\n' objs/Makefile

## Add CC AR RANLIB CROSS_COMPLIE for openssl
echo "Add variable for openssl: \033[032mCC=$cross_prefix-gcc AR=$cross_prefix-ar RANLIB=$cross_prefix-ranlib CROSS_COMPILE= \033[0m"
sed -i '/no-shared/i\&& CC=$(cross_prefix)-gcc AR=$(cross_prefix)-ar RANLIB=$(cross_prefix)-ranlib CROSS_COMPILE= \\' objs/Makefile

## rename config to Configure for arm openssl
line=`cat objs/Makefile | grep -n "\./config " | awk -F ':' '{print $1}'`
echo "replace: \033[032m./config --> ./Configure  in Makefile line $line\033[0m"
sed -i 's/&& .\/config /.\/Configure /g' objs/Makefile
line=`cat objs/Makefile | grep -n no-shared | awk -F ':' '{print $1}'`

echo "replace: \033[032mno-shared --> no-shared linux-armv4  in Makefile line $line\033[0m"
sed -i 's/no-shared/no-shared linux-armv4 /g' objs/Makefile
fi
## remove Werror option
echo "---------------------------------------------------------------------------------------------"
line=`cat objs/Makefile | grep -n Werror | awk -F ':' '{print $1}'`
echo "remove option: \033[032m-Werror in Makefile line $line\033[0m"
sed -i 's/-Werror//g' objs/Makefile
echo "---------------------------------------------------------------------------------------------"

