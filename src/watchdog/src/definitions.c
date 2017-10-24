#include "definitions.h"
#include <stdarg.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
char* getVersion(){ return "0.0.1-dev";}


void log_print(FILE* file, char* fmt,...){
    //Set up timeval struct from sys/time
    struct timeval tv;
    // time.h format string
    char timestr[128];
    //get epoch time in seconds and nanoseconds
    gettimeofday(&tv,NULL);
    struct tm * p = localtime(&tv.tv_sec);
    //Timeformat <WeekDay> <Date> <Time>
    strftime(timestr, 128, "%a %x %X",p);
    //Variadic arguments ...
    va_list args;
    va_start(args, fmt);
    //TimeStamp
    fprintf(file, ">>>%s.%li: ",timestr,tv.tv_usec);
    //fprint (va_list edition)
    vfprintf(file, fmt, args);
    fprintf(file, "<<<\n");
}
