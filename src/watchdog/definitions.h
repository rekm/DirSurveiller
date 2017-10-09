#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H

#ifndef DEBUG
    #define DEBUG 0
#endif

#define NOMINAL 0
#define MEMORY_ALLOCATION_ERROR 1
#define ALREADY_EXISTS_IN_CONTAINER 2

#define zfree(ptr) ({ free(*ptr); *ptr = NULL; })
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#endif
