#!/bin/bash


function get_make_command()
{
  echo command make
}

function make()
{
    local start_time=$(date +"%s")
    $(get_make_command) "$@"
    local ret=$?
    local end_time=$(date +"%s")
    local tdiff=$(($end_time-$start_time))
    local hours=$(($tdiff / 3600 ))
    local mins=$((($tdiff % 3600) / 60))
    local secs=$(($tdiff % 60))
    echo
    if [ $ret -eq 0 ] ; then
        echo -n -e "\033[1;32m #### make \""$@"\" completed successfully!"
    else
        echo -n -e "\033[1;31m #### make target \""$@"\" failed !!!"
    fi
    if [ $hours -gt 0 ] ; then
        printf "(%02g:%02g:%02g (hh:mm:ss))" $hours $mins $secs
    elif [ $mins -gt 0 ] ; then
        printf "(%02g:%02g (mm:ss))" $mins $secs
    elif [ $secs -gt 0 ] ; then
        printf "(%s seconds)" $secs
    fi
    echo -e " ####\033[1;0m"
    echo
    return $ret
}

function setpaths()
{
    arm_histbv310_path=${PWD}/tools/linux/toolchains/arm-histbv310-linux/bin
    aarch64_histbv100_path=${PWD}/tools/linux/toolchains/aarch64-histbv100-linux/bin
    utils_path=${PWD}/tools/linux/utils/bin

    PATH=${arm_histbv310_path}:${PATH//${arm_histbv310_path}:/}
    PATH=${aarch64_histbv100_path}:${PATH//${aarch64_histbv100_path}:/}
    PATH=${utils_path}:${PATH//${utils_path}:/}
}

function set_cross_compile_env()
{
    ## defualt path
    cross_compile_chain_path=/opt/hisi-linux/x86-arm/arm-hisiv200-linux/bin

    ## use CROSS_PATH if it is set
    if [ ! -z $CROSS_PATH ] ; then
        cross_compile_chain_path=$CROSS_PATH
    fi

    echo "The current cross_compile_chain_path: $cross_compile_chain_path"
    echo -e "check cross-compiler... \c"
    ## check gcc
    found_gcc=0
    gcc=`ls $cross_compile_chain_path/*gcc 2>&1`
    if [ -x "$gcc" ] ; then
        found_gcc=1
    fi

    # print result
    if [ $found_gcc -eq 1 ] ; then
        PATH=$PATH:$cross_compile_chain_path
        echo -e "\033[32mOK \033[0m"
    else
        echo -e "\033[33mFAILED \033[0m"
        echo -e "\033[31m Found not cross-compile tools! \033[0m"
        echo -e "please install cross-compiler to path: \033[32m $cross_compile_chain_path \033[0m"
        echo -e "Note: if you have already a cross-compiler, please set the CROSS_PATH environment variable."
        echo -e "export CROSS_PATH=\$your_compile_path "
    fi
}

function check_bash()
{
    is_bash=`${SHELL} --version | grep "GNU bash"`
    is_bash=${is_bash:0:8}
    if [ "${is_bash}" != "GNU bash" ] ; then
        echo "The shell may NOT be a bash, we need a bash to build SDK, please use bash!";
        echo "To change your shell, you should be root first, and then set by steps followed:";
        echo "    rm -f /bin/sh";
        echo "    ln -s /bin/bash  /bin/sh";
        echo ""
    fi
}

check_bash

set_cross_compile_env
#setpaths

export HISI_ENV=ok
export TOP_DIR=$(pwd)
export SRC_DIR=$TOP_DIR/src
export INC_DIR=$TOP_DIR/inc
export MAKE_DIR=$TOP_DIR/make

