/** @file watchdog.h
 *  @brief Prototypes and structs for the watchdog daemon
 *
 *  @author Rene Kmieciski
 *  @bug All of them!
 */

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
 * @brief ExecCall of Process
 *
 */
typedef struct {
    pid_t pid; /**< PID of Process*/
    pid_t ppid; /**< Parent PID of Process*/
    int deadness; /**< if set to 1 incrases until removal threshold reached*/
    int tracked; /**< Flag set to 1 if it is (remotely) associated with an opencall*/
    int call_num; /**< Number of opencalls associated */
    char *cmdName; /**< Name of command*/
    char *args;     /**< Command line arguments of command*/
    size_t callBuff_size;       /**< Size of openCall Buffer of this struct*/
    openCall **callBuff;        /**< Buffer of Opencalls*/
    struct timeval time_stamp;  /**< TimeStamp in seconds and nanoseconds */
    struct timeval ptime_stamp; /**< TimeStamp (S.Ns) of parent process*/
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
 /**< An art_tree* for an art_tree struct that
 *    comes from libart. This is an adaptive radix tree
 *    that is a cool prefix tree datastructure that is
 *    used to keep track of the filtered filepath prefixes*/
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
 *  @brief Main struct encapsulating the state of the daemon
 *
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
    int restart; /**< if flag is one the daemon will atempt an restart
                  *   when shutting down */
    int killpipe_fd[2]; /**< Pipe used by shutdown thread to unblock
                         *   parser threads that are waiting for io/data.
                         *   Otherwise shutdown would hang until data was
                         *   recieved over socket*/
    int shutting_down; /**< Flag set by shutdown thread, incase of shutdown*/
    int processing_execcall_socket; /**< Flag signaling if execCalls are being
                                     *   processed from socket input*/
    int processing_opencall_socket; /**< Flag signaling if execCalls are being
                                     *   processed from socket input*/
    FILE* mainLog_fp;/**< Filepointer used by the main thread to log events*/
    FILE* openCallLog_fp;/**< Filepointer used by the openCall parse thread
                          *   to log events*/
    FILE* execCallLog_fp;/**< Fielpointer used by the execCall parse thread
                          *   to log events*/
    FILE* ctrlLog_fp;/**< Filepointer used by the control thread to log events*/
    pid_t ownPID;/**< PID of Process initializing this struct*/
    unsigned boot_time;/**< Boottime of System used to convert timestamps to
                        *   epoch time */
    char status_msg[200];/**< Status message prepared by main thread to be
                          *   send out, if status is queried by controlling
                          *   procs*/
    pthread_t* shutdownThreadId;/**<ThreadId of shutdown thread is used to
                                 *  signal the thread. More importantly to
                                 *  intitiate shutdown procedures*/
    const char* opencall_socketaddr;/**< Path to opencall socket */
    const char* execcall_socketaddr;/**< Path to execCall socket */
    const char* ctl_socketaddr;/**< Path to control socket */
    procindex procs;/**< An dynamic index of running Proccesses*/
    openCall_filter open_filter;/**< A filter for opencalls used by the parser*/
    g_ringBuffer dispatchQueue;/**< Ringbuffer queue for exec- openCall parcel
                                *   ready for archival in database*/
    g_ringBuffer commandQueue;/**<  Ringbuffer queue that temporarily stores
                               *    parsed execCalls*/
    g_ringBuffer openQueue;/**< RingBuffer queue that temporarily stores
                            *   parsed openCalls*/
    db_manager db_man;/**< state and management struct
                       *   for database interactions*/
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
 * @retval 0 SUCCESS
 */
int surv_init(surveiller* this, const char* opencall_socketaddr,
              const char* execcall_socketaddr,
              const char* ctl_socketaddr,
              pthread_t* shutdownThread,
              const char* database_dir);
/**
 * @brief destructor of surveiller
 *
 * @warning This causes the complete shutdown of all operations
 */
void surv_destroy(surveiller* this);

int surv_check_running(surveiller* this);

// ############### Thread functions ################## //

/**
 * @brief Thread function Handling ExecCall Socket
 *        and populating ExecCall Queue
 *
 * This thread function sets up and listens to the execCall
 * Socket, specified in the suveiller struct. It parses the
 * socket output, constructs execCall structs and populates
 * the commandQueue of the surveiller with them.
 * @param surv_struct surveiller struct pointer casted to void*
 *                     of which the corresponding struct will be modified
 * @retval 0 SUCCESS
 * @retval 1 MEMORY ALLOCATION ERROR
 * @retval 3 NO CONNECTION TO SOCKET
 */
void* surv_handleExecCallSocket(void* surv_struct);


/**
 * @brief Thread function handling OpenCall Socket
 *        and populating OpenCall Queue
 *
 * This thread function sets up and listens to the openCall
 * Socket, specified in the suveiller struct. It parses the
 * socket output, constructs openCallstructs and writes them into
 * the openCallQueue of the surveiller struct.
 * Currently uses a adaptive radix tree for filtering.
 *
 * @param surv_struct: surveiller struct pointer casted to void*
 * @retval 0 SUCCESS
 * @retval 1 MEMORY ALLOCATION ERROR
 * @retval 3 NO CONNECTION TO SOCKET
 */
void* surv_handleOpenCallSocket(void* surv_struct);

/**
 * @brief Thread function handling unprivaliged access to deamon
 *
 * threadfunction that listens on a socket for maintanance events
 *  - adding and removing of directories/files from the openCall_filter
 *  - shutting down of the deamon
 *  - checking if directory/file is being tracked
 *
 * @param surv_struct: surveiller struct pointer casted to (void*)
 *
 * @retval 0 NORMAL EXIT
 * @retval 1 MEMORY ALLOCATION ERROR
 * @retval 3 NO CONNECTION TO SOCKET
 */
void* surv_handleAccess(void* surv_struct);

void* surv_shutdown(void* surv_struct);


int surv_dispatch(surveiller* this);


#endif
