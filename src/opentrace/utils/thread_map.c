#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "thread_map.h"

static int filter(const struct dirent *dir){
    if(dir->d_name[0] == '.')
        return 0;
    else
        return 1;
}

static void thread_map_reset(struct thread_map *map, int start, int nr){
    size_t size = (nr - start) * sizeof(map->map[0]);
    memset(&map->map[start],0,size);
}

struct thread_map *thread_map_realloc(struct thread_map *map, int nr){
    size_t size = sizeof(*map) + sizeof(map->map[0]) * nr;
    int start = map ? map->nr : 0;

    map = realloc(map, size);
    if (map)
        thread_map_reset(map, start, nr);
    return map;
}


#define thread_map_alloc(__nr) thread_map_realloc(NULL, __nr)

struct thread_map *new_thread_map(pid_t pid){
    struct thread_map *threads;
    char name[256];
    int items;
    struct dirent **namelist = NULL;
    int i;
    sprintf(name,"/proc/%d/task",pid);
    items = scandir(name, &namelist, filter, NULL);
    if (items <= 0)
        return NULL;
    threads = thread_map_alloc(items);
    if (threads != NULL) {
        for (i = 0; i < items; i++)
            thread_map_set_pid(threads, i, atoi(namelist[i]->d_name));
        threads->nr = items;
        refcount_set(&threads->refcnt, i);
    }

    for (i=0; i<items; i++)
       zfree(&namelist[i]);
    free(namelist);
    return threads;
}   

