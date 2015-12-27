#include "server.h"

void server(int port)
{
    int sock = create_socket(port);
    struct sockaddr_in client_address;
    int len = sizeof(client_address);
    int connection;
    pthread_t thread;

    while(1){
        if ((connection = accept(sock, (struct sockaddr*) &client_address,&len)) == -1) {
            perror("accept Error\n");
            exit(1);
        }
        Arg *arg = (void*)malloc(sizeof(Arg));
        arg->connfd = connection;
        arg->listenfd = sock;
        memcpy((void *)&arg->client, &client_address, sizeof(client_address));
        if (pthread_create(&thread, NULL, start_routine, (void*)arg)) {
            perror("Pthread_create() error");
            exit(1);
        }
    }
}
/*Thread function*/
void *start_routine(void* arg)
{
    Arg *info;
    info = (Arg*)arg;
    pthread_t thread = pthread_self();
    process_cli(info->connfd, info->client, thread);
    free(arg);
    pthread_exit(NULL);
}
/*Process Request*/
void process_cli(int connectfd, struct sockaddr_in client, pthread_t thread)
{
    char buffer[BSIZE];
    int num = 0;
    Command *cmd = malloc(sizeof(Command));
    State *state = malloc(sizeof(State));
    memset(buffer,0,BSIZE);

    char welcome[BSIZE] = "220 ";
    strcat(welcome, "Welcome to FTP service.\n");
    write(connectfd, welcome, strlen(welcome));

    while(1){
        memset(buffer, 0, BSIZE);
        memset(cmd, 0, sizeof(*cmd));
        num = read(connectfd, buffer, BSIZE);
        printf("User %s sent command: %s\n", (state->username==0)?"unknown":state->username, buffer);
        parse_command(buffer,cmd);
        state->connection = connectfd;
        //response(cmd, state);
        switch(lookup_cmd(cmd->command)){
            case USER: ftp_user(cmd,state); break;
            case PASS: ftp_pass(cmd,state); break;
            case PASV: ftp_pasv(cmd,state); break;
            case LIST: ftp_list(cmd,state); break;
            case CWD:  ftp_cwd(cmd,state); break;
            case PWD:  ftp_pwd(cmd,state); break;
            case MKD:  ftp_mkd(cmd,state); break;
            case RMD:  ftp_rmd(cmd,state); break;
            case RETR: ftp_retr(cmd,state); break;
            case STOR: ftp_stor(cmd,state); break;
            case DELE: ftp_dele(cmd,state); break;
            case SIZE: ftp_size(cmd,state); break;
            case ABOR: ftp_abor(state); break;
            case QUIT: ftp_quit(state); break;
            case TYPE: ftp_type(cmd,state); break;
            case CDUP: ftp_cdup(cmd,state); break;
            case NOOP:
                       if(state->logged_in){
                           state->message = "200 Nice to NOOP you!\n";
                       }else{
                           state->message = "530 NOOB hehe.\n";
                       }   
                       write_state(state);
                       break;
            default: 
                       state->message = "500 Unknown command\n";
                       write_state(state);
                       break;
        }

    }
    close(connectfd);
}
/*Create socket & bind it*/
int create_socket(int port)
{
    int sock;
    int reuse = 1;

    struct sockaddr_in server_address = (struct sockaddr_in){ AF_INET, htons(port), (struct in_addr){INADDR_ANY}};

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Cannot open socket");
        exit(EXIT_FAILURE);
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    if(bind(sock,(struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        fprintf(stderr, "Cannot bind socket to address");
        exit(EXIT_FAILURE);
    }

    listen(sock,5);
    return sock;
}

int accept_connection(int socket)
{
    int addrlen = 0;
    struct sockaddr_in client_address;
    addrlen = sizeof(client_address);
    return accept(socket,(struct sockaddr*) &client_address,&addrlen);
}

void getip(int sock, int *ip)
{
    socklen_t addr_size = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    getsockname(sock, (struct sockaddr *)&addr, &addr_size);
    int host,i;

    host = (addr.sin_addr.s_addr);
    for(i=0; i<4; i++){
        ip[i] = (host>>i*8)&0xff;
    }
}


int lookup_cmd(char *cmd){
    const int cmdlist_count = sizeof(cmdlist_str)/sizeof(char *);
    return lookup(cmd, cmdlist_str, cmdlist_count);
}

int lookup(char *needle, const char **haystack, int count)
{
    int i;
    for(i=0;i<count; i++){
        if(strcmp(needle,haystack[i])==0)return i;
    }
    return -1;
}

void write_state(State *state)
{
    write(state->connection, state->message, strlen(state->message));
}

void gen_port(Port *port)
{
    srand(time(NULL));
    port->p1 = 128 + (rand() % 64);
    port->p2 = rand() % 0xff;

}

void parse_command(char *cmdstring, Command *cmd)
{
    sscanf(cmdstring,"%s %s",cmd->command,cmd->arg);
}

int main(int argc, char *argv[])
{
    int port = 0, i = 0;
    if(argc != 2) {
        printf("Usage: %s <Server port>\n", argv[0]);
        exit(1);
    }
    /*String to int*/
    for(i = 0; i < strlen(argv[1]); i++) {
        port = port * 10 + (argv[1][i] - '0');
    }
    time_t t = time(NULL);
    struct tm *timef = localtime(&t);
    printf("----------------------------------------------\n");
    printf("Date: %4d-%02d-%02d %d:%d:%d\n",
            timef->tm_year+1900,
            timef->tm_mon+1,
            timef->tm_mday,
            timef->tm_hour,
            timef->tm_min,
            timef->tm_sec
          );
    struct passwd *pwd;
    pwd = getpwuid(getuid());
    printf("User: %s\n", pwd->pw_name);
    printf("Port: %d\n", port);
    printf("----------------------------------------------\n");
    server(port);
    return 0;
}

