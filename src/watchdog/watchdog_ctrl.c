#define _GNU_SOURCE

#include <argp.h>
#include <error.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
// Argparse code mostly copied from the argparse manual

const char *argp_program_version =
  	"watchdog_ctrl 0.0.1 ";
const char *argp_program_bug_address =
  	"<r.w.kmiecinski@gmail.com>";
static char doc[] =
  	"watchdog_ctrl -- control process for the watchdog daemon\n \
      	options\n\
      	This program is supposed to represent the api for controlling\n \
      	the watchdog daemon.\n\
        It is used to check its status, start, stop, or\n\
      	add and remove files/directories from the watchlist";

static char args_doc[] = "...";

#define OPT_ABORT  1            /* –abort */

static struct argp_option options[] = {
  	{"start",  's', 0,       0, "Start watchdog" },
  	{"stop",   'q', 0,       0, "Stop watchdog daemon" },
 	{"status", 'u', 0,       0, "Get status update from daemon"},
  	{0,0,0,0, "Manipulating Watchlist:" },
  	{"add",    'a', "FILE/DIR",  OPTION_ARG_OPTIONAL,
        "Add FILE or DIRECTORY to watchlist" },
  	{"remove", 'r', "FILE/DIR",  OPTION_ARG_OPTIONAL,
        "Remove FILE or DIRECTORY to watchlist" },
  	{"abort",    OPT_ABORT, 0, 0, "Abort before showing any output"},
	{ 0 }
};

struct arguments
{
  	int start, stop, status, abort;
  	char *add_file;
	char *remove_file;    /* file arg to ‘--output’ */
};

static error_t parse_opt (int key, char *arg, struct argp_state *state){
	/* Get the input argument from argp_parse, which we
     * know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

	switch (key){
		case 's':
		  	arguments->start = 1;
		  	break;
		case 'q':
		  	arguments->stop = 1;
		  	break;
		case 'u':
		  	arguments->status = 1;
		  	break;
		case 'a':
		  	realpath(arg ? arg : "./", arguments->add_file);
		  	break;
		case 'r':
		  	realpath(arg ? arg : "./", arguments->remove_file);
		  	break;
		case OPT_ABORT:
		  arguments->abort = 1;
		  break;

		case ARGP_KEY_NO_ARGS:
		  state->next = state->argc;
		  //argp_usage (state);

		case ARGP_KEY_ARG:
		  /* Here we know that state->arg_num == 0, since we
			 force argument parsing to end before any more arguments can
			 get here. */
//		  arguments->arg1 = arg;

		  /* Now we consume all the rest of the arguments.
			 state->next is the index in state->argv of the
			 next argument to be parsed, which is the first string
			 we’re interested in, so we can just use
			 &state->argv[state->next] as the value for
			 arguments->strings.

			 In addition, by setting state->next to the end
			 of the arguments, we can force argp to stop parsing here and
			 return. */
//		  arguments->strings = &state->argv[state->next];
		  state->next = state->argc;

		  break;

		default:
		  return ARGP_ERR_UNKNOWN;
		}
	  return 0;
	}

static struct argp argp = { options, parse_opt, args_doc, doc };

int main (int argc, char** argv){
	struct arguments args;
	char ctrl_socket_addr[] = "/tmp/opentrace_ctl.socket";
    int ret = 0;
    args.add_file = calloc(PATH_MAX+1,sizeof(char));
	if(!args.add_file)
    {
        perror("file name");
        exit(EXIT_FAILURE);
    }
    args.remove_file = calloc(PATH_MAX+1, sizeof(char));
    if(!args.remove_file)
    {
        free(args.add_file);
        perror("file name");
        exit(EXIT_FAILURE);
    }
    args.start = 0;
    args.stop = 0;
    args.status = 0;
    args.abort = 0;

    argp_parse (&argp, argc, argv, 0, 0, &args);

    if(args.abort)
        error (10, 0, "ABORTED");
    if(1 < (args.start + args.stop))
        error (11, 0, "START and STOP exclude eachother");
    if(!(args.stop || args.start || args.status ||
         args.add_file[0] || args.remove_file[0]))
    {
        fprintf(stderr, "No options provided! Will check daemon status\n\n");
        args.status = 1;
    }

    //socket setup
    struct sockaddr_un addr;
    char buf[200];
    char request[300];
    int so_fd;
    if( (so_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("cannot create socket");
        goto endprog;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if( *ctrl_socket_addr == '\0'){
        *addr.sun_path = '\0';
        strncpy( addr.sun_path + 1,
                ctrl_socket_addr + 1, sizeof(addr.sun_path) - 2);
    } else {
        strncpy(addr.sun_path,
                ctrl_socket_addr, sizeof(addr.sun_path) -1);
    }
    if (connect( so_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        if(args.start)
        {
            printf("\nStarting deamon!\n");
            //TODO Start deamon
            ret = 0;
        }
        else if (args.stop)
        {
            printf("\nAlready stopped\n");
            ret = 0;
        }
        perror("Could not establish connection to daemon\n"
               "Deamon is most likely not running or talking over\n"
               "the specified socket\n ERROR");
        ret = 3;
        goto endprog;
    }
    if(args.start)
    {
        printf("\nDeamon is already running!\n");
        ret = 3;
        goto disconnect_socket;
    }
    if(args.status)
    {
        char* status_request = "get_status$";
        if (write(so_fd, status_request, strlen(status_request)+1)<0)
        {
            perror("Error communicating with deamon");
            goto disconnect_socket;
        }
        else
        {
            read(so_fd, buf, sizeof(buf));
            printf("Watchdog Daemon replied: Woof\n%s\n", buf);
        }
    }
    if(args.stop)
    {
        sprintf(request, "shutdown$");
        if (write(so_fd, request, strlen(request)+1)<0)
        {
            perror("Coudn't reach watchdog");
            goto disconnect_socket;
        }
        else
        {
            read(so_fd, buf, sizeof(buf));
            printf("Watchdog Daemon replied: Woof\n%s\n", buf);
            goto disconnect_socket;
        }
    }
    if(args.add_file[0])
    {
        sprintf(request, "add_filter$%s",args.add_file);
        if (write(so_fd, request, strlen(request)+1)<0)
        {
            perror("Coudn't reach watchdog");
            goto disconnect_socket;
        }
        else
        {
            read(so_fd, buf, sizeof(buf));
            printf("Watchdog Daemon replied: Woof\n%s\n", buf);
            goto disconnect_socket;
        }
    }
    if(args.add_file[0])
    {
        sprintf(request, "remove_filter$%s",args.remove_file);
        if (write(so_fd, request, strlen(request)+1)<0)
        {
            perror("Coudn't reach watchdog");
            goto disconnect_socket;
        }
        else
        {
            read(so_fd, buf, sizeof(buf));
            printf("Watchdog Daemon replied: Woof\n%s\n", buf);
            goto disconnect_socket;
        }
    }

disconnect_socket:
    close(so_fd);
endprog:
    free(args.add_file);
    free(args.remove_file);
    exit(ret ? EXIT_FAILURE : EXIT_SUCCESS);
}
