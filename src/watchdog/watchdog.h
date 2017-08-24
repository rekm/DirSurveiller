#ifndef _WATCHDOG_H
#define _WATCHDOG_H

#include <sys/types.h>

// Ringbuffer
#define SIZE_RING 1024 

typedef struct {
    u_int32_t read;
    u_int32_t write;
    void* buffer; 
} ringBuffer;


int ringBuffer_init(ringBuffer* rb, size_t size);
int ringBuffer_write(ringBuffer* rb, void *content, size_t len); 
int ringBuffer_read(ringBuffer* rb, void *content, size_t len);
int ringBuffer_full(ringBuffer* rb);
void ringBuffer_destroy(ringBuffer* rb);

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
 *  ->pid
 *  ->start_time
 *  ->char* pname Process Name
 *  ->openCall *openCall
 *
 */

typedef struct {
    pid_t pid;
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

} watchdog_processor;


#endif
