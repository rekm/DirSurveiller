#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
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

void void_openCall_destroy(void* void_oCall){
    openCall_destroy((openCall*) void_oCall);
}

// #############  ExecCall Utils ############## //
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
    this->callBuff = calloc(8, sizeof(*this->callBuff));
    if(!this->callBuff){
        ret = MEMORY_ALLOCATION_ERROR;
        goto freeArgs;
    }
    this->call_num = 0;
    this->callBuff_size = 8;
endfun:
    return ret;

freeArgs:
    zfree(&this->args);
freeCmd:
    zfree(&this->cmdName);
    return ret;
}
void execCall_destroy(execCall* this){
    zfree(&this->args);
    zfree(&this->cmdName);
    for(int i=0; i<this->call_num; i++)
        openCall_destroy(this->callBuff[i]);
    zfree(&this->callBuff);
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
            ret = MEMORY_ALLOCATION_ERROR;
            goto endfun;
        }
        this->size = this->size*2;
    }
    for(; oldsize<this->size; oldsize++)
        this->procs[oldsize] = NULL;
    if(this->procs[call->pid+1]){ //PID already present = old process dead
        ret = ALREADY_EXISTS_IN_CONTAINER;
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
    while(this->size < ocall->pid+1){
        size_t newSize = this->size*2;
        newSize = (newSize <= this->maxPid) ? newSize : this->maxPid;
        execCall** newProcs = realloc(this->procs,
                                      sizeof(*this->procs)*newSize);
        if(!newProcs){
            ret = MEMORY_ALLOCATION_ERROR;
            goto endfun;
        }
        for(; this->size < newSize; this->size++)
            newProcs[this->size] = NULL;
    }

    execCall* target_execCall = this->procs[ocall->pid+1];
    if(target_execCall){
        if(target_execCall->call_num+2==target_execCall->callBuff_size){
            openCall* newCallBuff =
                realloc(target_execCall->callBuff,
                        target_execCall->callBuff_size*2);
            if(!newCallBuff){
               ret = MEMORY_ALLOCATION_ERROR;
               goto endfun;
            }
            target_execCall->callBuff_size *= 2;
        }
    }
    else{
        //Setup dummy process
        execCall newProc;
        newProc.ppid = 0; //Unknown parent
        newProc.args = "unknown";
        newProc.pid = ocall->pid;
        newProc.cmdName = ocall->cmdName;
        newProc.callBuff = calloc(8, sizeof(*newProc.callBuff));
        if(!newProc.callBuff){
            ret = MEMORY_ALLOCATION_ERROR;
            goto endfun;
        }
        newProc.callBuff_size = 8;
        newProc.callBuff[0] = ocall;
        newProc.call_num = 0;
        procindex_add(this, &newProc);
    }
    target_execCall->call_num++;
    target_execCall->callBuff[target_execCall->call_num] = ocall;
endfun:
    return ret;
}


execCall* procindex_retrieve(procindex* this, pid_t target_pid){
    execCall* retCall = NULL;
    if(this->size >= target_pid && this->procs[target_pid+1]){
        retCall = this->procs[target_pid+1];
        this->procs[target_pid+1] = NULL;
    }
    return retCall;
}

// ################# Surveiller ################### //

int surv_init(surveiller* this, const char* opencall_socketaddr,
              const char* execcall_socketaddr,
              const char* ctl_socketaddr){
    int ret = NOMINAL;
    this->opencall_socketaddr = opencall_socketaddr;
    this->execcall_socketaddr = execcall_socketaddr;
    this->ctl_socketaddr = ctl_socketaddr;
    this->ownPID = getpid();
    this->shutting_down = 0;
    this->processing_execcall_socket = 0;
    this->processing_opencall_socket = 0;
    //Filter init
    ret = openCall_filter__init(&this->open_filter);

    //Process Index init
    ret = procindex_init(&this->procs, this->ownPID);

    //Queue init
    ret = g_ringBuffer_init(&this->commandQueue, sizeof(execCall*));

    ret = g_ringBuffer_init(&this->openQueue, sizeof(openCall*));

    ret = g_ringBuffer_init(&this->dispatchQueue, sizeof(execCall*));
    return ret;
}

void surv_destroy(surveiller* this){
    //destroy execCalls
    //destroy Queues
    g_ringBuffer_destroyd(&this->commandQueue, void_execCall_destroy);
    g_ringBuffer_destroyd(&this->openQueue, void_openCall_destroy);
    g_ringBuffer_destroyd(&this->dispatchQueue, void_execCall_destroy);
    procindex_destroy(&this->procs);
    //destroy OpencallFilter
    openCall_filter__destroy(&this->open_filter);
}

/**
 * Handling of incomming opencalls and filtering of them
 */
void* surv_handleOpenCallSocket(void* surv_struct){
    int* ret = malloc(sizeof(*ret));
    *ret = 0;
    int maxtries_enqueue = 3;
    unsigned int seconds_to_wait = 20;
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

    surveiller* surv = (surveiller*) surv_struct;
    if ( (rc_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        *ret = 2;
        goto endfun;
    }
    *ret = sb_init(&sbuf, 512);
    if( *ret ) {
        goto endfun;
    }

    surv->processing_opencall_socket = 1;
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
        *ret = 3;
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

    fd_set rfds;
    //struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(rc_fd, &rfds);

    //tv.tv_sec = 20;
    //tv.tv_usec = 0;
    int curr_field = 0;
    int lfield_start = 0;
    int allocated = 0;

    *ret = sb_init(&fields[OTime],64);
    if(*ret) goto cleanupStringBuffers; allocated++;
    *ret = sb_init(&fields[Cmd],32);
    if(*ret) goto cleanupStringBuffers; allocated++;
    *ret = sb_init(&fields[PID],16);
    if(*ret) goto cleanupStringBuffers; allocated++;
    *ret = sb_init(&fields[Rval],8);
    if(*ret) goto cleanupStringBuffers; allocated++;
    *ret = sb_init(&fields[Path],128);
    if(*ret) goto cleanupStringBuffers; allocated++;

    while(!surv->shutting_down){
        *ret = select(rc_fd + 1, &rfds, NULL, NULL, NULL);
        if(*ret < 0){
            //Error using socket
            break;
        }
        if(*ret){
            rlen = recv(rc_fd, buf, sizeof(buf), 0);
            *ret = NOMINAL;
        }
        else{
            //Timeout
            break;
        }

        if(!rlen){
            printf("No longer recieving data\n");
            break;
        }
        *ret = sb_appendl(&sbuf, buf,rlen);
        if(*ret) break;
        for (int i=0; i<=sbuf.end_pos; i++){
            switch (sbuf.string[i]){
                case '\n':
                    //finish struct
                    if (curr_field == field_num - 1){
                        sbuf.string[i] = '\0';
                        *ret = sb_append(fields+curr_field,
                                        sbuf.string+lfield_start);
                        openCall oCall;
                        switch (openCall_init(&oCall, fields+OTime,
                                            fields+Cmd, fields+PID,
                                            fields+Rval,fields+Path)){
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
                            case NOMINAL:
                                if (openCall_filter__filter(&surv->open_filter,
                                                            &oCall)){
                                    *ret = g_ringBuffer_write(&surv->openQueue,
                                                              &oCall);
                                    int tries = 0;
                                    openCall_print(&oCall);
                                    while( *ret && (tries<maxtries_enqueue)){
                                        *ret = g_ringBuffer_write(
                                                &surv->openQueue,
                                                &oCall);
                                        tries++;
                                        sleep(seconds_to_wait);
                                    }
                                    if(ret){
                                        openCall_destroy(&oCall);
                                        *ret = 4;
                                        goto cleanupStringBuffers;
                                    }
                                }
                                else
                                    openCall_destroy(&oCall);
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
    surv->processing_opencall_socket = 0;
    return ret;

}


/**
 * Handler for incomming execcalls
 */
void* surv_handleExecCallSocket(void* surv_struct){

    int* ret = malloc(sizeof(*ret));
    *ret = 0;
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


    surveiller* surv = (surveiller*) surv_struct;
    if ( (rc_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        *ret = 2;
        goto endfun;
    }
    *ret = sb_init(&sbuf, 512);
    if( *ret ) {
        goto endfun;
    }

    surv->processing_execcall_socket = 1;
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
        *ret = 3;
        goto cleanupRecieveBuffer;
    }

    fd_set rfds;
    //struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(rc_fd, &rfds);

    //tv.tv_sec = 20;
    //tv.tv_usec = 0;
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

    *ret = (sb_init(&fields[OTime],64)
         | sb_init(&fields[Cmd],32)
         | sb_init(&fields[PID],16)
         | sb_init(&fields[PPID],16)
         | sb_init(&fields[Args],128) );
    if(ret)
        goto cleanupStringBuffers;

    while(!surv->shutting_down){
        *ret = select(rc_fd+1, &rfds, NULL, NULL, NULL);
        if(*ret < 0){
            //Error using socket
            break;
        }
        if(*ret){
            rlen = recv(rc_fd, buf, sizeof(buf), 0);
            *ret = NOMINAL;
        }
        else{
            //Timeout
            break;
        }

        if(!rlen){
            printf("No longer recieving data\n");
            break;
        }
        //printf( "\nbuffer: \n%s\n", buf);
        *ret = sb_appendl(&sbuf, buf, rlen);
        //printf( "\nsbuffer: \n%s\n", sbuf.string);
        if(ret) break;
        for (int i=0; i<=sbuf.end_pos; i++){
            switch (sbuf.string[i]){
                case '\n':
                    //finish struct
                    if (curr_field == field_num - 1){
                        sbuf.string[i] = '\0';
                        *ret = sb_append(fields+curr_field,
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
                            case NOMINAL:
                                *ret = g_ringBuffer_write(&surv->commandQueue,
                                                          &eCall);
                                int tries = 0;
                                execCall_print(&eCall);
                                while( *ret && (tries<maxtries_enqueue)){
                                    *ret = g_ringBuffer_write(
                                            &surv->commandQueue,
                                            &eCall);
                                    tries++;
                                    sleep(seconds_to_wait);
                                }
                                if(ret){
                                    execCall_destroy(&eCall);
                                    *ret = 4;
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
    surv->processing_execcall_socket = 0;
    return (void*)ret;
}

void* surv_shutdown(void* surv_struct){
    int* ret = malloc(sizeof(*ret));
    *ret = NOMINAL;
    surveiller* surv = (surveiller*) surv_struct;
    int caught_a_fish;
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals,SIGTERM);
    sigaddset(&signals,SIGINT);
    /* Let this thread kick back, get some quality time alone
     * and let it follow its true passion... fishing SIGNALFISH. */
    sigwait(&signals, &caught_a_fish);
    surv->shutting_down = 1;
    return ret;
}

int surv_addExecCall(surveiller* this, execCall* ecall){
    int ret = NOMINAL;
    ret = procindex_add(&this->procs, ecall);
    if (ret == ALREADY_EXISTS_IN_CONTAINER){
        execCall* retrieved_Ecall;
        retrieved_Ecall = procindex_retrieve(&this->procs, ecall->pid);
        if(!retrieved_Ecall){
            //Could not retrieve eCall
        }
        ret = g_ringBuffer_write(&this->dispatchQueue, retrieved_Ecall);
        ret = procindex_add(&this->procs, ecall);
    }
    return ret;
}

void surv_flush_procindex(surveiller* this){
    execCall* currProc;
    for(size_t i=0; i<this->procs.size; i++){
        currProc = procindex_retrieve(&this->procs, i);
        if(currProc && currProc->tracked){
            g_ringBuffer_write(&this->dispatchQueue, currProc);
        }
    }
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
    char* ctlsocket_addr = "/tmp/opentrace_ctl.socket";
    surveiller surv;
    ret = surv_init(&surv,
                    opensocket_addr,
                    execsocket_addr,
                    ctlsocket_addr);
    if(ret)
        exit(EXIT_FAILURE);

    openCall_filter__add(&surv.open_filter,
            "/home/userofdoom/Berufspraktikum/test");
    //  Blocking Signals in Threads   //

    /* Lock down SIGTERM and maybe other signals so
     * that everything is neat if we need a hasty exit. */
    sigset_t signals2block;
    sigemptyset(&signals2block);
    sigaddset(&signals2block, SIGTERM);
    sigaddset(&signals2block, SIGINT);
    //sigprocmask(SIG_BLOCK, &signals2block, NULL);
    pthread_sigmask(SIG_BLOCK, &signals2block, NULL);

    // Setting up Threads //

    pthread_t threadExecId;
    int* t_exec_ret;
    pthread_t threadOpenId;
    int* t_open_ret;
    pthread_t threadShutdownId;
    ret = pthread_create(&threadExecId, NULL,
                         &surv_handleExecCallSocket, &surv);
    ret = pthread_create(&threadOpenId, NULL,
                         &surv_handleOpenCallSocket, &surv);
    ret = pthread_create(&threadShutdownId, NULL,
                         &surv_shutdown, &surv);

    // Main loop of programm
    int dead = 0;
    int ready_calls = 0;
    int ecall_first = 0;
    int compMode = 0;
    int has_next = 0;
    int needs_dispatch =0;
    int dispatch_batch = 0;
    size_t counter = 1;
    execCall* tempEcall;
    openCall* tempOcall;
    execCall* dispatch_call;
    // Somewhat convoluted logic determining flow control


    while(!dead){
        //Dispatch
        //Sleep to keep CPU Usage sane
        if(!(counter % 128))
           sleep(1);
        if(counter >= 424242)
           surv.shutting_down = 1;
        // Moving through and comparing timestamps on both Queues
        if(compMode){
            ecall_first = timercmp(&tempEcall->time_stamp,
                                   &tempOcall->time_stamp, <=);
            if(ecall_first){
                surv_addExecCall(&surv, tempEcall);
            }
            else
                procindex_opencall_add(&surv.procs, tempOcall);
            has_next =
              ( ecall_first && g_ringBuffer_size(&surv.commandQueue))
              || (!ecall_first && g_ringBuffer_size(&surv.openQueue));

            if(has_next){
                if(ecall_first)
                    g_ringBuffer_read(&surv.commandQueue, ((void**)tempEcall));
                else
                    g_ringBuffer_read(&surv.openQueue, ((void**)tempOcall));
            }
            else{
                if(!ecall_first)
                    surv_addExecCall(&surv, tempEcall);
                else
                    procindex_opencall_add(&surv.procs, tempOcall);
                ready_calls = 0;
                compMode = 0;
            }
        }
        else{

            if(g_ringBuffer_size(&surv.commandQueue))
                ready_calls++;
            if(g_ringBuffer_size(&surv.openQueue)){
                if(ready_calls){
                    g_ringBuffer_read(&surv.commandQueue,(void**) &tempEcall);
                    g_ringBuffer_read(&surv.commandQueue,(void**) &tempOcall);
                    compMode = 1;
                    continue;
                }

                else{
                    needs_dispatch = procindex_opencall_add(&surv.procs,
                                                            tempOcall);
                    if(needs_dispatch){
                        //Move to dispatch queue and try again
                    }
                    ready_calls = 0;
                }
            }
            if(ready_calls){
                procindex_add(&surv.procs, tempEcall);
                if(needs_dispatch){

                }
            }
        }
        dead =    surv.shutting_down && !surv.processing_opencall_socket
               && !surv.processing_execcall_socket
               && g_ringBuffer_empty(&surv.openQueue)
               && g_ringBuffer_empty(&surv.commandQueue);
        if(dead){
            printf("\nExiting watchdog... \n%s\n%s\n",
                  "socket processing -> stopped",
                  "flushing process index and starting final dispatch");
            surv_flush_procindex(&surv);
        }
        if(dead || !(counter % 512)){
            dispatch_batch = g_ringBuffer_size(&surv.dispatchQueue);
            for( int i =0; i<dispatch_batch; i++){
                g_ringBuffer_read(&surv.dispatchQueue,
                                  (void**) &dispatch_call);
                if(!dispatch_call){
                    perror("Dispatch Call doesn't exist!");
                }
                execCall_print(dispatch_call);
                zfree(&dispatch_call);
            }
            printf("\nlast_dispatch:%i;oQueue:%i;eQueue:%i\n%s%s\n%s%s\n",
                    dispatch_batch,
                   g_ringBuffer_size(&surv.openQueue),
                   g_ringBuffer_size(&surv.commandQueue),
                   "openSockethandler:",
                   surv.processing_opencall_socket ? "running":"exited",
                   "execSockethandler:",
                   surv.processing_execcall_socket ? "running":"exited" );
        }
        counter++;
    }
    printf("Flush -> Done\nFinal Dispatch -> Done\n\nGOODBYE\n");
    pthread_join(threadExecId,(void**)&t_exec_ret);
    pthread_join(threadOpenId,(void**)&t_open_ret);
    printf("ExecHandler returned: %i\n OpenHandler returned: %i\n",
            *t_exec_ret, *t_open_ret);
    surv_destroy(&surv);
    exit(EXIT_SUCCESS);
}
