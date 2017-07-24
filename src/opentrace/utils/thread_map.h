#ifndef _THREAD_MAP_H
#define _THREAD_MAP_H

#include <sys/types.h>
#include <stdio.h>
#include <linux/refcount.h>


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
