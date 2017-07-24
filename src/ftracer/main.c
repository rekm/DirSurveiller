#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

/*Small program that traces system calls using ptrace */


/* Full disclosure: This iteration is mostly stolen* from 
 *  
 *  Write yourself an strace in 70 lines of code
 *  by Nelson Elhage 2010
 *
 *  His github project "ministrace" lacks licensing information
 *  
 *  The Blog for which it was written is licensed as
 *  Attribution 4.0 International (CC BY 4.0)  
 *
 *  *Stolen means adapted. Slight changes were made so only sys_open
 *  calls are outputted.
 *
 * */



#define offsetof(a,b)__builtin_offsetof(a,b)

int run_workpayload(int argc, char **argv);
int run_traceOnPayload(pid_t worker);
int wait_for_syscall(pid_t worker);
char *get_filename(pid_t worker, unsigned long addr);

int main(int argc, char **argv){
    if (argc<2){
        fprintf(stderr,"Need atleast one argument!");
        return -1;
    }
    pid_t worker = fork();
    if(worker==0){
        return run_workpayload(argc-1,argv+1);
    } else {
        return run_traceOnPayload(worker);
    }
}


int run_workpayload(int argc,char **argv){
    char *args [argc+1];
    memcpy(args, argv, argc *sizeof(char*));
    args[argc] = NULL;
    ptrace(PTRACE_TRACEME);
    /* This might be unnecessary because one can set a trap for execv
     * systemcalls */
    kill(getpid(),SIGSTOP);
    return execvp(args[0], args);
}

int run_traceOnPayload(pid_t worker){
    char *sval;
    int status, syscall;
    int retval;
    int openFlag;
    unsigned long fileNameRegOffset;
    waitpid(worker, &status, 0);
    ptrace(PTRACE_SETOPTIONS, worker, 0, PTRACE_O_TRACESYSGOOD);  
    while(1) {
        if (wait_for_syscall(worker) != 0) break;
        syscall = ptrace(PTRACE_PEEKUSER, worker, sizeof(long)*ORIG_RAX);
        if (syscall==2){
            /*fprintf(stderr,"syscall(%d) = ", syscall);*/
            fileNameRegOffset = ptrace(PTRACE_PEEKUSER, worker, 
                    offsetof(struct user, regs.rdi)); 
            fprintf(stderr, "PID: %i, ",worker);  
            sval = get_filename(worker,fileNameRegOffset);
            fprintf(stderr, "\"%s\"", sval);
            free(sval);
            openFlag = ptrace(PTRACE_PEEKUSER, worker,
                    offsetof(struct user, regs.rsi));
            fprintf(stderr, ", %i",openFlag); 
            if (wait_for_syscall(worker) != 0) break;
            retval = ptrace(PTRACE_PEEKUSER,worker, sizeof(long)*RAX);
            fprintf(stderr, ", %d\n", retval);

        }
    }
    return 0;
}

int wait_for_syscall(pid_t worker){
    int status;
    while (1) {
        ptrace(PTRACE_SYSCALL, worker, 0, 0);
        waitpid(worker, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
            return 0;
        if (WIFEXITED(status))
            return 1;
    }
}   

char *get_filename(pid_t worker, unsigned long addr) {
    char *val = malloc(0x1000);
    int allocated = 0x1000, total_read = 0;
    unsigned long tmp;
    while (1) {
        if (total_read + sizeof tmp > allocated){
            allocated *= 2;
            val = realloc(val, allocated);
        }
        tmp = ptrace(PTRACE_PEEKDATA, worker, addr + total_read);
        if(errno != 0) {
            val[total_read] = 0;
            break;
        }
        memcpy(val + total_read, &tmp, sizeof tmp);
        if (memchr(&tmp, 0, sizeof tmp) != NULL)
            break;
        total_read += sizeof tmp;
    }
    return val;
}
