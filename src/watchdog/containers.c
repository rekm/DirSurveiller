#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "containers.h"
// StringBuffer implementation


int sb_init(stringBuffer* this, size_t size){
    this->string = malloc(sizeof(char)*size);
    if (!this->string | !size)
        return 1;
    this->size = size;
    this->end_pos = 0;
    this->string[0] = '\0';
    return 0;
}

int sb_appendl(stringBuffer* this, char* string, size_t len){
    while (this->end_pos+len >= this->size){
        char *ret = realloc(this->string,this->size*2);
        if(!ret) return 1;
        this->string = ret;
        this->size = this->size*2;
    }
    memcpy(this->string+this->end_pos, string, len);
    this->end_pos+=len;
    return 0;
}

int sb_append(stringBuffer* this, char* string){
    size_t len = strlen(string);
    while (this->end_pos+len >= this->size){
        char *ret = realloc(this->string, this->size*2);
        if(!ret) return 1;
        this->size = this->size*2;
        this->string = ret;
    }
    memcpy(this->string+this->end_pos, string, len+1);
    this->end_pos+=len;
    return 0;
}

void sb_deletehead(stringBuffer* this, size_t len){
    if (len > this->end_pos) len = this->end_pos;
    memmove(this->string, this->string+len, this->end_pos+1 - len);
    this->end_pos -= len;
    this->string[this->end_pos] = '\0';
}

void sb_flush(stringBuffer* this){
    memset(this->string, 0, this->size);
    this->end_pos = 0;
}


void sb_destroy(stringBuffer* this){
    this->size = 0;
    this->end_pos = 0;
    zfree(&this->string);
}

int sb_stringCpy(char** retString, stringBuffer* this){
    int ret = NOMINAL;
    *retString = malloc(this->end_pos+1);
    if(!retString)
        ret=1;
    else
        strcpy(*retString, this->string);
        //printf("%s\n",*retString);
    return ret;
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
        rb->buffer[g_ringBuffer_mask(rb->write++)] = content;
    return ret;
}

int g_ringBuffer_read(g_ringBuffer* rb, void** content){
    int ret = g_ringBuffer_empty(rb);
    if(!ret)
       *content = rb->buffer[g_ringBuffer_mask(rb->read++)];
    return ret;
}

u_int32_t g_ringBuffer_mask(u_int32_t val){
    return val & (SIZE_RING -1);
}

int g_ringBuffer_empty(g_ringBuffer* rb){
    return rb->read == rb->write;
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

void g_ringBuffer_destroyd(g_ringBuffer* this,
                           void (*destruct_fun)(void*)){
    int startSize = g_ringBuffer_size(this);
    for (int i = 0; i < startSize; i++){
        void* value = NULL;
        if(g_ringBuffer_read(this, &value))
            break;
        destruct_fun(value);
        //zfree(&value);
    }
    zfree(&this->buffer);
}

