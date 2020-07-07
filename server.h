#ifndef _SERVER_H_ 
#define _SERVER_H_ 
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <netinet/in.h>
#include <time.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/utsname.h>
#include <sys/sendfile.h>


#define BSIZE 1024
#define DATAPORT 20
#endif

typedef struct Port
{
  int p1;
  int p2;
} Port;

typedef struct State
{
    /* Connection mode: NORMAL, SERVER, CLIENT */
    int32_t mode;
    /* Is user loggd in? */
    int32_t logged_in;
    /* Is this username allowed? */
    int32_t username_ok;
    char *username;
    /* Response message to client e.g. 220 Welcome */
    char *message;
    /* Commander connection */
    int32_t connection;
    /* PASV MOD*/
    int32_t sock_pasv;
    /* PORT MOD*/
    int32_t sock_port;
    /* Transport type 0-bin 1-ascii */
    uint8_t type;
} State;

typedef struct Arg
{
    int connfd;
    int listenfd;
    pthread_t thread;
    struct sockaddr_in client;
} Arg;
/* Command struct */
typedef struct Command
{
  char command[5];
  char arg[BSIZE];
} Command;

/**
 * Connection mode
 * NORMAL - normal connection mode, nothing to transfer
 * SERVER - passive connection (PASV command), server listens
 * CLIENT - server connects to client (PORT command)
 */
typedef enum conn_mode{ NORMAL, SERVER, CLIENT }conn_mode;

/* Commands enumeration */
typedef enum cmdlist 
{ 
  ABOR, CWD, DELE, LIST, MDTM, MKD, NLST, PASS, PASV,
  PORT, PWD, QUIT, RETR, RMD, RNFR, RNTO, SITE, SIZE,
  STOR, TYPE, CDUP, USER, NOOP, SYST
} cmdlist;

/* String mappings for cmdlist */
static const char *cmdlist_str[] = 
{
  "ABOR", "CWD", "DELE", "LIST", "MDTM", "MKD", "NLST", "PASS", "PASV",
  "PORT", "PWD", "QUIT", "RETR", "RMD", "RNFR", "RNTO", "SITE", "SIZE",
  "STOR", "TYPE", "CDUP", "USER", "NOOP", "SYST"
};
/* User nome */
static const char *usernames[] = {"ftp", "anonymous","lab"};


/* Server functions */
void gen_port(Port *);
void getip(int32_t, int32_t*);
void parse_command(char *, Command *);
int create_socket(int port);
void write_state(State *);
int accept_connection(int);
/*Thread functions*/
void *start_routine(void*);
void process_cli(int, struct sockaddr_in, pthread_t);
void ignore_pipe();
int32_t conn_cli(char*, uint16_t);
int32_t lookup_cmd(char*);
int32_t lookup(char*, const char**, int);
uint32_t inet_addr(char*);

/* void response(Command *, State *); */
void ftp_user(Command *, State *);
void ftp_pass(Command *, State *);
void ftp_pwd(Command *, State *);
void ftp_cwd(Command *, State *);
void ftp_mkd(Command *, State *);
void ftp_rmd(Command *, State *);
void ftp_pasv(Command *, State *);
void ftp_list(Command *, State *);
void ftp_retr(Command *, State *);
void ftp_stor(Command *, State *);
void ftp_dele(Command *, State *);
void ftp_size(Command *, State *);
void ftp_quit(State *);
void ftp_type(Command *, State *);
void ftp_cdup(Command *, State *);
void ftp_abor(State *);
void ftp_port(Command *, State *);
void ftp_syst(State *);

void str_perm(int, char *);

