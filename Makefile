all: ftp
ftp: server.c handles.c
	gcc -o ftp server.c handles.c -lpthread
clean: 
	rm -rf *.o ftp
