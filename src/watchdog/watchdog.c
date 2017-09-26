#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "watchdog.h"


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
    ret = sb_stringCpy(&this->cmdName, cmdName);
    if(ret) goto endfun;
    //printf("%s\n",this->cmdName);
    // FilePath
    ret = sb_stringCpy(&this->filepath, path);
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
    printf("Command: %s with PID: %i opened\n %s \n inode:%lu \n ts: %lis.%li\n",
            this->cmdName, this->pid, this->filepath, this->inode,
            this->time_stamp.tv_sec,this->time_stamp.tv_usec);
}

void openCall_destroy(openCall* this){
    zfree(&this->filepath);
    zfree(&this->cmdName);
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
    ret = sb_stringCpy(&this->cmdName, cmdName);
    if(ret) goto endfun;
    // Args
    ret = sb_stringCpy(&this->args, args);
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
    //zfree(&this->calls);
}
void void_execCall_destroy(void* void_exec_ptr){
    execCall_destroy((execCall*)void_exec_ptr);
}


void execCall_print(execCall* this){
    printf("%s-> pid: %i ppid: %i time: %li.%li\n args: %s\n tracked: %i\n",
          this->cmdName, this->pid, this->ppid, this->time_stamp.tv_sec,
          this->time_stamp.tv_usec, this->args, this->tracked);
}


//OpenCall Filter

int openCall_filter__init(openCall_filter* this){
    return art_tree_init(&this->filter_trie);
}
void openCall_filter__destroy(openCall_filter* this){
    art_tree_destroy(&this->filter_trie);
}

int openCall_filter__filter(openCall_filter* this, openCall* openCall){
    return art_prefixl_search(
            &this->filter_trie,
            (unsigned char*) openCall->filepath,
            strlen(openCall->filepath)+1);
}

int openCall_filter__add(openCall_filter* this, const char* filepath){
    return art_insert(&this->filter_trie,
                      (const unsigned char*) filepath,
                      strlen(filepath)+1,0);
}

// ################# ProcIndex ##################### //

int procindex_init(procindex* this, pid_t guide_pid){
    int ret = NOMINAL;
    this->maxPid = get_max_PID();
    if(this->maxPid == -1){
        ret = -1;
        goto endfun;
    }
    if (guide_pid < 1)
        this->size = this->maxPid*2;
    else
        this->size = (guide_pid < this->maxPid)? guide_pid:this->maxPid;
    this->procs = calloc(2*guide_pid, sizeof(*this->procs));
    if(!this->procs){
        ret = -1;
        this->size = 0;
    }
endfun:
    return ret;
}

void procindex_destroy(procindex* this){
    for(pid_t i = 0; i<this->size; i++){
        if(this->procs+i)
            execCall_destroy(*this->procs+i);
    }
    zfree(&this->procs);
}

int procindex_add(procindex* this, execCall* call){
    int ret = NOMINAL;
    size_t oldsize = this->size;
    while (call->pid > this->size){
        execCall** newExec = realloc(this->procs,
                                    this->size*2*sizeof(*this->procs));
        if(!newExec){
            ret = 1;
            goto endfun;
        }
        this->size = this->size*2;
    }
    for(; oldsize<this->size; oldsize++)
        this->procs[oldsize] = NULL;
    if(this->procs[call->pid+1]){
        ret = 2;
        goto endfun;
    }
    if(!this->procs[call->ppid+1])
        call->ppid = 0;
    this->procs[call->pid+1] = call;

endfun:
    return ret;
}

int procindex_opencall_add(procindex* this, openCall* ocall){
    int ret = NOMINAL;


    return ret;
}
// ################# Surveiller ################### //

int surv_init(surveiller* this, const char* opencall_socketaddr,
              const char* execcall_socketaddr){
    int ret = NOMINAL;
    this->opencall_socketaddr = opencall_socketaddr;
    this->execcall_socketaddr = execcall_socketaddr;
    this->ownPID = getpid();
    this->is_shutting_down = 0;
    this->is_processing_execcall_socket = 0;
    this->is_processing_opencall_socket = 0;
    //Filter init
    ret = openCall_filter__init(&this->open_filter);

    //Process Index init
    ret = procindex_init(&this->procs, this->ownPID);

    //Queue init
    ret = g_ringBuffer_init(&this->commandQueue, sizeof(execCall*));
    return ret;
}

void surv_destroy(surveiller* this){
    //destroy execCalls
    //destroy Queues
    g_ringBuffer_destroyd(&this->commandQueue, void_execCall_destroy);
    procindex_destroy(&this->procs);
    //destroy OpencallFilter
    openCall_filter__destroy(&this->open_filter);
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
    int allocated = 0;

    ret = sb_init(&fields[OTime],64);
    if(ret) goto cleanupStringBuffers; allocated++;
    ret = sb_init(&fields[Cmd],32);
    if(ret) goto cleanupStringBuffers; allocated++;
    ret = sb_init(&fields[PID],16);
    if(ret) goto cleanupStringBuffers; allocated++;
    ret = sb_init(&fields[Rval],8);
    if(ret) goto cleanupStringBuffers; allocated++;
    ret = sb_init(&fields[Path],128);
    if(ret) goto cleanupStringBuffers; allocated++;

    while( (rlen = recv(rc_fd, buf, sizeof(buf), 0)) != -1){
        if(!rlen){
            printf("No longer recieving data\n");
            break;
        }
        ret = sb_appendl(&sbuf, buf,rlen);
        if(ret) break;
        for (int i=0; i<=sbuf.end_pos; i++){
            switch (sbuf.string[i]){
                case '\n':
                    //finish struct
                    if (curr_field == field_num - 1){
                        sbuf.string[i] = '\0';
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
                                break;
                        }
                    }
                    lfield_start = i+1;
                    curr_field =0;
                    //flush fieldbuffers
                    for (int ii=0; ii<field_num; ii++)
                       sb_flush(&fields[ii]);
                    break;
                case '\t':
                    sbuf.string[i] = '\0';
                    sb_append(fields+curr_field,sbuf.string+lfield_start);
                    curr_field++;
                    lfield_start = i+1;
                    break;
            }
        }
        sb_deletehead(&sbuf,lfield_start);
        lfield_start = 0;
    }
cleanupStringBuffers:
     for (int ii=0; ii<allocated;ii++)
        sb_destroy(fields+ii);

endfun:
    return ret;
}


/**
 * Handler for incomming execcalls
 */
int surv_handleExecCallSocket(void* surv_struct){
    int ret = 0;
    int maxtries_enqueue = 3;
    unsigned int seconds_to_wait = 20;
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
    if( ret ) {
        goto endfun;
    }

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
        goto cleanupRecieveBuffer;
    }
    //parse input

    /* Fields:
     *     time 34 chars
     *     command name 7
     *     PID  6
     *     PPID 6
     *     Args unbounded 100
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
        if(!rlen){
            printf("No longer recieving data\n");
            break;
        }
        //printf( "\nbuffer: \n%s\n", buf);
        ret = sb_appendl(&sbuf, buf, rlen);
        //printf( "\nsbuffer: \n%s\n", sbuf.string);
        if(ret) break;
        for (int i=0; i<=sbuf.end_pos; i++){
            switch (sbuf.string[i]){
                case '\n':
                    //finish struct
                    if (curr_field == field_num - 1){
                        sbuf.string[i] = '\0';
                        ret = sb_append(fields+curr_field,
                                        sbuf.string+lfield_start);
                        //printf("field: %i \n cont: %s\n",
                        //        curr_field,fields[curr_field].string);
                        execCall eCall;
                        switch (execCall_init(&eCall, fields+OTime,
                                            fields+Cmd, fields+PID,
                                            fields+PPID,fields+Args)){
                            case 1:
                                //Memory error => Panic
                                perror("ToDo handle memory Error");
                                break;
                            case 2:
                                perror("Handle misalignment of fields");
                                break;
                            case -1:
                                perror("missing something");
                                //Handle case
                                //execCall_print(&eCall);
                            case NOMINAL:
                                ret = g_ringBuffer_write(&surv->commandQueue,
                                                         &eCall);
                                int tries = 0;
                                while( ret & (tries<maxtries_enqueue)){
                                    ret = g_ringBuffer_write(
                                            &surv->commandQueue,
                                            &eCall);
                                    tries++;
                                    sleep(seconds_to_wait);
                                }
                                if(ret){
                                    execCall_destroy(&eCall);
                                    ret = 4;
                                    goto cleanupStringBuffers;
                                }
                        }
                    }
                    lfield_start = i+1;
                    curr_field =0;
                    //flush fieldbuffers
                    for (int ii=0; ii<field_num; ii++)
                       sb_flush(&fields[ii]);
                    break;
                case '\t':
                    sbuf.string[i] = '\0';
                    sb_append(fields+curr_field,sbuf.string+lfield_start);
                    //printf("field: %i \n cont: %s\n",
                    //       curr_field,fields[curr_field].string);
                    curr_field++;
                    lfield_start = i+1;
                    break;

            }
        }
        sb_deletehead(&sbuf,lfield_start);
        //printf("Buffer:%s\n",sbuf.string);
        lfield_start=0;
    }
    //Cleanup if other type of error
cleanupStringBuffers:
    for (int ii=0; ii<field_num;ii++)
        sb_destroy(fields+ii);

cleanupRecieveBuffer:
    sb_destroy(&sbuf);
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
    int ret = NOMINAL;
    max_pid = get_max_PID();
    if (max_pid == -1) {
        fprintf(stderr, "Could not read /proc/sys/kernel/pid_max\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Max PID: %i\n",max_pid);
    fprintf(stderr,"connecting to sockets: ...\n");
    char* opensocket_addr = "/tmp/opentrace_opencalls";
    char* execsocket_addr = "/tmp/opentrace_execcalls";
    surveiller surv;
    ret = surv_init(&surv, opensocket_addr, execsocket_addr);
    if(ret)
        exit(EXIT_FAILURE);
    surv_handleExecCallSocket((void*)&surv);
    surv_destroy(&surv);
    exit(EXIT_SUCCESS);
}
