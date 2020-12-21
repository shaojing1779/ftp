all: ftp
ftp: server.c handles.c
	gcc -g -o ftp  -g server.c -g handles.c -lpthread
clean: 
	rm -rf *.o ftp
