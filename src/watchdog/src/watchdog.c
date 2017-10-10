#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
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
#include "DirSurveillerConfig.h"

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
    this->inode = 0;
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
    this->flag = 0;
    this->flag = atoi(returnVal->string);
    this->pid = 0;
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


void openCallPtr_destroy(openCall* this){
    zfree(&this->filepath);
    zfree(&this->cmdName);
    zfree(&this);
}

void void_openCallPtr_destroy(void* void_oCall){
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
    this->deadness = 0;
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
    this->tracked = 0;
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
    for(int i=0; i<this->call_num; i++){
        openCall_destroy(this->callBuff[i]);
        zfree(&this->callBuff[i]);
    }
    zfree(&this->callBuff);
}
void void_execCall_destroy(void* void_exec_ptr){
    execCall_destroy((execCall*)void_exec_ptr);
}

void execCallPtr_destroy(execCall* this){
    zfree(&this->args);
    zfree(&this->cmdName);
    for(int i=0; i<this->call_num; i++){
        openCall_destroy(this->callBuff[i]);
        zfree(&this->callBuff[i]);
    }
    zfree(&this->callBuff);
    zfree(&this);
}
void void_execCallPtr_destroy(void* void_exec_ptr){
    execCallPtr_destroy((execCall*)void_exec_ptr);
}

void execCall_print(execCall* this){
    printf("%s-> pid: %i ppid: %i time: %li.%li\n args: %s\n tracked: %i\n"
           " openCalls: %i\n",
          this->cmdName, this->pid, this->ppid, this->time_stamp.tv_sec,
          this->time_stamp.tv_usec, this->args, this->tracked, this->call_num);
    for(int i=0; i<this->call_num; i++)
       openCall_print(this->callBuff[i]);
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
        this->size = this->maxPid+1;
    else
        this->size =
            ((guide_pid+1)*2 < this->maxPid)? (guide_pid+1)*2:this->maxPid+1;
    this->procs = calloc(this->size, sizeof(*this->procs));
    if(!this->procs){
        ret = -1;
        this->size = 0;
    }
endfun:
    return ret;
}

void procindex_destroy(procindex* this){
    for(pid_t i = 0; i<this->size; i++){
        if(this->procs[i]){
            execCall_destroy(this->procs[i]);
            zfree(&this->procs[i]);
        }
    }
    zfree(&this->procs);
}

int procindex_add(procindex* this, execCall* call){
    int ret = NOMINAL;
    //execCall_print(call);
    pid_t oldsize = this->size;
    if( (call->pid < 0) || (call->pid > this->maxPid) ){
        ret = -1;
        execCall_print(call);
        goto endfun;
    }

    while (call->pid+1 > this->size){
        this->size =
               (this->size*2 <= this->maxPid+1) ? this->size*2 : this->maxPid+1;
        execCall** newExec = realloc(this->procs,
                                     this->size*sizeof(*this->procs));
        if(!newExec){
            ret = MEMORY_ALLOCATION_ERROR;
            goto endfun;
        }
        this->procs = newExec;
        for(; oldsize<this->size; oldsize++)
            this->procs[oldsize] = NULL;
    }
    if(this->procs[call->pid+1]){ //PID already present = old process dead
        ret = ALREADY_EXISTS_IN_CONTAINER;
        goto endfun;
    }
    if(this->size < (call->ppid+1) || !this->procs[call->ppid+1])
        call->ppid = 0;
    this->procs[call->pid+1] = call;

endfun:
    return ret;
}

void procindex_trackparents(procindex* this, pid_t child){
    pid_t parentID = this->procs[child+1]->ppid;
    execCall* parentProc = this->procs[parentID+1];
    if(parentID && parentProc && !parentProc->tracked){
        parentProc->tracked = 1;
        procindex_trackparents(this, parentID);
    }
}

int procindex_opencall_add(procindex* this, openCall* ocall){
    int ret = NOMINAL;
    if( ocall->pid < 0 || (this->maxPid < ocall->pid) ){
        ret = -1;
        openCall_print(ocall);
        goto endfun;
    }
    //openCall_print(ocall);
    while(this->size < ocall->pid+1){
        pid_t newSize = this->size*2;
        newSize = (newSize <= this->maxPid+1) ? newSize : this->maxPid+1;
        execCall** newProcs = realloc(this->procs,
                                      sizeof(*this->procs)*newSize);
        if(!newProcs){
            ret = MEMORY_ALLOCATION_ERROR;
            goto endfun;
        }
        for(; this->size < newSize; this->size++)
            newProcs[this->size] = NULL;
        this->procs = newProcs;
    }

    execCall* target_execCall =
        (this->size < ocall->pid+1) ? NULL : this->procs[ocall->pid+1];
    if(target_execCall){
        if(target_execCall->call_num+2==target_execCall->callBuff_size){
            openCall** newCallBuff =
                realloc(target_execCall->callBuff,
                        target_execCall->callBuff_size*2
                        * sizeof(*target_execCall->callBuff));
            if(!newCallBuff){
               ret = MEMORY_ALLOCATION_ERROR;
               goto endfun;
            }
            target_execCall->callBuff = newCallBuff;
            target_execCall->callBuff_size *= 2;
        }
    }
    else{
        //Setup dummy process
        execCall* newProc = malloc(sizeof(*newProc));
        *newProc = (execCall) {0};
        if(!newProc)
            goto freeNewProc;
        newProc->ppid = 0; //Unknown parent
        newProc->args = malloc(sizeof(char)*8);
        if(!newProc->args)
            goto freeNewProcArgs;
        strcpy(newProc->args,"unknown");
        newProc->tracked = 0;
        newProc->pid = 0;
        newProc->pid = ocall->pid;
        newProc->cmdName = malloc(sizeof(char)*(strlen(ocall->cmdName)+1));
        if(!newProc->cmdName)
            goto freeNewProcName;
        strcpy(newProc->cmdName, ocall->cmdName);
        newProc->callBuff = calloc(8, sizeof(*newProc->callBuff));
        if(!newProc->callBuff){
freeNewProcName:
            zfree(&newProc->cmdName);
freeNewProcArgs:
            zfree(&newProc->args);
freeNewProc:
            zfree(&newProc);
            ret = MEMORY_ALLOCATION_ERROR;
            goto endfun;
        }
        newProc->callBuff_size = 8;
        newProc->call_num = 0;
        procindex_add(this, newProc);
        target_execCall = this->procs[ocall->pid+1];

    }
    target_execCall->callBuff[target_execCall->call_num++] = ocall;
    if(!target_execCall->tracked){
        target_execCall->tracked = 1;
        procindex_trackparents(this, target_execCall->pid);
    }
endfun:
    return ret;
}


execCall* procindex_retrieve(procindex* this, pid_t target_pid){
    execCall* retCall = NULL;
    if((this->size >= (target_pid+1)) && this->procs[target_pid+1]){
        retCall = this->procs[target_pid+1];
        this->procs[target_pid+1] = NULL;
    }
    return retCall;
}

// ################# Surveiller ################### //

int surv_init(surveiller* this, const char* opencall_socketaddr,
              const char* execcall_socketaddr,
              const char* ctl_socketaddr,
              pthread_t* shutdownThread){
    int ret = NOMINAL;
    sprintf(this->status_msg,"starting up...\n");
    this->opencall_socketaddr = opencall_socketaddr;
    this->execcall_socketaddr = execcall_socketaddr;
    this->ctl_socketaddr = ctl_socketaddr;
    this->ownPID = getpid();
    this->shutdownThreadId = shutdownThread;
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
    if (pipe(this->killpipe_fd) == -1) {
        perror("could not create kill_pipe");
        ret = -1;
    }
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
    close(this->killpipe_fd[0]);
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
    stringBuffer sbuf = (stringBuffer) {0};

    int field_num = 5; // Number of fields that need parsing
    enum FieldDesc { OTime, Cmd, PID, Rval, Path};
    // Array of dynamic StringBuffers for fields
    stringBuffer fields[field_num];
    //for(int i = 0; i<field_num; i++)
    //    fields[i] = (stringBuffer) {0};

    int rc_fd; // recieve file descriptor
    ssize_t rlen = 0;
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
        goto cleanupRecieveBuffer;
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
    FD_SET(surv->killpipe_fd[0], &rfds);
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
                        openCall* oCall = malloc(sizeof(*oCall));
                        *oCall = (openCall) {0};
                        switch (openCall_init(oCall, fields+OTime,
                                            fields+Cmd, fields+PID,
                                            fields+Rval,fields+Path)){
                            case 1:
                                //Memory error => Panic
                                perror("ToDo handle memory Error");
                                openCall_destroy(oCall);
                                zfree(&oCall);
                                break;
                            case 2:
                                perror("Handle misalignment of fields");
                                openCall_destroy(oCall);
                                zfree(&oCall);
                                break;
                            case -1:
                                //perror("Ocall missing something");
                                //Handle case
                            case NOMINAL:
                                if (openCall_filter__filter(&surv->open_filter,
                                                            oCall)){
                                    *ret = g_ringBuffer_write(&surv->openQueue,
                                                              oCall);
                                    int tries = 0;
                                    while( *ret && (tries<maxtries_enqueue)){
                                        *ret = g_ringBuffer_write(
                                                &surv->openQueue,
                                                oCall);
                                        tries++;
                                        sleep(seconds_to_wait);
                                    }
                                    if(*ret){
                                        openCall_destroy(oCall);
                                        zfree(&oCall);
                                        *ret = 4;
                                        goto cleanupStringBuffers;
                                    }
                                    //openCall_print(oCall);
                                }
                                else{
                                    openCall_destroy(oCall);
                                    zfree(&oCall);
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
    close(rc_fd);
cleanupRecieveBuffer:
    sb_destroy(&sbuf);

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
    stringBuffer sbuf = (stringBuffer) {0};
    int field_num = 5; // Number of fields that need parsing
    enum FieldDesc { OTime, Cmd, PID, PPID, Args};
    // Array of dynamic StringBuffers for fields
    stringBuffer fields[field_num];
    int rc_fd; // recieve file descriptor
    ssize_t rlen = 0;
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
    FD_SET(surv->killpipe_fd[0], &rfds);

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
    if(*ret)
        goto cleanupStringBuffers;

    while(!surv->shutting_down){
        *ret = select(rc_fd+2, &rfds, NULL, NULL, NULL);
        if(*ret < 0){
            //Error using socket
            break;
        }
        if(*ret /*&& FD_ISSET(rc_fd, &rfds)*/){
            rlen = recv(rc_fd, buf, sizeof(buf), 0);
            debug_print("%s\n","Recieved");
            //*ret = NOMINAL;
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
        //fflush_unlocked(stdout);
        *ret = sb_appendl(&sbuf, buf, rlen);
        //printf( "\nsbuffer: \n%s\n", sbuf.string);
        if(*ret) break;
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
                        execCall* eCall = malloc(sizeof(*eCall));
                        //execCall eCall;
                        switch (execCall_init(eCall, fields+OTime,
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
                                                          eCall);
                                int tries = 0;
                                //execCall_print(eCall);
                                fflush(stdout);
                                while( *ret && (tries<maxtries_enqueue)){
                                    *ret = g_ringBuffer_write(
                                            &surv->commandQueue,
                                            eCall);
                                    tries++;
                                    sleep(seconds_to_wait);
                                }
                                if(*ret){
                                    execCall_destroy(eCall);
                                    zfree(&eCall);
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
    close(rc_fd);
cleanupRecieveBuffer:
    sb_destroy(&sbuf);
endfun:
    printf("ExecCallSocket_offline:%i\n",*ret);
    surv->processing_execcall_socket = 0;
    return ret;
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
    printf("soft shutdown initiated...\n");
    surv->shutting_down = 1;
    //Free blocking selects
    close(surv->killpipe_fd[1]);
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
    for(size_t i=0; i<this->procs.size-1; i++){
        currProc = procindex_retrieve(&this->procs, i);
        if(currProc && currProc->tracked){
            g_ringBuffer_write(&this->dispatchQueue, currProc);
        }
        else if(currProc){
            execCall_destroy(currProc);
            zfree(&currProc);
        }
    }
}


void surv_check_proc_vitals(surveiller* this){
    execCall* currProc;
    int needsDispatch;
    for(size_t i=0; i<this->procs.size; i++){
        currProc = this->procs.procs[i];
        if (currProc){
            needsDispatch = ((kill(currProc->pid,0) < 0) && (errno == ESRCH));
            if(needsDispatch && currProc->tracked){
                g_ringBuffer_write(&this->dispatchQueue, currProc);
                this->procs.procs[i] = NULL;
            }
            else if(needsDispatch && (currProc->deadness > 2)){
                execCall_destroy(currProc);
                zfree(&this->procs.procs[i]);
            }
            else if(needsDispatch)
                currProc->deadness++;

        }
    }
}



void surv_handleRequest(surveiller* this, char* request, char* answer){
    int ret = NOMINAL;
    char* command = strtok(request,"$");
    char* args = strtok(NULL,"$");
    if(!strcmp(command, "shutdown")){
        if(!this->shutting_down){
            ret = pthread_kill(*this->shutdownThreadId, SIGINT);
            if(ret){
                sprintf(answer,
                        "tried signaling termination and failed!\n"
                        "pthread_kill returned: %i\n", ret);
            }
            else
                sprintf(answer,"shutting watchdog down...\n");
        }
        else
            sprintf(answer,"already in the process of shutting down!\n");
    }
    else if(!strcmp(command, "get_status")){
        sprintf(answer,"%s",this->status_msg);
    }
    else if(!strcmp(command, "add_filter")){
        if(args){
            ret = openCall_filter__add(&this->open_filter, args);
            if(ret){
               sprintf(answer, "Error adding\n %s\nto filter list",
                       args);
            }
            else{
                sprintf(answer, "Added\n %s\n to filter list",args);
            }
        }
        else{
           sprintf(answer, "Error: Need something to add to filter list!\n");
           ret = 2;
        }
    }
    else if(!strcmp(command, "remove_filter")){
        if(args){
            ret = 1;
            if(ret){
               sprintf(answer, "Error removing\n %s\nfrom filter list",
                       args);
            }
            else{
                sprintf(answer, "Removed\n %s\nfrom filter list", args);
            }
        }
        else{
           sprintf(answer, "Error: Need something to add to filter list!\n");
           ret = 2;
        }
    }
    else{
        sprintf(answer, "Request not recognized!!");
        ret = -1;
    }
}




void* surv_handleAccess(void* void_surv_struct){
    int* ret = malloc(sizeof(*ret));
    *ret = NOMINAL;
    surveiller* surv = (surveiller*) void_surv_struct;
    int rc_fd; // recieve file descriptor
    int newcon_fd;
    ssize_t rlen = 0;
    struct sockaddr_un addr;
    fd_set op_sockets;
    fd_set rd_sockets;
    int fd_size;
    char buf[256]; //buffer for recieve call
    char answer[256];
    if ( (rc_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        *ret = 2;
        goto endfun;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if(*surv->execcall_socketaddr == '\0'){
        *addr.sun_path = '\0';
        strncpy( addr.sun_path + 1,
                surv->ctl_socketaddr + 1, sizeof(addr.sun_path) - 2);
    } else {
        strncpy(addr.sun_path,
                surv->ctl_socketaddr, sizeof(addr.sun_path) -1);
    }
    unlink(surv->ctl_socketaddr);
    int reuse = 1;
    if(setsockopt(rc_fd, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse))){
        fprintf(stderr, "Reuse failed with code %i\n",errno);
        *ret = -1;
        goto endfun;
    }

    if (bind(rc_fd, (struct sockaddr*)&addr,
             sizeof(addr)) == -1) {
        perror( "bind of open_address failed");
        *ret = -1;
        goto endfun;
    }

    if (listen(rc_fd, 10) == -1) {
       perror( "Can't set listen mode in open socket");
       *ret = -1;
       goto endfun;
    }
    fd_size = getdtablesize();
    FD_ZERO(&op_sockets);
    FD_SET(rc_fd, &op_sockets);
    FD_SET(surv->killpipe_fd[0], &op_sockets);

    while(!surv->shutting_down){
        rd_sockets = op_sockets;
        *ret = select(fd_size, &rd_sockets, NULL, NULL, NULL);

        if(FD_ISSET(rc_fd, &rd_sockets)){
            /*Check if we need to add this new connection
             *to the set of open sockets */
            newcon_fd = accept(rc_fd, NULL, NULL);
            if(newcon_fd >= 0)
                FD_SET(newcon_fd, &op_sockets);
            continue;
        }
        for(int i = 0; i<fd_size; i++){
            if( i!=rc_fd && FD_ISSET(i, &rd_sockets)){
                rlen = read(i, buf, sizeof(buf));
                if(!rlen){
                    close(i);
                    FD_CLR(i,&op_sockets);
                }
                else{
                    surv_handleRequest(surv, buf, answer);
                    write(i, answer, strlen(answer)+1);
                }
            }
        }
    }
    close(rc_fd);
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



int main(int argc, char** argv){
    int max_pid;
    int ret = NOMINAL;
    max_pid = get_max_PID();
    fprintf(stdout,"\n%s Version %d.%d\n",
            argv[0],
            DirSurveiller_VERSION_MAJOR,
            DirSurveiller_VERSION_MINOR);
    if (max_pid == -1) {
        fprintf(stderr, "Could not read /proc/sys/kernel/pid_max\n");
        exit(EXIT_FAILURE);
    }
    debug_print( "Max PID: %i\n",max_pid);
    printf("connecting to sockets: ...\n");
    char* opensocket_addr = "/tmp/opentrace_opencalls";
    char* execsocket_addr = "/tmp/opentrace_execcalls";
    char* ctlsocket_addr = "/tmp/opentrace_ctl.socket";

    //Preparing thread ids and return values//
    pthread_t threadExecId;
    int* t_exec_ret;
    pthread_t threadOpenId;
    int* t_open_ret;
    pthread_t threadShutdownId;
    int* t_shutdown_ret;
    pthread_t threadControlId;
    int* t_ctrl_ret;
    // Initializing surveiller struct //
    surveiller surv;
    ret = surv_init(&surv,
                    opensocket_addr,
                    execsocket_addr,
                    ctlsocket_addr,
                    &threadShutdownId);
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
    pthread_sigmask(SIG_BLOCK, &signals2block, NULL);

    // Creating Threads //

    //t_exec_ret = surv_handleExecCallSocket(&surv);
    ret = pthread_create(&threadExecId, NULL,
                         &surv_handleExecCallSocket, &surv);
    ret = pthread_create(&threadOpenId, NULL,
                         &surv_handleOpenCallSocket, &surv);
    ret = pthread_create(&threadShutdownId, NULL,
                         &surv_shutdown, &surv);
    ret = pthread_create(&threadControlId, NULL,
                         &surv_handleAccess, &surv);
    // Main loop of programm
    int dead = 0;
    int ready_calls = 0;
    int ecall_first = 0;
    int compMode = 0;
    int has_next = 0;
    int dispatch_batch = 0;
    size_t counter = 1;
    execCall* tempEcall;
    openCall* tempOcall;
    execCall* dispatch_call;
    // Somewhat convoluted logic determining flow control

    while(!dead){
        //Dispatch
        //Sleep to keep CPU Usage sane
        if(!(counter % 256))
           sleep(1);
        if(counter >= 424242)
           surv.shutting_down = 1;
        //Check if processes are dead if you haven't got anything better to do
        if(!(counter % 1024))
        //   && g_ringBuffer_empty(&surv.commandQueue)
        //   && g_ringBuffer_empty(&surv.openQueue))
                surv_check_proc_vitals(&surv);
        // Moving through and comparing timestamps on both Queues
        if(compMode){
            ecall_first = timercmp(&tempEcall->time_stamp,
                                   &tempOcall->time_stamp, <=);
            if(ecall_first){
                surv_addExecCall(&surv, tempEcall);
                //tempEcall = NULL;
            }
            else{
                procindex_opencall_add(&surv.procs, tempOcall);
                //tempOcall = NULL;
            }
            has_next =
              ( ecall_first && g_ringBuffer_size(&surv.commandQueue))
              || (!ecall_first && g_ringBuffer_size(&surv.openQueue));

            if(has_next){
                if(ecall_first)
                    g_ringBuffer_read(&surv.commandQueue, ((void**)&tempEcall));
                else
                    g_ringBuffer_read(&surv.openQueue, ((void**)&tempOcall));
            }
            else{
                if(!ecall_first){
                    //execCall_print(tempEcall);
                    surv_addExecCall(&surv, tempEcall);
                    //tempEcall=NULL;
                }
                else{
                    //openCall_print(tempOcall);
                    ret = procindex_opencall_add(&surv.procs, tempOcall);
                    //tempOcall=NULL;
                }
                ready_calls = 0;
                compMode = 0;
            }
        }
        else{

            if(g_ringBuffer_size(&surv.commandQueue))
                ready_calls = 1;
            if(g_ringBuffer_size(&surv.openQueue)){
                if(ready_calls){
                    g_ringBuffer_read(&surv.commandQueue,(void**) &tempEcall);
                    g_ringBuffer_read(&surv.openQueue,(void**) &tempOcall);
                    //execCall_print(tempEcall);
                    //openCall_print(tempOcall);
                    compMode = 1;
                    continue;
                }

                else{
                    g_ringBuffer_read(&surv.openQueue,(void**) &tempOcall);
                    //openCall_print(tempOcall);
                    procindex_opencall_add(&surv.procs,
                                           tempOcall);
                    ready_calls = 0;
                }
            }
            if(ready_calls){
                g_ringBuffer_read(&surv.commandQueue,(void**) &tempEcall);
                procindex_add(&surv.procs, tempEcall);
                ready_calls = 0;
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
                printf("\n");
                execCall_destroy(dispatch_call);
                zfree(&dispatch_call);
            }
            sprintf(surv.status_msg,
                    "\nlast_dispatch:%i;oQueue:%i;eQueue:%i\n%s%s\n%s%s\n",
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
    pthread_join(threadShutdownId,(void**)&t_shutdown_ret);
    pthread_join(threadControlId,(void**)&t_ctrl_ret);
    printf("ExecHandler returned: %i\nOpenHandler returned: %i\n",
            *t_exec_ret, *t_open_ret);
    surv_destroy(&surv);
    zfree(&t_shutdown_ret);
    zfree(&t_exec_ret);
    zfree(&t_open_ret);
    zfree(&t_ctrl_ret);
    exit(EXIT_SUCCESS);
}
