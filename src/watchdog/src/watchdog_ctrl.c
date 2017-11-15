#define _GNU_SOURCE
#define _XOPEN_SOURCE 500
#include <argp.h>
#include <error.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <DirSurveillerConfig.h>
#include "database.h"

// Argparse code mostly copied from the argparse manual

const char* argp_program_version = "watchdog_ctrl " DirSurveiller_FULL_VERSION;
const char *argp_program_bug_address =
  	"<r.w.kmiecinski@gmail.com>";
static char doc[] =
  	"watchdog_ctrl -- control process for the watchdog daemon\n \
      	options\n\
      	This program is supposed to represent the api for controlling\n \
      	the watchdog daemon.\n\
        It is used to check its status, start, stop, or\n\
      	add and remove files/directories from the watchlist.\n\
        Also it presents the option to query the call database";

static char args_doc[] = "[ARG1]";

#define OPT_ABORT  1            /* –abort */

static struct argp_option options[] = {
  	{"start",  's', 0,       0, "Start watchdog" },
  	{"stop",   'q', 0,       0, "Stop watchdog daemon" },
    {"restart",'r', 0,       0, "Restart watchdog daemon"},
 	{"status", 'u', 0,       0, "Get status update from daemon"},
  	{0,0,0,0, "Manipulating Watchlist:" },
  	{"add",    'a', "FILE/DIR",  OPTION_ARG_OPTIONAL,
        "Add FILE or DIRECTORY to watchlist" },
  	{"delete", 'd', "FILE/DIR",  OPTION_ARG_OPTIONAL,
        "Delete FILE or DIRECTORY from watchlist" },
    {0,0,0,0, "Get stored information from databases"
              " (using the first argument)"},
    {"get", 'g', "Suboption", OPTION_ARG_OPTIONAL,
     "Get Record for given File or Directory"
        "(current working directory, if none provided)"},
    {0,0,0,0, "suboptions of \"get\" options (-g opt1=arg,opt2,opt3=arg)"},
	{"get", 'g', ",format=[text(t),json(j),interactive(i)]",0,
     "Selection of output format. Interactive mode is not yet implemented"},
    {"get", 'g', ",ex=[key(k),name(n),timestamp(t)]", 0,
     "Triggers process retrieval and alows for specifiers on how to retrieve"
     " them. default is key retrieval (if \"ex\" w/o args)"
     "The first non option argument (arg1) is the input for retrieval."
     "\n key: timestamp+pid\n name: process_name\n timestamp: sec.usec"
     " (sec is in epoch time )" },
    {"get", 'g', ",no_as", 0, "Do not get associated open or exec calls."
     "Only retrieve what you asked for."},
    {0,0,0,0, "Standard Otions"},
    {"abort",    OPT_ABORT, 0, 0, "Abort before showing any output"},
};

struct arguments
{
  	int start, stop, status, abort, restart,
        get, format, eCallMode, no_associated;
  	char *add_file;
	char *remove_file;    /* file arg to ‘--output’ */
    char *arg1;
    char *retrievePath;
};


enum {
    FORMAT_OPTION = 0,
    OCALLS_FROM_ECALL_KEY_OPTION,
    NO_ASSOCIATED,
    THE_END
};


enum {
    TEXT = 0,
    JSON = 1,
    INTERACTIVE =2
};

//Enum for type of execCall retrieval
enum {
    KEY = 1,
    NAME,
    TIME_STAMP
};

char *get_opts[] = {
    [FORMAT_OPTION] = "format",
    [OCALLS_FROM_ECALL_KEY_OPTION] = "ex",
    [NO_ASSOCIATED] = "no_as",
    [THE_END] = NULL
};

static error_t parse_opt (int key, char *arg, struct argp_state *state){
	/* Get the input argument from argp_parse, which we
     * know is a pointer to our arguments structure. */
	opterr = 0;
    char* subopts;
    char* value;
    struct arguments *arguments = state->input;

	switch (key){
		case 's':
		  	arguments->start = 1;
		  	break;
		case 'q':
		  	arguments->stop = 1;
		  	break;
        case 'g':
            arguments->get = 1;
            subopts = arg;
            if(!arg){
                break;
            }
            while(*subopts != '\0'){
                switch( getsubopt(&subopts, get_opts, &value)){
                    case FORMAT_OPTION:
                        if( value == NULL || !strncmp("text",value,1) ){
                            arguments->format = TEXT;
                        }
                        else if(!strncmp("json",value,1)){
                            arguments->format = JSON;
                        }
                        else if(!strncmp("interactive",value,1)){
                            arguments->format = INTERACTIVE;
                        }
                        else{
                            fprintf(stderr, "Subcommand not recognized!\n");
                        }
                        break;
                    case OCALLS_FROM_ECALL_KEY_OPTION:
                        if( value == NULL || !strncmp("key",value,1)){
                            arguments->eCallMode = KEY;
                        }
                        else if (!strncmp("name",value,1)){
                            arguments->eCallMode = NAME;
                        }
                        else if (!strncmp("timestamp", value, 1)){
                            arguments->eCallMode = TIME_STAMP;
                        }
                        else{
                            arguments->eCallMode = -1;
                        }
                        break;
                    case NO_ASSOCIATED:
                        arguments->no_associated = 1;
                        break;
                    default:
                        fprintf(stderr, "No suboption detected!\n"
                                "Supplied: %s\n", arg);
                }
            }

        case 'r':
            arguments->restart = 1;
            break;
        case 'u':
		  	arguments->status = 1;
		  	break;
		case 'a':
		  	realpath(arg ? arg : "./", arguments->add_file);
		  	break;
		case 'd':
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
		  arguments->arg1 = arg;

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


int get_from_database(struct arguments* args){
    int ret = 0;
    stringBuffer ret_stringbuffer;
    ret = sb_init(&ret_stringbuffer,256);
    db_manager db_man;
    db_man_init(&db_man, DATABASE_DIR);
    createDatabase(&db_man);
    // Result vector
    vector result_recordv;
    vector_init(&result_recordv, 32, sizeof(db_full_Record*));
    if(!args->eCallMode){
        realpath( args->arg1 ? args->arg1 : "./", args->retrievePath );
        retrieveRecords_Path(&result_recordv, &db_man,
                             args->retrievePath);
    }
    switch(args->format){
        case TEXT:
            if(!result_recordv.length){
                printf("No Records Found for query:\n %s\n",
                       args->retrievePath);
            }
            else{
                printf("Text return not implemented yet\n");
            }
            break;
        case JSON:
            sb_append(&ret_stringbuffer,"[");
            for(unsigned ii=0; ii<result_recordv.length; ii++){
                db_full_Record_jsonsb(result_recordv.elems[ii],
                                      &ret_stringbuffer);
                sb_append(&ret_stringbuffer,", ");
            }
            sb_append(&ret_stringbuffer,"]\n");
            printf("%s\n", ret_stringbuffer.string);
            break;
        case INTERACTIVE:
            if(!result_recordv.length){
                printf("No Records Found for query:\n %s\n",
                       args->retrievePath);
            }
            else{
                printf("Interactive mode not implemented yet\n");
            }
            break;
        default:
            printf("Error occured: fell through format switch\n"
                   "Enum value: %i\n", args->format);

    }
    vector_destroyd( &result_recordv, db_void_full_Record_destroy);
    db_man_close(&db_man);
    sb_destroy(&ret_stringbuffer);
    return ret;
}


int main (int argc, char** argv){
	struct arguments args;
	char ctrl_socket_addr[] = "/tmp/opentrace_ctl.socket";
    int ret = 0;
    args.add_file = calloc(PATH_MAX+1,sizeof(char));
	if(!args.add_file)
    {
        perror("cannot allocate add file");
        exit(EXIT_FAILURE);
    }
    args.remove_file = calloc(PATH_MAX+1, sizeof(char));
    if(!args.remove_file)
    {
        free(args.add_file);
        perror("file name");
        exit(EXIT_FAILURE);
    }
    args.retrievePath = calloc(PATH_MAX+1, sizeof(char));
    if(!args.retrievePath){
        perror("could not allocate first argument");
        exit(EXIT_FAILURE);
    }

    args.abort = 0;
    args.eCallMode = 0;
    args.get = 0;
    args.restart = 0;
    args.status = 0;
    args.start = 0;
    args.stop = 0;
    args.format = TEXT;
    argp_parse (&argp, argc, argv, 0, 0, &args);

    if(args.abort)
        error (10, 0, "ABORTED");
    if(1 < (args.start + args.stop))
        error (11, 0, "START and STOP exclude eachother");
    if(args.stop && args.restart)
        error (11, 0, "Cannot STOP and RESTART at the same time");
    if(args.get){
        ret = get_from_database(&args);
        goto endprog;
    }
    if(!(args.stop || args.start || args.status ||
         args.add_file[0] || args.remove_file[0] || args.restart || args.get))
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
    fcntl(so_fd, F_SETFD, FD_CLOEXEC);
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
            pid_t pid;
            if((pid = fork())< 0){
                fprintf(stderr,"Fork failed\n");
                ret = -1;
                goto endprog;
            }
            if(!pid){
                ret = execl(INSTALL_FOLDER "/watchdog_daemon",
                      INSTALL_FOLDER "/watchdog_daemon",
                      (char*) NULL);
                if (ret) {
                    fprintf(stderr,"Could not execute:"
                            "%s\n",INSTALL_FOLDER "/watchdog_daemon\n");
                    perror("Error");
                }
                return 0;
            }
            goto endprog;

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
    if(args.restart)
    {
        sprintf(request, "restart$");
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
    free(args.retrievePath);
    exit(ret ? EXIT_FAILURE : EXIT_SUCCESS);
}
