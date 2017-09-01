#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "watchdog.h"


#define zfree(ptr) ({ free(*ptr); *ptr = NULL; })


// StringBuffer implementation


int sb_init(stringBuffer* this, size_t size){
    this->string = malloc(sizeof(char)*size);
    if (!this->string | !size)
        return 1;
    this->size = size;
    this->pos = 0;
    this->string[0] = '\0';
    return 0;
}

int sb_appendl(stringBuffer* this, char* string, size_t len){
    while (this->pos+len >= this->size){
        char *ret = realloc(this->string,sizeof(char)*this->size*2);
        if(!ret) return 1;
        this->size = this->size*2;
    }
    memcpy(this->string+this->pos, string, len);
    this->pos+=len;
    return 0;
}

int sb_append(stringBuffer* this, char* string){
    size_t len = strlen(string);
    while (this->pos+len >= this->size){
        char *ret = realloc(this->string,sizeof(char)*this->size*2);
        if(!ret) return 1;
        this->size = this->size*2;
    }
    memcpy(this->string+this->pos, string, len+1);
    this->pos+=len;
    return 0;
}

void sb_deletehead(stringBuffer* this, size_t len){
    if (len > this->pos) len = this->pos;
    memmove(this->string, this->string+len, this->pos+1 - len);
    this->pos -= len;
}

void sb_destroy(stringBuffer* this){
    this->size = 0;
    this->pos = 0;
    zfree(&this->string);
}
 
// Char ringbuffer implementation

void char_ringBuffer_init(char_ringBuffer* rb){
    rb->read = 0;
    rb->write = 0;
    memset(&rb->buffer,0,SIZE_RING);
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
    rb->buffer = calloc(1, esize*SIZE_RING);
    if(!rb->buffer)
        return 1;
    rb->read = 0;
    rb->write = 0;
    return 0;
}

int g_ringBuffer_write(g_ringBuffer* rb, void* content){
    int ret = g_ringBuffer_full(rb);
    if(!ret)
        rb->buffer[g_ringBuffer_mask(rb->write++)] = &content;
    return ret;
}

int g_ringBuffer_read(g_ringBuffer* rb, void* content){
    int ret = g_ringBuffer_empty(rb);
    if(!ret)
       content = rb->buffer[g_ringBuffer_mask(rb->read++)];
    return ret;
}

u_int32_t g_ringBuffer_mask(u_int32_t val){
    return val & (SIZE_RING -1);
}

int g_ringBuffer_empty(g_ringBuffer* rb){
    return rb->read = rb->write;
}

int g_ringBuffer_full(g_ringBuffer* rb){
    return g_ringBuffer_size(rb)==SIZE_RING;
}

u_int32_t g_ringBuffer_size(g_ringBuffer* rb){
    return rb->write - rb->read;
}

void g_ringBuffer_destroy(g_ringBuffer* rb){
    int startSize = g_ringBuffer_size(rb);
    for (int i = 0; i < startSize; i++){
        void* value = NULL;
        if(g_ringBuffer_read(rb, value))
            break;
        zfree(&value);
    }
    zfree(&rb->buffer);
}
/**
 * Handling of incomming opencalls and filtering of them
 */
int surv_handleOpenCallSocket(void* surv_struct){
    int ret = 0;
    char buf[256];
    stringBuffer* fields;
    int bufpos = 0;
    int rc;
    ssize_t rlen;
    struct sockaddr_un addr;

    surveiller* surv = (surveiller*) surv_struct;
    if ( (rc = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
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
     if (connect( rc, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
         ret = 3;
         goto endfun;
     }
     /* Fields:
      *     time 34 chars
      *     command name 7
      *     PID  5
      *     Return Val 
      *     filename unbounded 100
      */
     int field_num = 5;
     fields = malloc(sizeof(stringBuffer*)*field_num);

     
     
     while( (rlen = recv(rc, buf+bufpos,
                         sizeof(buf)-sizeof(char)*bufpos, 0)) != -1){
        printf( "%s\n", buf);
        for (int i=0; i<=bufpos+rlen; i++){
            
        }
     }
     for (int ii=0; ii<field_num;ii++)
        sb_destroy(fields+ii);   

endfun:
    return ret;
}


/**
 * Handler for incomming execcalls
 */

int surv_handleExecCallSocket(void* surv_struct){
    int ret = 0;
    surveiller* surv = (surveiller*) surv_struct;
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



int test(){
    int ret = 0;
    printf("Testing data structures defined in watchdog.h\n");
    printf("Stringbuffer:\n\n init\n");
    stringBuffer sb;
    ret = sb_init(&sb, 16);
    if(ret){
        perror("Could not allocate memory\n");
        goto freeStringBuffer;
    }
    printf( "size:%zu pos:%i \nstring:%s\n\n", sb.size, sb.pos, sb.string);
    printf( "Appending to Buffer\n");
    ret = sb_append(&sb,"hello world ");
    printf( "size:%zu pos:%i \nstring:%s\n\n", sb.size, sb.pos, sb.string);
    ret = sb_append(&sb,"hello world ");
    ret = sb_append(&sb,"hello world ");
    printf( "size:%zu pos:%i \nstring:%s\n\n", sb.size, sb.pos, sb.string);
    printf( "delete head\n");
    sb_deletehead(&sb,5);
    printf( "size:%zu pos:%i \nstring:%s\n\n", sb.size, sb.pos, sb.string);
    sb_append(&sb, "hello");
    printf( "size:%zu pos:%i \nstring:%s\n\n", sb.size, sb.pos, sb.string);
    sb_deletehead(&sb, 40);
    printf( "size:%zu pos:%i \nstring:%s\n\n", sb.size, sb.pos, sb.string);
    
freeStringBuffer:       
    sb_destroy(&sb);
    printf( "size:%zu pos:%i \nstring:%s\n\n", sb.size, sb.pos, sb.string); 
    
    int field_num = 5;
    stringBuffer fields[field_num];
    // fields = (stringBuffer*)malloc(sizeof(stringBuffer*)*field_num);
    sb_init(fields,64); 
    sb_append(fields,"hello world "); 
    sb_init(&fields[1],16); 
    sb_init(&fields[2],16);
    sb_init(&fields[3],16);
    sb_init(&fields[4],128);
     
    for (int ii=0; ii<field_num;ii++)
        printf( "size:%zu pos:%i \nstring:%s\n\n",
                fields[ii].size, fields[ii].pos, fields[ii].string);
    
    for (int iii=0; iii<field_num;iii++){
        printf( "destroying %i\n",iii); 
        sb_destroy(fields+iii);   
    }
    for (int ii=0; ii<field_num;ii++)
        printf( "size:%zu pos:%i \nstring:%s\n\n",
                fields[ii].size, fields[ii].pos, fields[ii].string);

    exit(0);
} 


int main(void){
    int max_pid;

    max_pid = get_max_PID();
    if (max_pid == -1) {
        fprintf(stderr, "Could not read /proc/sys/kernel/pid_max\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Max PID: %i\n",max_pid);
    if (test()) 
        exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}
