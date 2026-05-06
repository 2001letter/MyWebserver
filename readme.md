### 0、项目结构
```
.
├── code           源代码
│   ├── buffer
│   ├── http
│   ├── log
│   ├── pool
│   ├── server
│   ├── timer
│   └── main.cpp
├── test           测试
|
├── resource       静态资源
│   ├── xxx.html
│   ├── picture
│   ├── video
│   ├── js
│   └── css
├── bin            可执行文件
│   └── main
├── log            日志文件
├── webbench-1.5   压力测试
├── Makefile
└── readme.md
```

### 1、build
下载mysql开发库
```shell
# C++ mysql开发库
sudo apt install libmysqlcppconn-dev -y
# mysql
sudo apt install mysql-server
```
mysql配置
```sql
CREATE DATABASE webserverdb;
USE webserverdb;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL,
    email    char(50) NULL
);
```
编译运行
```shell
make
./bin/main
```

### 2、Test
```shell
sudo apt install wrk
wrk -t 16 -c 5000 -d 30s --latency http://ip:port/
# 参数：
# 	-t 表示使用线程数
# 	-c 表示客户端连接数量
#   -d 表示时间
```
![alt text](imgs/press.png)