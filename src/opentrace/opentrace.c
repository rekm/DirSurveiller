#ifdef __linux__
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <linux/perf_event.h>

#include "opentrace.h"

int primeCommandToExecute(
        procController *controller, int argc, const char **argv,
        void (*exec_error)(int signo, siginfo_t *info, void *ucontext));

int main(int argc, char **argv)
{    
    int ret=0;
    int err=1;
    if (argc<2){
        printf("Wrong Input: Need at least one argument! \n");
        goto out;
    }
    procController pControl = { 
        .triggerPipe = -1,
        .pid = -1
    };

    fprintf(stderr ,"This still works \n");
     
    ret = primeCommandToExecute( &pControl, argc-1, argv+1, NULL);
    if (ret < 0){
        fprintf(stderr, "Priming of execute command failed\n");
        goto close_triggerPipe;
    }
    
    /* Build request body for perf_event_open 
     *
     * Note: On my machine open syscalls are trace
     *  0x1F1 -> 497 */

    /* Handle Threads in some way */

    /*goto close_triggerPipe;*/
    ret = runCommand( &pControl);
    
    /* Deal with perf events */
        
    return 0;

close_triggerPipe:
    close(pControl.triggerPipe);
out:
     return err; 
}

/*  Function that primes the user given command for execution and 
 *  stops just before executing it.
 *  
 *  It provides a way to signal failure of an execv command 
 *  by utilizing a nonparameter in the function definition.
 *  
 *  Pipes are used for parent child communication. 
 *
 *  ListEntry: 0 recieve and 1 send*/

int primeCommandToExecute(
        procController *controller, int argc, const char **argv,
        void (*exec_error)(int signo, siginfo_t *info, void *ucontext)){
    int subcommand_ready2execPipe[2], startSignalPipe[2];
    
    char *args [argc+1];
    memcpy(args, argv, argc *sizeof(char*));
    args[argc] = NULL;
    
    
    char pmsg; 
    /*Creating pipes*/    
    if (pipe(subcommand_ready2execPipe) < 0){
        fprintf(stderr, "Could not open pipe\n");
        return -1;
    }

    if (pipe(startSignalPipe) < 0) {
        fprintf(stderr, "Can not open start pipe\n");
        goto close_ready2exec;
    }
    controller->pid = fork();
    if (controller->pid < 0){
        fprintf(stderr,"Fork failed\n");
        goto close_pipes;
    }

    if (!controller->pid){
     /* making a copy of sysout*/
        int ret;
        dup2(2,1);
        ssignal(SIGTERM, SIG_DFL);
        close(subcommand_ready2execPipe[0]);
        close(startSignalPipe[1]);
        
        fcntl(startSignalPipe[0], F_SETFD, FD_CLOEXEC);       
        
        /* Closing the send ready pipeline signals readyness*/
        close(subcommand_ready2execPipe[1]);
        ret = read(startSignalPipe[0], &pmsg, 1);

        if (ret != 1) {
            if(ret == -1){
                fprintf(stderr, "Error reading pipe\n");
            }
            exit(ret);
        }
        
        execvp(args[0], (char**)args);
        if (exec_error) { 
            union sigval val;
            val.sival_int = errno;
            if (sigqueue(getppid(), SIGUSR1, val))
                fprintf(stderr, "%s\n", argv[0]);
        }else
            fprintf(stderr,"%s\n", argv[0]);
        exit(-1);

    }
    /* Prepare for error signal from command executer process */
    if (exec_error){
        struct sigaction act = {
            .sa_flags = SA_SIGINFO,
            .sa_sigaction = exec_error
        };
        sigaction(SIGUSR1, &act, NULL);
    }

    close(subcommand_ready2execPipe[1]);
    close(startSignalPipe[0]);
    /* Check if commad executer is ready */
    
    if (read(subcommand_ready2execPipe[0],&pmsg,1) == -1){
        fprintf(stderr,
                "Read of pipe failed: Can not determine executer state\n");
        goto close_pipes;
    }
    
    /*Setting up trigger in control*/
    fcntl(startSignalPipe[1], F_SETFD, FD_CLOEXEC);
    controller->triggerPipe = startSignalPipe[1];
    
    close(subcommand_ready2execPipe[0]);
    return 0;

close_pipes:
    close(startSignalPipe[0]);
    close(startSignalPipe[1]);

close_ready2exec:
    close(subcommand_ready2execPipe[0]);
    close(subcommand_ready2execPipe[1]);
    
    return -1;   
}

int runCommand(procController *controller){
    if (controller->triggerPipe > 0){
        char pmsg = 0;
        int ret;

        ret = write(controller->triggerPipe, &pmsg, 1);
        if (ret < 0)
            fprintf(stderr, "Write to pipe failed: Can't run commands\n");
        close(controller->triggerPipe);
        return ret;
    }
    return 0; 
}

struct thread_map *create_thread_map(pid_t pid){

    
}
