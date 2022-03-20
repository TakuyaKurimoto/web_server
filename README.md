# http_server
setup
make build
make up-all
make exec-takuya
make

ベンチマーク測定方法
まずsetupでdocker を立ち上げる

nginxのベンチマーク ab -k -c 10 -n 1000 http://127.0.0.1:8080/
自作　 ab -k -c 10 -n 1000 http://127.0.0.1:8000/

