#
# Build help description
#

## step1: set environment 
source env.sh

## step2: compile ffmpeg/libx264/nginx, etc.
make all

## step3: clean all targets/object
make clean

## step4: install include and library to "install/target" 
make install

