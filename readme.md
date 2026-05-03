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
需要先配置好数据库
```sql
CREATE DATABASE webserverdb;
USE webserverdb;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL,
    email    char(50) NULL
);
```
```shell
make
./bin/main
```

### 2、Test
```shell
cd ./webbench-1.5
make
./webbench -c 5000 -t 30 http://ip:port/
# 参数：
# 	-c 表示客户端数
# 	-t 表示时间
# 或使用wrk进行测试
wrk -t 4 -c 5000 -d 30s --latency http://ip:port/
```

### bug fix