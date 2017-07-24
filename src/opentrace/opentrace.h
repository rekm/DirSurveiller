#ifndef _OPENTRACE_H
#define _OPENTRACE_H

#include <sys/types.h> 


typedef struct{ 
    int triggerPipe;
    int pid;
} procController;


int runCommand(procController *controller);

/*Stole threadmap object from Linus Torvalds perf*/
struct thread_map_data {
        pid_t    pid;
        char    *comm;
};

struct thread_map {
        refcount_t refcnt;
        int nr;
        struct thread_map_data map[];
};


struct thread_map *create_thread_map(pid_t pid);

# endif
