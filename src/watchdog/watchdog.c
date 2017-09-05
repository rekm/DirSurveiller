#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include "watchdog.h"
#include <fcntl.h>


// OpenCall utils

/**
 * Setting up an openCall with a stringBuffers
 * containing the relevant information via
 * pass through
 * @returns:(-1/_) if inode determination fails
 *           (0/NOMINAL) if success
 *           (1/_) if memory allocation failed
 *           (2/_) if timevalue cannot be parsed
 */
int openCall_init(openCall* this,
                  stringBuffer* openTimeStamp,
                  stringBuffer* cmdName,
                  stringBuffer* pid,
                  stringBuffer* returnVal,
                  stringBuffer* path){
    //filedescriptor for inode
    int ret = 0;
    // timeStamp
    int timedot = 0;
    for(; timedot<openTimeStamp->end_pos; timedot++){
       if(openTimeStamp->string[timedot] == '.'){
          openTimeStamp->string[timedot] = '\0';
          break;
       }
    }
    if (timedot==openTimeStamp->end_pos){
        ret = 2;
        goto endfun;
    }
    this->time_stamp.tv_sec = atol(openTimeStamp->string);
    this->time_stamp.tv_usec = atol(openTimeStamp->string+1+timedot);
    // CMD name
    ret = sb_stringCpy(this->cmdName, cmdName);
    if(ret) goto endfun;
    // FilePath
    ret = sb_stringCpy(this->filepath, path);
    if(ret) goto freeCmd;
    this->flag = atoi(returnVal->string);
    this->pid = atoi(pid->string);
    //Get inode number
    struct stat file_stat;
    ret = stat(path->string, &file_stat);
    if(ret<0) goto endfun;
    this->inode = file_stat.st_ino;
endfun:
    return ret;

freeCmd:
    zfree(&this->cmdName);
    return ret;
}

void openCall_print(openCall* this){
    printf("Command: %s with PID: %i opened\n %s \n inode:%lu \n ts: %li.%li\n",
            this->cmdName, this->pid, this->filepath, this->inode,
            this->time_stamp.tv_sec,this->time_stamp.tv_usec);
}

void openCall_destroy(openCall* this){
    zfree(&this->filepath);
    zfree(&this->cmdName);
}

/**
 * Handling of incomming opencalls and filtering of them
 */
int surv_handleOpenCallSocket(void* surv_struct){
    int ret = 0;
    char buf[256]; //buffer for recieve call
    /*
     * more dynamic buffer with functions that
     * make usage more enjoyable
     */
    stringBuffer sbuf;
    int field_num = 5; // Number of fields that need parsing
    enum FieldDesc { OTime, Cmd, PID, Rval, Path};
    // Array of dynamic StringBuffers for fields
    stringBuffer fields[field_num];
    int rc_fd; // recieve file descriptor
    ssize_t rlen;
    struct sockaddr_un addr;

    ret = sb_init(&sbuf, 512);
    surveiller* surv = (surveiller*) surv_struct;
    if ( (rc_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        ret = 2;
        goto endfun;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if(*surv->opencall_socketaddr == '\0'){
        *addr.sun_path = '\0';
        strncpy( addr.sun_path + 1,
                surv->opencall_socketaddr + 1, sizeof(addr.sun_path) - 2);
    } else {
        strncpy(addr.sun_path,
                surv->opencall_socketaddr, sizeof(addr.sun_path) -1);
    }
    if (connect( rc_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        ret = 3;
        goto endfun;
    }
    //parse input

    /* Fields:
     *     time 34 chars
     *     command name 7
     *     PID  5
     *     Flag
     *     filename unbounded 100
     */
    int curr_field = 0;
    int lfield_start = 0;

    ret = (sb_init(&fields[OTime],64)
         | sb_init(&fields[Cmd],32)
         | sb_init(&fields[PID],16)
         | sb_init(&fields[Rval],8)
         | sb_init(&fields[Path],128) );
    if(ret)
        goto cleanupStringBuffers;

    while( (rlen = recv(rc_fd, buf, sizeof(buf), 0)) != -1){
        printf( "%s\n", buf);
        ret = sb_append(&sbuf, buf);
        if(ret) break;
        for (int i=0; i<=sbuf.end_pos; i++){
            switch (sbuf.string[i]){
                case '\n':
                    //finish struct
                    if (curr_field >= field_num - 1){
                        //Error try and recover
                    }
                    else {
                        ret = sb_append(fields+curr_field,
                                        sbuf.string+lfield_start);
                        openCall oCall;
                        switch (openCall_init(&oCall, fields+OTime,
                                            fields+Cmd, fields+PID,
                                            fields+Rval,fields+Path)){
                            case NOMINAL:
                                openCall_print(&oCall);
                                openCall_destroy(&oCall);
                                break;
                            case 1:
                                //Memory error => Panic
                                perror("ToDo handle memory Error");
                                break;
                            case 2:
                                perror("Handle misalignment of fields");
                                break;
                            case -1:
                                openCall_print(&oCall);
                                openCall_destroy(&oCall);
                        }
                        sb_deletehead(&sbuf,i+1);
                        lfield_start = 0;
                        curr_field =0;

                    }
                    //flush fieldbuffers
                    for (int ii=0; ii<field_num; ii++)
                       sb_flush(&fields[ii]);
                    break;
                case '\t':
                    sbuf.string[i] = '\0';
                    sb_append(fields+curr_field,sbuf.string+lfield_start);
                    lfield_start = i+1;
                    break;
            }
        }
    }
cleanupStringBuffers:
     for (int ii=0; ii<field_num;ii++)
        sb_destroy(fields+ii);

endfun:
    return ret;
}


int execCall_init(execCall* this,
                  stringBuffer* execTimeStamp,
                  stringBuffer* cmdName,
                  stringBuffer* pid,
                  stringBuffer* ppid,
                  stringBuffer* args){
    this->tracked = 0;
    int ret = 0;
    // timeStamp
    int timedot = 0;
    for(; timedot<execTimeStamp->end_pos; timedot++){
       if(execTimeStamp->string[timedot] == '.'){
          execTimeStamp->string[timedot] = '\0';
          break;
       }
    }
    if (timedot==execTimeStamp->end_pos){
        ret = 2;
        goto endfun;
    }
    this->time_stamp.tv_sec = atol(execTimeStamp->string);
    this->time_stamp.tv_usec = atol(execTimeStamp->string+1+timedot);
    // CMD name
    ret = sb_stringCpy(this->cmdName, cmdName);
    if(ret) goto endfun;
    // Args
    ret = sb_stringCpy(this->args, args);
    if(ret) goto freeCmd;
    this->ppid = atoi(ppid->string);
    this->pid = atoi(pid->string);
endfun:
    return ret;

freeCmd:
    zfree(&this->cmdName);
    return ret;
}
void execCall_destroy(execCall* this){
    zfree(&this->args);
    zfree(&this->cmdName);
    zfree(&this->calls);
}

void execCall_print(execCall* this){
    printf("%s-> pid: %i ppid: %i time: %li.%li\n args: %s\n tracked: %i\n",
          this->cmdName, this->pid, this->ppid, this->time_stamp.tv_sec,
          this->time_stamp.tv_usec, this->args, this->tracked);
}  



/**
 * Handler for incomming execcalls
 */
int surv_handleExecCallSocket(void* surv_struct){
    int ret = 0;
    char buf[256]; //buffer for recieve call
    /*
     * more dynamic buffer with functions that
     * make usage more enjoyable
     */
    stringBuffer sbuf;
    int field_num = 5; // Number of fields that need parsing
    enum FieldDesc { OTime, Cmd, PID, PPID, Args};
    // Array of dynamic StringBuffers for fields
    stringBuffer fields[field_num];
    int rc_fd; // recieve file descriptor
    ssize_t rlen;
    struct sockaddr_un addr;

    ret = sb_init(&sbuf, 512);
    surveiller* surv = (surveiller*) surv_struct;
    if ( (rc_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        ret = 2;
        goto endfun;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if(*surv->execcall_socketaddr == '\0'){
        *addr.sun_path = '\0';
        strncpy( addr.sun_path + 1,
                surv->execcall_socketaddr + 1, sizeof(addr.sun_path) - 2);
    } else {
        strncpy(addr.sun_path,
                surv->execcall_socketaddr, sizeof(addr.sun_path) -1);
    }
    if (connect( rc_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        ret = 3;
        goto endfun;
    }
    //parse input

    /* Fields:
     *     time 34 chars
     *     command name 7
     *     PID  5
     *     Flag
     *     filename unbounded 100
     */
    int curr_field = 0;
    int lfield_start = 0;

    ret = (sb_init(&fields[OTime],64)
         | sb_init(&fields[Cmd],32)
         | sb_init(&fields[PID],16)
         | sb_init(&fields[PPID],16)
         | sb_init(&fields[Args],128) );
    if(ret)
        goto cleanupStringBuffers;

    while( (rlen = recv(rc_fd, buf, sizeof(buf), 0)) != -1){
        printf( "%s\n", buf);
        ret = sb_append(&sbuf, buf);
        if(ret) break;
        for (int i=0; i<=sbuf.end_pos; i++){
            switch (sbuf.string[i]){
                case '\n':
                    //finish struct
                    if (curr_field >= field_num - 1){
                        //Error try and recover
                    }
                    else {
                        ret = sb_append(fields+curr_field,
                                        sbuf.string+lfield_start);
                        execCall eCall;
                        switch (execCall_init(&eCall, fields+OTime,
                                            fields+Cmd, fields+PID,
                                            fields+PPID,fields+Args)){
                            case NOMINAL:
                                execCall_print(&eCall);
                                execCall_destroy(&eCall);
                                break;
                            case 1:
                                //Memory error => Panic
                                perror("ToDo handle memory Error");
                                break;
                            case 2:
                                perror("Handle misalignment of fields");
                                break;
                            case -1:
                                execCall_print(&eCall);
                                execCall_destroy(&eCall);
                        }
                        sb_deletehead(&sbuf,i+1);
                        lfield_start = 0;
                        curr_field =0;

                    }
                    //flush fieldbuffers
                    for (int ii=0; ii<field_num; ii++)
                       sb_flush(&fields[ii]);
                    break;
                case '\t':
                    sbuf.string[i] = '\0';
                    sb_append(fields+curr_field,sbuf.string+lfield_start);
                    lfield_start = i+1;
                    break;
            }
        }
    }
cleanupStringBuffers:
     for (int ii=0; ii<field_num;ii++)
        sb_destroy(fields+ii);

endfun:
    return ret;
}
/**
 * Gets max pid from proc pseudo filesystem under linux
 */
int get_max_PID(){
    int ret = 0;
    FILE* pid_maxf;
    //reading /proc/sys/kernel/pid_max

    char line[100];
    pid_maxf = fopen("/proc/sys/kernel/pid_max","r");
    if (!pid_maxf){
        ret = -1;
        goto endfun;
    }
    if (!fgets(line, 100, pid_maxf)){
        ret = -1;
        goto closeFile;
    }
    ret = atoi(line);

closeFile:
    fclose(pid_maxf);
endfun:
    return ret;

}





int main(void){
    int max_pid;

    max_pid = get_max_PID();
    if (max_pid == -1) {
        fprintf(stderr, "Could not read /proc/sys/kernel/pid_max\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Max PID: %i\n",max_pid);
    exit(EXIT_SUCCESS);
}
