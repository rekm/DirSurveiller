/**
 * Process that takes two pipes  as input and manages a forest of trees 
 * that partially represent the currently runnig processes 
 *
 * It tries to track sys_open events delivered by one of the pipes and 
 * tries to integrate it with treestructurs representing the currently running 
 * processes and their parent-child relationships. 
 *
 * The first idea was to simply use an adjacency list...
 * An Adaptive Radix Tree would be more fun and might find some use in future
 * projects. This will result in unnecessary over engineering, but that is half
 * the fun.*/

#include<sys/types.h> 

/* One pice of defensive programming. Eliminating the possibility of 
 * reuse and ensuing memory weirdness */
#define zfree(ptr) ({ free(*ptr); *ptr = NULL; })


struct process {
    pid_t pid;
    pid_t ppid;
    pid_t index;
    char *cmdName;
    char **cmdArgs;
};


struct proc_map {
    u_int64_t count; 
    struct process *map;    
};



int main(int argc, char **argv){
            

}    
