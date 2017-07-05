
default:	build

clean:
	rm -rf Makefile objs

build:
	$(MAKE) -f objs/Makefile
	$(MAKE) -f objs/Makefile manpage

install:
	$(MAKE) -f objs/Makefile install

upgrade:
	/home/ychen/platform/project/STB-live-stream-server/webserver/nginx/nginx-1.4.7/_arm_install/sbin/nginx -t

	kill -USR2 `cat /home/ychen/platform/project/STB-live-stream-server/webserver/nginx/nginx-1.4.7/_arm_install/logs/nginx.pid`
	sleep 1
	test -f /home/ychen/platform/project/STB-live-stream-server/webserver/nginx/nginx-1.4.7/_arm_install/logs/nginx.pid.oldbin

	kill -QUIT `cat /home/ychen/platform/project/STB-live-stream-server/webserver/nginx/nginx-1.4.7/_arm_install/logs/nginx.pid.oldbin`
