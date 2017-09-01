#ifdef __linux__
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/syscall.h>

volatile sig_atomic_t child_pid = 0;

typedef struct{
    int triggerPipe;
    int pid;
    int gpid;
} procController;

int runCommand(procController *controller);
int primeCommandToExecute(
        procController *controller, int argc, char **argv,
        void (*exec_error)(int signo, siginfo_t *info, void *ucontext));

void term(int signum){
    if(child_pid)
        kill(child_pid, SIGTERM);
    exit(1);
}

int main(int argc, char **argv)
{
    struct sigaction action;
    memset(&action,0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);
    int ret=EXIT_SUCCESS;
    if (argc<2){
        printf("Wrong Input: Need at least one argument! \n");
        goto out;
    }
    procController pControl = {
        .triggerPipe = -1,
        .pid = -1,
        .gpid = -1,
    };


    ret = primeCommandToExecute( &pControl, argc-1, argv+1, NULL);
    if (ret < 0){
        fprintf(stderr, "Priming of execute command failed\n");
        goto close_triggerPipe;
    }
    fprintf(stderr, "CommandPID: %i\n", pControl.pid );
    child_pid = pControl.pid;
    ret = runCommand( &pControl);
    sleep(2);
    /*pControl.gpid = getpgid(pControl.gpid);
    if ( pControl.gpid == -1 ){
        fprintf(stderr, "bash script has already exited!");
    exit(EXIT_FAILURE);
    } */
    /* Deal with perf events */
    wait(&ret);
    return 0;

close_triggerPipe:
    close(pControl.triggerPipe);
out:
     return ret;
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
        procController *controller, int argc, char **argv,
        void (*exec_error)(int signo, siginfo_t *info, void *ucontext)){
    int subcommand_ready2execPipe[2], startSignalPipe[2];

    char *args [argc+1];
    memcpy(args, argv, argc*sizeof(char*));
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
        int open_connection, exec_connection;
        char *opencall_socket_path = "/tmp/opentrace_opencalls";
        char *execcall_socket_path = "/tmp/opentrace_execcalls";
        struct sockaddr_un open_addr;
        struct sockaddr_un exec_addr;
        struct timeval timeout;
        int fd_open, fd_exec;
        fd_set fds;
        if (( fd_open = socket(AF_UNIX, SOCK_STREAM,0)) == -1) {
            perror("Can't create socket");
            exit(-1);
        }
        if (( fd_exec = socket(AF_UNIX, SOCK_STREAM,0)) == -1) {
            perror("Can't create socket");
            exit(-1);
        }
        memset(&open_addr, 0, sizeof(open_addr));
        memset(&exec_addr, 0, sizeof(exec_addr));
        open_addr.sun_family = AF_UNIX;
        exec_addr.sun_family = AF_UNIX;
        // Creating adresses
        strncpy(open_addr.sun_path, opencall_socket_path,
                sizeof(open_addr.sun_path)-1);
        unlink(opencall_socket_path);

        strncpy(exec_addr.sun_path, execcall_socket_path,
                sizeof(exec_addr.sun_path)-1);
        unlink(execcall_socket_path);
        //binding of sockets

        int reuse = 1;
        if(setsockopt(fd_open, SOL_SOCKET, SO_REUSEADDR,
                       &reuse, sizeof(reuse))){
            fprintf(stderr, "Reuse failed with code %i\n",errno);
            exit(-1);
        }

        mode_t current_mask = umask((mode_t) 0111);
        if (bind(fd_open, (struct sockaddr*)&open_addr,
                 sizeof(open_addr)) == -1) {
            perror( "bind of open_address failed");
            exit(-1);
        }
        if (bind(fd_exec, (struct sockaddr*)&exec_addr,
                 sizeof(exec_addr)) == -1) {
            perror( "bind of exec_address failed");
            exit(-1);
        }
        umask(current_mask);
        //setting sockets to be listened to
        if (listen(fd_open, 5) == -1) {
           perror( "Can't set listen mode in open socket");
           exit(-1);
        }
        if (listen(fd_exec, 5) == -1) {
           perror( "Can't set listen mode in exec socket");
           exit(-1);
        }
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
        //Set timeout
        timeout.tv_sec = 5;
        timeout.tv_usec = 500000;
        //clear fd_set
        FD_ZERO(&fds);
        //Add sockets to set
        FD_SET(fd_open, &fds);
        FD_SET(fd_exec, &fds);
//        if(( open_connection = accept( fd_open,NULL,NULL)) == -1) exit(-1);
//        if(( exec_connection = accept( fd_exec,NULL,NULL)) == -1) exit(-1);
        ret = select(fd_exec+1, &fds, NULL, NULL, &timeout);
        dup2( open_connection, STDOUT_FILENO);
        dup2( exec_connection, STDERR_FILENO);
        close(open_connection);
        close(exec_connection);
        execvp(args[0], (char**)args);
        close(fd_exec);
        close(fd_open);
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

