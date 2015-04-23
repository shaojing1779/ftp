all: ftp
ftp: server.c response.c
	gcc -o ftp server.c response.c -lpthread
clean: 
	rm -rf *.o ftp
