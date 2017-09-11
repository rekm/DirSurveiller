#ifndef _CONTAINERS_H
#define _CONTAINERS_H

#include <sys/types.h>

#define zfree(ptr) ({ free(*ptr); *ptr = NULL; })
#define NOMINAL 0
//################ Stringbuffer #############

typedef struct {
    char* string;
    size_t size;
    u_int32_t end_pos;
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
 * reset string in StringBuffer to empty string
 */
void sb_flush(stringBuffer* this);

/**
 * Destructor of StringBuffer
 */
void sb_destroy(stringBuffer* this);

/**
 * Get copy of StringBuffer string
 * @retString: uninitialized passthrough string
 *             that gets filled with content
 * @returns: (0/NOMINAL) if allocation and assignment happend
 *           (1) if allocation failed
 */
int sb_stringCpy(char** retString, stringBuffer* this);

//########## Ringbuffer ###############
#define SIZE_RING 1024

typedef struct {
    u_int32_t read;
    u_int32_t write;
    void** buffer;
} g_ringBuffer;

int g_ringBuffer_init(g_ringBuffer* rb, size_t esize);
int g_ringBuffer_write(g_ringBuffer* rb, void *content);
int g_ringBuffer_read(g_ringBuffer* rb, void **content);
u_int32_t g_ringBuffer_size(g_ringBuffer* rb);
u_int32_t g_ringBuffer_mask(u_int32_t val);
int g_ringBuffer_empty(g_ringBuffer* rb);
int g_ringBuffer_full(g_ringBuffer* rb);


/**
 * A destructor for a g_ringbuffer that clears allocated pointers\n
 * and calls a given destructor function, to make sure that the stored\n
 * data is also taken care of.\n
 *
 * @p this: A poiter to the general ringbuffer that needs destroying.
 * @p destruct_fun: A function that will be called on each elemet in the
 *                  buffer.
 */
void g_ringBuffer_destroyd(g_ringBuffer* this,
                           void (*destruct_fun)(void*));


/**
 * A destructor function that clears the allocated pointers\n
 * in a general ringbuffer struct.
 *
 * @p this: reference to g:ringbuffer to be destroyed
 * @returns: void;
 */
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




#endif
