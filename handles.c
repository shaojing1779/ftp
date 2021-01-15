#include "server.h"

void ftp_user(Command *cmd, State *state)
{
    const int total_usernames = sizeof(usernames)/sizeof(char *);
    if(lookup(cmd->arg,usernames,total_usernames)>=0){
        state->username = malloc(32);
        memset(state->username,0,32);
        strcpy(state->username,cmd->arg);
        state->username_ok = 1;
        state->message = "331 User name okay, need password\n";
    }else{
        state->message = "530 Invalid username\n";
    }
    write_state(state);
}

/* PASS */
void ftp_pass(Command *cmd, State *state)
{
    if(state->username_ok==1){
        state->logged_in = 1;
        state->message = "230 Login successful\n";
    }else{
        state->message = "500 Invalid username or password\n";
    }
    write_state(state);
}

/* PORT */
void ftp_port(Command *cmd, State *state)
{
    if(state->logged_in){
        char ip_addr[32];
        uint8_t tcp_addr[6] = {0, 0, 0, 0, 0, 0};
        if(cmd->arg == NULL){
            fprintf(stderr, "FTP: Get Arg error.\n");
            pthread_exit(NULL);
        }
        char *pNext;
        pNext = strtok(cmd->arg, ",");
        int32_t count = 0;
        while(pNext != NULL) {
            tcp_addr[count] = atoi(pNext);
            count++;
            pNext = strtok(NULL,",");
        }
        sprintf(ip_addr, "%d.%d.%d.%d", tcp_addr[0], tcp_addr[1], tcp_addr[2], tcp_addr[3]);
        /* Connect to client */
        state->sock_port = conn_cli(ip_addr, 256 * tcp_addr[4] + tcp_addr[5]);
        if(state->sock_port < 0) {
            state->message = "501 Connect error.\n";
            fprintf(stderr, "FTP: Connect error.\n");
            pthread_exit(NULL);
        }
        state->message = "200 PORT SUCCESS!\n";
        state->mode = CLIENT;
    }else{
        state->message = "530 Please login with USER and PASS.\n";
    }
    write_state(state);
}

/* PASV */
void ftp_pasv(Command *cmd, State *state)
{
    if(state->logged_in){
        int ip[4];
        char buff[255];
        char *response = "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n";
        Port *port = malloc(sizeof(Port));
        gen_port(port);
        getip(state->connection,ip);

        /* Close previous passive socket? */
        close(state->sock_pasv);

        /* Start listening here, but don't accept the connection */
        state->sock_pasv = create_socket((256*port->p1)+port->p2);
        printf("port: %d\n",256*port->p1+port->p2);
        sprintf(buff,response,ip[0],ip[1],ip[2],ip[3],port->p1,port->p2);
        state->message = buff;
        state->mode = SERVER;
        puts(state->message);

    }else{
        state->message = "530 Please login with USER and PASS.\n";
        printf("%s",state->message);
    }
    write_state(state);
}

/** LIST */
void ftp_list(Command *cmd, State *state)
{
    if(state->logged_in==1){
        struct dirent *entry;
        struct stat statbuf;
        struct tm *time;
        char timebuff[80];
        int connection;
        time_t rawtime;
        char *dest_dirt = (void*)malloc(strlen(state->cwd) + strlen(cmd->arg) + 2);
        memset((void*)dest_dirt, 0 ,sizeof (dest_dirt));
        if (strlen(cmd->arg) > 0) {
            sprintf(dest_dirt, "%s/%s", state->cwd, cmd->arg);
        }else {
            strcpy(dest_dirt, state->cwd);
        }
        char* orig_dict = (void*)malloc(strlen(state->cwd));
        memset((void*)orig_dict, 0 ,sizeof (orig_dict));
        printf("dest_dirt=%s, orig_dict=%s\n", dest_dirt, orig_dict);
        DIR *dp = opendir(dest_dirt);
        if(!dp){
            state->message = "551 Failed to open directory.\n";
        }else{
            if(state->mode == SERVER){
                connection = accept_connection(state->sock_pasv);
                state->message = "150 Here comes the directory listing.\n";
                puts(state->message);

                while(entry=readdir(dp)){
                    if(stat(entry->d_name,&statbuf)==-1){
                        fprintf(stderr, "SERVER FTP: Error reading file stats...\n");
                    }else{
                        char *perms = malloc(9);
                        memset(perms,0,9);

                        /* Convert time_t to tm struct */
                        rawtime = statbuf.st_mtime;
                        time = localtime(&rawtime);
                        strftime(timebuff,80,"%b %d %H:%M",time);
                        str_perm((statbuf.st_mode & ALLPERMS), perms);
                        dprintf(connection,
                                "%c%s %5ld %4d %4d %8ld %s %s\r\n",
                                (entry->d_type==DT_DIR)?'d':'-',
                                perms,
                                statbuf.st_nlink,
                                statbuf.st_uid,
                                statbuf.st_gid,
                                statbuf.st_size,
                                timebuff,
                                entry->d_name);
                    }
                }
                write_state(state);
                state->message = "226 Directory send OK.\n";
                state->mode = NORMAL;
                close(connection);
                close(state->sock_pasv);
            }else if(state->mode == CLIENT){

                state->message = "150 Here comes the directory listing.\n";
                while(entry=readdir(dp)){
                    if(stat(entry->d_name,&statbuf)==-1){
                        fprintf(stderr, "CLIENT FTP: Error reading file stats...\n");
                    }else{
                        char *perms = malloc(9);
                        memset(perms,0,9);
                        /* Convert time_t to tm struct */
                        rawtime = statbuf.st_mtime;
                        time = localtime(&rawtime);
                        strftime(timebuff,80,"%b %d %H:%M",time);
                        str_perm((statbuf.st_mode & ALLPERMS), perms);
                        dprintf(state->sock_port,
                                "%c%s %5ld %4d %4d %8ld %s %s\r\n",
                                (entry->d_type==DT_DIR)?'d':'-',
                                perms,
                                statbuf.st_nlink,
                                statbuf.st_uid,
                                statbuf.st_gid,
                                statbuf.st_size,
                                timebuff,
                                entry->d_name);
                    }
                }
                write_state(state);
                state->message = "226 Directory send OK.\n";
                state->mode = NORMAL;
                close(state->sock_port);
            }else{
                state->message = "425 Use PASV or PORT first.\n";
            }
        }
        closedir(dp);
    }else{
        state->message = "530 Please login with USER and PASS.\n";
    }
    state->mode = NORMAL;
    write_state(state);
}

/* QUIT */
void ftp_quit(State *state)
{
    state->message = "221 Goodbye!\n";
    write_state(state);
    close(state->connection);
    pthread_exit(NULL);
}

/* PWD */
void ftp_pwd(Command *cmd, State *state)
{
    if(state->logged_in){
        char *result = (char*)malloc(strlen(state->cwd) + 1);
        memset(result, 0, strlen(state->cwd));
        strcat(result,"257 \"");
        strcat(result,state->cwd);
        strcat(result,"\"\n");
        state->message = result;
    }else{
        state->message = "500 Login with USER and PASS.\n";
    }
    write_state(state);
}

/* CWD */
void ftp_cwd(Command *cmd, State *state)
{
    if(state->logged_in){
        do {
            if (strlen(cmd->arg) > 0) {
                printf("BEFO state->cwd|cmd->arg [%s|%s]\n", state->cwd, cmd->arg);

                if (strlen(cmd->arg) == 2 && cmd->arg[0]=='.' && cmd->arg[1]=='.') {

                    if (strcmp(state->cwd, "/") == 0) {
                        state->message = "250 Directory successfully changed.\n";
                        break;
                    } else {
                        char **dirs = split(state->cwd, "/");
                        memset((void*)state->cwd, 0, sizeof (state->cwd));
                        int del_index = 0;
                        for (int i = 0; dirs[i] != NULL; i++) {
                            del_index = i;
                        }
                        if (del_index <= 1) {
                            sprintf(state->cwd, "/");
                        }
                        for (int i = 0; i < del_index; i++) {
                            sprintf(state->cwd, "/%s", dirs[i]);
                        }
                        break;
                    }
                } else if (cmd->arg[0]=='/') {
                    if (access(cmd->arg, F_OK|R_OK) <0 ) {
                        state->message = "550 Failed to change directory.\n";
                        break;
                    }
                    state->cwd = (void*)malloc(strlen(state->cwd) + 1);
                    memset(state->cwd, 0, strlen(state->cwd));
                    strcpy(state->cwd, cmd->arg);
                    state->message = "250 Directory successfully changed.\n";
                } else {
                    char *chg_dir = (char*)malloc(strlen(cmd->arg) + strlen(state->cwd) + 2);
                    memset(chg_dir,0,strlen(chg_dir));
                    sprintf(chg_dir, "%s/%s",state->cwd, cmd->arg);
                    if (access(chg_dir, F_OK|R_OK) <0 ) {
                        state->message = "550 Failed to change directory.\n";
                        break;
                    }
                    state->cwd = (void*)malloc(strlen(chg_dir) + 1);
                    memset((void*)state->cwd, 0, sizeof(state->cwd));
                    strcpy(state->cwd, chg_dir);
                    state->message = "250 Directory successfully changed.\n";
                }
                printf("END state->cwd|cmd->arg [%s|%s]\n", state->cwd, cmd->arg);
            }
        } while(0);
    }else{
        state->message = "500 Login with USER and PASS.\n";
    }
    write_state(state);
}

/* MKD */
void ftp_mkd(Command *cmd, State *state)
{
    if(state->logged_in){
        char res[BSIZE];
        memset(res,0,BSIZE);
        if(cmd->arg[0]=='/'){
            if(mkdir(cmd->arg,S_IRWXU)==0){
                strcat(res,"257 \"");
                strcat(res,cmd->arg);
                strcat(res,"\" new directory created.\n");
                state->message = res;
            }else{
                state->message = "550 Failed to create directory. Check path or permissions.\n";
            }
        }
        else{
            char* dest_dir = malloc(strlen(state->cwd) + strlen(cmd->arg) + 2);
            memset(dest_dir, 0, sizeof (dest_dir));
            sprintf(dest_dir, "%s/%s", state->cwd, cmd->arg);
            if(mkdir(dest_dir,S_IRWXU)==0){
                sprintf(res,"257 \"%s\" new directory created.\n", dest_dir);
                state->message = res;
            }else{
                state->message = "550 Failed to create directory.\n";
            }
        }
    }else{
        state->message = "500 Good news, everyone! There's a report on TV with some very bad news!\n";
    }
    write_state(state);
}

/* RETR */
void ftp_retr(Command *cmd, State *state)
{
    int32_t fd;
    struct stat stat_buf;
    off_t offset = 0;
    ssize_t sent_total = 0;
    if(state->logged_in){
        if(state->mode == SERVER) {
            int32_t connection = -1;
            if(access(cmd->arg,R_OK)==0 && (fd = open64(cmd->arg,O_RDONLY))){
                fstat(fd,&stat_buf);
                state->message = "150 Opening BINARY mode data connection.\n";
                write_state(state);
                connection = accept_connection(state->sock_pasv);
                close(state->sock_pasv);
                do {
                    ssize_t add_byte = sendfile(connection, fd, &offset, stat_buf.st_size);
                    sent_total += add_byte;
                    if(add_byte){

                        if(sent_total == stat_buf.st_size){
                            state->message = "226 File send OK.\n";
                            break;
                        }

                    }else{
                        state->message = "550 Failed to read file.\n";
                    }
                } while (sent_total != stat_buf.st_size);
            }else{
                state->message = "550 Failed to get file\n";
            }
            close(connection);
            close(state->sock_pasv);
        } else if(state->mode == CLIENT) {
            if(access(cmd->arg,R_OK)==0 && (fd = open64(cmd->arg,O_RDONLY))){
                fstat(fd,&stat_buf);
                state->message = "150 Opening BINARY mode data connection..\n";
                write_state(state);
                do {
                    ssize_t add_byte = sendfile(state->sock_port, fd, &offset, stat_buf.st_size);
                    sent_total += add_byte;
                    if(add_byte){

                        if(sent_total == stat_buf.st_size){
                            state->message = "226 File send OK.\n";
                            break;
                        }

                    }else{
                        state->message = "550 Failed to read file.\n";
                    }
                } while (sent_total != stat_buf.st_size);
            }else{
                state->message = "550 Failed to get file\n";
            }
            close(state->sock_port);
        } else{
            state->message = "550 Use PASV or PORT first.\n";
        }
    }else{
        state->message = "530 Please login with USER and PASS.\n";
    }
    close(fd);
    write_state(state);
    state->mode = NORMAL;
}

/* STOR */
void ftp_stor(Command *cmd, State *state)
{
    int32_t fd;
    const int buff_size = 8192;
    FILE *fp = fopen(cmd->arg,"w");
    if(fp==NULL){
        perror("ftp_stor:fopen");
    }else if(state->logged_in){
        if(state->mode == SERVER) {
            int32_t res = 0;
            int32_t pipefd[2];
            fd = fileno(fp);
            int32_t connection = accept_connection(state->sock_pasv);
            close(state->sock_pasv);
            if(pipe(pipefd)==-1)
                perror("ftp_stor: pipe");

            state->message = "125 Data connection already open; transfer starting.\n";
            write_state(state);

            while ((res = splice(connection, 0, pipefd[1], NULL, buff_size, SPLICE_F_MORE | SPLICE_F_MOVE))>0) {
                splice(pipefd[0], NULL, fd, 0, buff_size, SPLICE_F_MORE | SPLICE_F_MOVE);
            }

            if(res==-1) {
                perror("ftp_stor: splice");
                pthread_exit(NULL);
            }else{
                state->message = "226 File send OK.\n";
            }
            close(connection);
            close(state->sock_pasv);

        } else if(state->mode == CLIENT) {
            int32_t res = 0;
            int32_t pipefd[2];
            fd = fileno(fp);
            if(pipe(pipefd)==-1)
                perror("ftp_stor: pipe");
            printf("DEBUG CLIENT socket=%d, fd=%d\n", state->sock_port, fd);
            state->message = "125 Data connection already open; transfer starting.\n";
            write_state(state);

            while ((res = splice(state->sock_port, 0, pipefd[1], NULL, buff_size, SPLICE_F_MORE | SPLICE_F_MOVE))>0) {
                printf("DEBUG res=%d", res);
                splice(pipefd[0], NULL, fd, 0, buff_size, SPLICE_F_MORE | SPLICE_F_MOVE);
            }

            if(res==-1) {
                perror("ftp_stor: splice");
                pthread_exit(NULL);
            }else{
                state->message = "226 File send OK.\n";
            }
            close(state->sock_port);

        } else {
            state->message = "550 use PASV or PORT.\n";
        }
    }else{
        state->message = "530 Please login with USER and PASS.\n";
    }
    write_state(state);
    close(fd);
    state->mode = NORMAL;
}

/* ABOR */
void ftp_abor(State *state)
{
    if(state->logged_in){
        state->message = "226 Closing data connection.\n";
        state->message = "225 Data connection open; no transfer in progress.\n";
    }else{
        state->message = "530 Please login with USER and PASS.\n";
    }
    write_state(state);

}

/* TYPE */
void ftp_type(Command *cmd,State *state)
{
    if(state->logged_in){

        if(cmd->arg[0]=='A') {
            state->message = "200 Switching to ASCII mode.\n";
            state->type = 1;

        } else {
            state->message = "200 Switching to Binary mode.\n";
            state->type = 0;
        }
    }else{
        state->message = "530 Please login with USER and PASS.\n";
    }
    write_state(state);
}

/* CDUP */
void ftp_cdup(Command *cmd, State *state) {

//    if(state->logged_in){
//        if(chdir("..")==0){
//            state->message = "250 Directory successfully changed.\n";
//        }else{
//            state->message = "550 Failed to change directory.\n";
//        }
//    }else{
//        state->message = "500 Login with USER and PASS.\n";
//    }
//    write_state(state);
    memset(cmd->arg, 0 , sizeof (cmd->arg));
    strcpy(cmd->arg, "..");
    ftp_cwd(cmd, state);
}

/* SYST */
void ftp_syst(State *state) {
    struct utsname kernel_info;
    int ret = uname(&kernel_info);
    if (ret == 0) {
        char kversion[512] = { 0 };
        sprintf(kversion, "200 %s-%s%s\n", kernel_info.release, kernel_info.machine, kernel_info.version);
        state->message = kversion;
    } else {
        state->message = "425 Get system info error.\n";
    }
    write_state(state);
}

/* DELE */
void ftp_dele(Command *cmd,State *state)
{
    if(state->logged_in){
        if(unlink(cmd->arg)==-1){
            state->message = "550 File unavailable.\n";
        }else{
            state->message = "250 Requested file action okay, completed.\n";
        }
    }else{
        state->message = "530 Please login with USER and PASS.\n";
    }
    write_state(state);
}

/* RMD */
void ftp_rmd(Command *cmd, State *state)
{
    if(!state->logged_in){
        state->message = "530 Please login first.\n";
    }else{
        if(rmdir(cmd->arg)==0){
            state->message = "250 Requested file action okay, completed.\n";
        }else{
            state->message = "550 Cannot delete directory.\n";
        }
    }
    write_state(state);
}

void ftp_size(Command *cmd, State *state)
{
    if(state->logged_in){
        struct stat statbuf;
        char filesize[128];
        memset(filesize,0,128);
        if(stat(cmd->arg,&statbuf)==0){
            sprintf(filesize, "213 %ld\n", statbuf.st_size);
            state->message = filesize;
        }else{
            state->message = "550 Could not get file size.\n";
        }
    }else{
        state->message = "530 Please login with USER and PASS.\n";
    }

    write_state(state);

}

void str_perm(int perm, char *str_perm)
{
    int curperm = 0;
    int flag = 0;
    int read, write, exec;

    char fbuff[3];
    read = write = exec = 0;

    int i;
    for(i = 6; i>=0; i-=3){
        curperm = ((perm & ALLPERMS) >> i ) & 0x7;

        memset(fbuff,0,3);
        read = (curperm >> 2) & 0x1;
        write = (curperm >> 1) & 0x1;
        exec = (curperm >> 0) & 0x1;

        sprintf(fbuff,"%c%c%c",read?'r':'-' ,write?'w':'-', exec?'x':'-');
        strcat(str_perm,fbuff);
    }
}

