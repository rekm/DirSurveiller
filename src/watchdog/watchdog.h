#ifndef _WATCHDOG_H
#define _WATCHDOG_H

#include <sys/types.h>
#include <sys/time.h>

#include "containers.h"

#define NOMINAL 0


/**
 * OpenCall
 *  ->timestamp
 *  ->flag
 *  ->char* filepath
 *  ->inode
 */
typedef struct {
    struct timeval time_stamp;
    pid_t pid;
    int flag;
    char* cmdName;
    char* filepath;
    __INO_T_TYPE inode;
} openCall;

int openCall_init(openCall* this,
                  stringBuffer* openTimeStamp,
                  stringBuffer* cmdName,
                  stringBuffer* pid,
                  stringBuffer* returnVal,
                  stringBuffer* Path);

void openCall_destroy(openCall* this);

void openCall_print(openCall* this);

/**
 * Process
 *  ->tracked
 *  ->pid
 *  ->start_time
 *  ->char* pname Process Name
 *  ->openCall *openCall
 *
 */

typedef struct {
    pid_t pid;
    int tracked;
    pid_t ppid;
    struct timeval time_stamp;
    char *cmdName;
    char *args;
    int call_num;
    openCall *calls;
} execCall;


int execCall_init(execCall* this,
                  stringBuffer* execTimeStamp,
                  stringBuffer* cmdName,
                  stringBuffer* pid,
                  stringBuffer* ppid,
                  stringBuffer* Arg);

void execCall_destroy(execCall* this);

void execCall_print(execCall* this);
/*
 *  We need some construct storing relevant information.
 *  Struct storing
 *
 *  Dynamic array of PID resolved processes
 *  managed with specialized put command
 *
 *  Queue of filtered OpenEvents with time stamps
 *  Queue of ExecEvents with time stamps
 *
 */

typedef struct {
    //db_dispatch_handle
    //process** dispatchtrees
    char* opencall_socketaddr;
    char* execcall_socketaddr;
    execCall* procs;
    g_ringBuffer* commandQueue;
    g_ringBuffer* openQueue;
} surveiller;


int surv_init(surveiller*, const char* opencall_socketaddr,
              const char* execcall_socketaddr);

int surv_check_running(surveiller* watchdog);
int surv_handleExecCallSocket(void* surv_struct);
int surv_handleOpenCallSocket(void* surv_struct);
int surv_construct_dispatchtrees(surveiller* watchdog);
int surv_dispatch(surveiller* watchdog);
#endif
