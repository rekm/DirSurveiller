#include <dirent.h>
#include "thread_map.h"

static int filter(const struct dirent *dir){
    if(dir->d_name[0] == '.')
        return 0;
    else
        return 1;
}

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
    threads = thread_map__alloc(items);

