#!/bin/bash

START=1
STOP=1

case $1 in
    start)
        START=1
        STOP=0
        ;;
    stop)
        START=0
        STOP=1
        ;;
    "")
        STOP=1
        START=1
        ;;
    *)
        STOP=0
        START=0
        ;;
esac

# **************************** 杀死正在运行的CGI进程 **************************** 
if [ "$STOP" -eq 1 ];then
    # 登录
    kill -9 $(ps aux | grep "./bin_cgi/login" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
    # 注册
    kill -9 $(ps aux | grep "./bin_cgi/register" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
    # MD5 秒传
    kill -9 $(ps aux | grep "./bin_cgi/md5" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
    # WaresTable
    kill -9 $(ps aux | grep "./bin_cgi/wares" | grep -v grep | awk '{print $2}') > /dev/null 2>&1

    echo "CGI 程序已经成功关闭, bye-bye ..."

fi


# ******************************* 重新启动CGI进程 ******************************* 
if [ "$START" -eq 1 ];then
    # test
    echo -n "test: "
    spawn-fcgi -a 127.0.0.1 -p 9001 -f ./bin_cgi/test.cgi
    # 登录
    echo -n "登录："
    spawn-fcgi -a 127.0.0.1 -p 10002 -f ./bin_cgi/login
    # 注册
    echo -n "注册："
    spawn-fcgi -a 127.0.0.1 -p 10001 -f ./bin_cgi/register
    # MD5秒传
    echo -n "MD5："
    spawn-fcgi -a 127.0.0.1 -p 10003 -f ./bin_cgi/md5
    # WaresTable 
    echo -n "WaresTable: "
    spawn-fcgi -a 127.0.0.1 -p 10004 -f ./bin_cgi/wares

    echo "CGI 程序已经成功启动 ^_^..."
fi
