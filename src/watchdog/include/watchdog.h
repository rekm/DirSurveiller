#ifndef _WATCHDOG_H
#define _WATCHDOG_H

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>

#include "art.h"
#include "containers.h"

#define NOMINAL 0

// ######### General Utillity stuff ########### //
int get_max_PID();


/**
 * OpenCall
 *  ->timestamp
 *  ->flag
 *  ->char* filepath
 *  ->inode
 */
typedef struct {
    pid_t pid;
    int flag;
    char* cmdName;
    char* filepath;
    __INO_T_TYPE inode;
    struct timeval time_stamp;
} openCall;

int openCall_init(openCall* this,
                  stringBuffer* openTimeStamp,
                  stringBuffer* cmdName,
                  stringBuffer* pid,
                  stringBuffer* returnVal,
                  stringBuffer* Path);

void openCall_destroy(openCall* this);

void openCall_print(openCall* this);



// ############ ExecCall functions ################# //

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
    pid_t ppid;
    int deadness;
    int tracked;
    int call_num;
    char *cmdName;
    char *args;
    size_t callBuff_size;
    openCall **callBuff;
    struct timeval time_stamp;
    struct timeval ptime_stamp;
} execCall;


int execCall_init(execCall* this,
                  stringBuffer* execTimeStamp,
                  stringBuffer* cmdName,
                  stringBuffer* pid,
                  stringBuffer* ppid,
                  stringBuffer* Arg);

void execCall_destroy(execCall* this);

void execCall_print(execCall* this);

// ##################  OpenCall Filter ################### //

/**
 * @brief a struct responsible for mananging an opencall_filter
 *
 * @member filter_trie: An art_tree* for an art_tree struct that
 *                      comes from libart. This is an adaptive radix tree
 *                      that is a cool prefix tree datastructure that is
 *                      used to keep track of the filtered filepath prefixes
 */
typedef struct {
    art_tree filter_trie;
}openCall_filter;
// initilization of filter
int openCall_filter__init(openCall_filter* this);
// destruction of filter
void openCall_filter__destroy(openCall_filter* this);

int openCall_filter__filter(openCall_filter* this, openCall* openCall);
int openCall_filter__add(openCall_filter* this, const char* path);



// #################  Procindex ################ //

/**
 * @brief an index structure that is storing execCalls and allows
 *        access by pid
 *
 * @member procs (ExecCall*): Dynamic Array of ExeCall*
 *
 */
typedef struct {
    execCall** procs;
    pid_t size;
    pid_t maxPid;
} procindex;
/**
 * @brief Initialization of procindex struct
 */
int procindex_init(procindex* this, pid_t guide_pid);

/**
 * @brief Destructor of procindex struct
 */
void procindex_destroy(procindex* this);

/**
 * @brief adds process in form of an execCall to the index
 */
int procindex_add(procindex* this, execCall* call);

/**
 * @brief retrieves one execCall pointer and leaves a NULL
 */
execCall* procindex_retrieve(procindex* this, pid_t target_pid);

int procindex_delete(procindex* this, pid_t target_pid);

// ############### Surveiller struct ############### //

/**
 *  We need some construct organizing relevant information.
 *  Status:
 *      @member is_shutting_down:
 *          true, if in the process of shutting down
 *      @member is_processing_
 *        execcall_socket:
 *          true, while execalls are being processed
 *      @member is_processing_opencall_socket:
 *          true, while opencalls are being processed
 *  SocketAdresses:
 *      @member opencall_socketaddr (cstring):
 *          Path to UnixDomain socket for retrieval of Opencalls
 *      @member execcall_socketaddr (cstring):
 *          Path to UnixDomain socket for retrieval of ExecCalls
 *  Queues:
 *      @member commandQueue (g_ringbuffer*):
 *          Queue implementation using a general Ringbuffer implementation
 *          that stores ExecCalls aggregated by a socket handler thread
 *      @member openQueue (g_ringbuffer*):
 *          Queue that stores OpenCalls, aggregated by a socket handler thread
 *
 *  Dynamic array of PID resolved processes
 *  managed with specialized put command
 *
 *  Queue of filtered OpenEvents with time stamps
 *  Queue of ExecEvents with time stamps
 *
 */
typedef struct {
    //Status
    int restart;
    int killpipe_fd[2];
    int shutting_down;
    int processing_execcall_socket;
    int processing_opencall_socket;
    FILE* mainLog_fp;
    FILE* openCallLog_fp;
    FILE* execCallLog_fp;
    FILE* ctrlLog_fp;
    pid_t ownPID;
    unsigned boot_time;
    char status_msg[200];
    pthread_t* shutdownThreadId;
    const char* opencall_socketaddr;
    const char* execcall_socketaddr;
    const char* ctl_socketaddr;
    procindex procs;
    openCall_filter open_filter;
    g_ringBuffer dispatchQueue;
    g_ringBuffer commandQueue;
    g_ringBuffer openQueue;
    db_manager db_man;
} surveiller;

/**
 * @brief initialization of surveiller struct
 *
 * Initializes surveiller struct and populates
 * address fields.
 *
 * @param this: surveiller to be initialized
 * @param opencall_socketaddr: OpenCall socket address as string
 * @param execcall_socketaddr: ExecCall -II-
 *
 * @return: 0, if success
 */
int surv_init(surveiller* this, const char* opencall_socketaddr,
              const char* execcall_socketaddr,
              const char* ctl_socketaddr,
              pthread_t* shutdownThread,
              const char* database_dir);
void surv_destroy(surveiller* this);

int surv_check_running(surveiller* this);

// ############### Thread functions ################## //

/**
 * @brief: handles ExecCall Socket and populates ExecCall Queue
 *
 * This thread function sets up and listens to the execCall
 * Socket, specified in the suveiller struct. It parses the
 * socket output, constructs execCall structs and populates
 * the commandQueue of the surveiller with them.
 *
 * @param surv_struct: surveiller struct pointer casted to void*
 *                     of which the corresponding struct will be modified
 * @return: 0, if exited normally
 */
void* surv_handleExecCallSocket(void* surv_struct);


/**
 * @brief: handles OpenSocket and populates OpenCall Queue
 *
 * This thread function sets up and listens to the openCall
 * Socket, specified in the suveiller struct. It parses the
 * socket output, constructs openCallstructs and writes them into
 * the openCallQueue of the surveiller struct.
 * Currently uses a adaptive radix tree for filtering.
 *
 * @param surv_struct: surveiller struct pointer casted to void*
 *                     of which the corresponding struct will be modified
 * @return: 0, if exited normally
 */
void* surv_handleOpenCallSocket(void* surv_struct);

/**
 * @brief: Handles unprivaliged access to deamon
 *
 * threadfunction that listens on a socket for maintanance events
 *  - adding and removing of directories/files from the openCall_filter
 *  - shutting down of the deamon
 *  - checking if directory/file is being tracked
 *
 * @param surv_struct: surveiller struct pointer casted to (void*)
 *
 * @returns: 0 ,if it exited normally
 *           1 ,if memory allocation error occured
 */
void* surv_handleAccess(void* surv_struct);

void* surv_shutdown(void* surv_struct);


int surv_dispatch(surveiller* this);


#endif
