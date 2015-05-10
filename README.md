Date:2014-04-23</br>
此项目为大学三年级的《UNIX网络程序设计》课程的课程设计</br>
一个简单的多线程并发FTP服务器程序，仅供学习使用 </br>

######编译:
Makefile:</br>
`all: ftp`</br>
`ftp: server.c handles.c`</br>
`    gcc -o ftp server.c handles.c -lpthread`</br>
`clean:`</br>
`    rm -rf *.o ftp`</br>

~$`make` </br>
生成`./ftp`可执行服务器端程序</br>
######链接:
RFC:
* http://www.w3.org/Protocols/rfc959/
* https://www.ietf.org/rfc/rfc959.txt

[FTP Documents by IBM Developerworks] (http://www.ibm.com/developerworks/cn/linux/l-cn-socketftp/) </br>
