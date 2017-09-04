#ifndef _WATCHDOG_H
#define _WATCHDOG_H

#include <sys/types.h>



//################ Stringbuffer #############

typedef struct {
    char* string;
    size_t size;
    u_int32_t pos;
} stringBuffer;

/**
 * Initialize StringBuffer
 * @size is the inital size of the buffer
 * @returns 0, if successfull and
 *          1, if memory allocation fails
 */
int sb_init(stringBuffer* this, size_t size);

/**
 * Append string to buffer specifying length
 * @string String to append
 * @len Length to copy over from string
 */
int sb_appendl(stringBuffer* this, char* string, size_t len);

/**
 * Append string to buffer
 * @string
 */

int sb_append(stringBuffer* this, char* string);

/**
 * Delete first n (len) chars from Buffer and shift remaining section
 * to start of memory
 * @len chars to delete
 */
void sb_deletehead(stringBuffer* this, size_t len);

/**
 * Destructor of StringBuffer
 */
void sb_destroy(stringBuffer* this);

//########## Ringbuffer ###############
#define SIZE_RING 1024

typedef struct {
    u_int32_t read;
    u_int32_t write;
    void** buffer;
} g_ringBuffer;


int g_ringBuffer_init(g_ringBuffer* rb, size_t esize);
int g_ringBuffer_write(g_ringBuffer* rb, void *content);
int g_ringBuffer_read(g_ringBuffer* rb, void *content);
u_int32_t g_ringBuffer_size(g_ringBuffer* rb);
u_int32_t g_ringBuffer_mask(u_int32_t val);
int g_ringBuffer_empty(g_ringBuffer* rb);
int g_ringBuffer_full(g_ringBuffer* rb);
void g_ringBuffer_destroy(g_ringBuffer* rb);

// Ringbuffer chars
typedef struct {
    u_int32_t read;
    u_int32_t write;
    char buffer[SIZE_RING];
} char_ringBuffer;

void char_ringBuffer_init(char_ringBuffer* rb);
int char_ringBuffer_write(char_ringBuffer* rb, const char cont);
char char_ringBuffer_read(char_ringBuffer* rb);
u_int32_t char_ringBuffer_mask(u_int32_t val);
u_int32_t char_ringBuffer_size(char_ringBuffer* rb);
int char_ringBuffer_full(char_ringBuffer* rb);
int char_ringBuffer_empty(char_ringBuffer* rb);
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


int surv_init(surveiller*, const char* opencall_socketaddr,
               const char* execcall_socketaddr);

int surv_check_running(surveiller* watchdog);
int surv_handleExecCallSocket(void* surv_struct);
int surv_handleOpenCallSocket(void* surv_struct);
int surv_construct_dispatchtrees(surveiller* watchdog);
int surv_dispatch(surveiller* watchdog);
#endif
