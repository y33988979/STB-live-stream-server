# STB-live-stream-server
An embedded streaming media server (based on Hisilicon), to provide VOD / broadcast service, support HLS, RTMP protocol.

#
# Build help description
#

the is very simple for build, Use the following command steps：

## step1: set environment 
source ./env.sh

## step2: compile ffmpeg/libx264/nginx, etc.
make all

## step3: make hls application program
make app

## step4: install include and library to "install/target" 
make install


