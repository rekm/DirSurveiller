#include "definitions.h"
#include <stdarg.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
char* getVersion(){ return "0.0.1-dev";}


void log_print(FILE* file, char* fmt,...){
    struct timeval tv;
    char timestr[128];
    gettimeofday(&tv,NULL);
    struct tm * p = localtime(&tv.tv_sec);
    strftime(timestr, 128, "%a %x %X",p);
    va_list args;
    va_start(args, fmt);
    fprintf(file, ">>>%s.%li: ",timestr,tv.tv_usec);
    vfprintf(file, fmt, args);
    fprintf(file, "<<<\n");
}
