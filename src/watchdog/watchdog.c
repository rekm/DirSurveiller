#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>




/**
 * A function handling incomming execute commads
 * i.e reading constructing process objects and handling
 * double PIDs.
 */


/**
 * Handling of incomming opencalls and filtering of them
 */


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





int main(void){
    int max_pid;

    max_pid = get_max_PID();
    if (max_pid == -1) {
        fprintf(stderr, "Could not read /proc/sys/kernel/pid_max\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Max PID: %i\n",max_pid);
    exit(EXIT_SUCCESS);
}
