#ifndef _WATCHDOG_H
#define _WATCHDOG_H

#include <sys/types.h>

// Ringbuffer
#define SIZE_RING 1024

typedef struct {
    u_int32_t read;
    u_int32_t write;
    void* buffer;
} g_ringBuffer;


int g_ringBuffer_init(g_ringBuffer* rb, size_t esize);
int g_ringBuffer_write(g_ringBuffer* rb, void *content, size_t size);
int g_ringBuffer_read(g_ringBuffer* rb, void *content, size_t size);
int g_ringBuffer_full(g_ringBuffer* rb);
void g_ringBuffer_destroy(g_ringBuffer* rb);

// Ringbuffer chars
typedef struct {
    u_int32_t read;
    u_int32_t write;
    char buffer[SIZE_RING];
} char_ringBuffer;

int char_ringBuffer_init(char_ringBuffer* rb);
int char_ringBuffer_write(char_ringBuffer* rb, char *content, size_t len);
int char_ringBuffer_read(char_ringBuffer* rb, char *content, size_t len);
int char_ringBuffer_full(char_ringBuffer* rb);
void char_ringBuffer_destroy(char_ringBuffer* rb);

/**
 * OpenCall
 *  ->timestamp
 *  ->flag
 *  ->char* filepath
 *  ->inode
 */
typedef struct {
    double time_stamp;
    int flag;
    char* filepath;
    long inode;
} openCall;

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
    double start_time;
    char *pname;
    int call_num;
    openCall *calls;
} process;


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
    process* procs;
    g_ringBuffer* commandQueue;
    g_ringBuffer* openQueue;
} surveiller;


int wproc_init(surveiller*, const char* opencall_socketaddr,
               const char* execcall_socketaddr);

int wproc_check_running(surveiller* watchdog);
int wproc_handleExecCallSocket(surveiller* watchdog);
int wproc_handleOpenCallSocket(surveiller* watchdog);
int wproc_construct_dispatchtrees(surveiller* watchdog);
int wproc_dispatch(surveiller* watchdog);
#endif
