# STB-live-stream-server
An embedded streaming media server (based on Hisilicon), to provide VOD / broadcast service, support HLS, RTMP protocol.

### Author yuchen
### E-mail: 382347665@qq.com

#
### Build help description
#

the is very simple for build, Use the following command steps：

#### step0: source download
git clone https://github.com/y33988979/STB-live-stream-server.git

#### step1: set environment 
source ./env.sh

#### step2: compile ffmpeg/libx264/nginx, etc.
make all

#### step3: make hls application program
make app

#### step4: install include and library to "install/target" 
make install

### nginx config for hls/rtmp 
in nginx.conf

#user  nobody;
##nginx多进程worker模式;
worker_processes  2; 

##nginx log记录;
#error_log  logs/error.log;
#error_log  logs/error.log  notice;
#error_log  logs/error.log  info;

#pid        logs/nginx.pid;


events {
    worker_connections  1024;
}

#rtmp配置，nginx必须要添加nginx-rtmp-module模块才能支持;
rtmp {
    server {
        listen 1935;
        chunk_size 4000;
        #直播流配置;
        application app {
            ## enable live streaming
            live on;
        }
        #hls切片, 视频目录, m3u8索引, 播放控制等配置  ;
        application hls {
            live on;
            hls on;
            hls_path /tmp/hls;
            hls_fragment 4s;
            hls_playlist_length 8s;
        }
    }
}

#web 模块;
http {
    include       mime.types;
    default_type  application/octet-stream;

    #log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
    #                  '$status $body_bytes_sent "$http_referer" '
    #                  '"$http_user_agent" "$http_x_forwarded_for"';

    #access_log  logs/access.log  main;

    sendfile        on;
    #tcp_nopush     on;

    #keepalive_timeout  0;
    keepalive_timeout  65;

    #gzip  on;

    server {
        listen       80;
        server_name  localhost;

        #charset koi8-r;

        #access_log  logs/host.access.log  main;

        location / {
            root   html;
            index  index.html index.htm;
        }
        #hls的http请求meta type;
        location /hls {
            # Serve HLS fragments
            types {
                application/vnd.apple.mpegurl m3u8;
                video/mp2t ts;
            }
            #使用ramfs不占用物理flash;
            root /tmp;
            add_header Cache-Control no-cache;
        }

        #error_page  404              /404.html;

        # redirect server error pages to the static page /50x.html
        #
        error_page   500 502 503 504  /50x.html;
        location = /50x.html {
            root   html;
        }

        # proxy the PHP scripts to Apache listening on 127.0.0.1:80
        #
        #location ~ \.php$ {
        #    proxy_pass   http://127.0.0.1;
        #}

        # pass the PHP scripts to FastCGI server listening on 127.0.0.1:9000
        #
        #location ~ \.php$ {
        #    root           html;
        #    fastcgi_pass   127.0.0.1:9000;
        #    fastcgi_index  index.php;
        #    fastcgi_param  SCRIPT_FILENAME  /scripts$fastcgi_script_name;
        #    include        fastcgi_params;
        #}

        # deny access to .htaccess files, if Apache's document root
        # concurs with nginx's one
        #
        #location ~ /\.ht {
        #    deny  all;
        #}
    }


    # another virtual host using mix of IP-, name-, and port-based configuration
    #
    #server {
    #    listen       8000;
    #    listen       somename:8080;
    #    server_name  somename  alias  another.alias;

    #    location / {
    #        root   html;
    #        index  index.html index.htm;
    #    }
    #}


    # HTTPS server
    #
    #server {
    #    listen       443;
    #    server_name  localhost;

    #    ssl                  on;
    #    ssl_certificate      cert.pem;
    #    ssl_certificate_key  cert.key;

    #    ssl_session_timeout  5m;

    #    ssl_protocols  SSLv2 SSLv3 TLSv1;
    #    ssl_ciphers  HIGH:!aNULL:!MD5;
    #    ssl_prefer_server_ciphers   on;

    #    location / {
    #        root   html;
    #        index  index.html index.htm;
    #    }
    #}

}
