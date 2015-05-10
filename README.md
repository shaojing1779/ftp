Date:2014-04-23</br>
此项目为大学三年级的《UNIX网络程序设计》课程的课程设计</br>
一个简单的多线程并发FTP服务器程序，仅供学习使用 </br>

######编译:
Makefile:</br>
```sh
all: ftp
ftp: server.c handles.c
    gcc -o ftp server.c handles.c -lpthread
clean:
    rm -rf *.o ftp
```
~$`make` </br>
生成`./ftp`可执行服务器端程序</br>
######链接:
RFC:
* http://www.w3.org/Protocols/rfc959/
* https://www.ietf.org/rfc/rfc959.txt</br>
IBM Developerworks:</br>
[使用 Socket 通信实现 FTP 客户端程序](http://www.ibm.com/developerworks/cn/linux/l-cn-socketftp/) </br>
