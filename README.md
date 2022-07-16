# http_server
## setup

make build

make up-all

make exec-takuya

make

## ベンチマーク測定方法
まずsetupでdocker を立ち上げる

nginxのベンチマーク ab -k -c 10 -n 1000 http://127.0.0.1:8080/

自作　 ab -k -c 10 -n 1000 http://localhost:8000/

2022/3/20 時点だとどちらもこんな感じで大差ない

<img width="559" alt="スクリーンショット 2022-03-20 23 27 14" src="https://user-images.githubusercontent.com/75765648/159167249-ef7bba77-fae9-4eae-8cdd-0d26a33dc4be.png">

