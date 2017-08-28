#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "watchdog.h"





// Char ringbuffer implementation

void char_ringBuffer_init(char_ringBuffer* rb){
    rb->read = 0;
    rb->write = 0;
    memset(rb->buffer,0,SIZE_RING);
}

int char_ringBuffer_write(char_ringBuffer* rb,const char cont){
    int ret = char_ringBuffer_full(rb);
    if(!ret)
        rb->buffer[char_ringBuffer_mask(rb->write++)]=cont;
    return ret;
}

char char_ringBuffer_read(char_ringBuffer* rb){
    int ret = char_ringBuffer_empty(rb);
    if(!ret)
      return rb->buffer[char_ringBuffer_mask(rb->read++)];  
    return 0;
}

u_int32_t char_ringBuffer_mask(u_int32_t val){
   return val & (SIZE_RING - 1); 
}

int char_ringBuffer_empty(char_ringBuffer* rb){
    return rb->read==rb->write;
}

int char_ringBuffer_full(char_ringBuffer* rb){
    return char_ringBuffer_size(rb) == SIZE_RING;
}

u_int32_t char_ringBuffer_size(char_ringBuffer* rb){
    return rb->write - rb->read;
}


// general ringbuffer implementation
int g_ringBuffer_init(g_ringBuffer* rb, size_t esize){
    rb->buffer = calloc(rb->buffer, esize*SIZE_RING);
    if(!rb->buffer)
        return 1;
    rb->read = 0;
    rb->write = 0;
    return 0;
}

int g_ringBuffer_write(g_ringBuffer* rb 


/**
 * Handling of incomming opencalls and filtering of them
 */


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
