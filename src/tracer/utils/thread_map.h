#ifndef _THREAD_MAP_H
#define _THREAD_MAP_H

#include <sys/types.h>
#include <stdio.h>
#include <linux/refcount.h>


/*This will propably end up in a util header */

#define zfree(ptr) ({ free(*ptr); *ptr = NULL; })

/*The threadmap logic largely taken from Linus Torvalds Perf git project*/

struct thread_map_data {
    pid_t pid;
    char *cmd;
};

struct thread_map {
    refcount_t refcnt;
    int nr;
    struct thread_map_data map[];
};

struct thread_map *new_thread_map(pid_t pid);

struct thread_map *thread_map_realloc(struct thread_map *map, int nr);
/* Inlined Functions*/

static inline int thread_map_nr(struct thread_map *map){
   return map ? map->nr : 1;
} 

static inline pid_t thread_map_pid(struct thread_map *map, int thread){
    return map->map[thread].pid;
}

static inline void thread_map_set_pid(struct thread_map *map, int thread, pid_t pid){
    map->map[thread].pid =pid;
}

static inline char *thread_map_comm(struct thread_map *map, int thread){
    return map->map[thread].cmd;
}

#endif
