#ifndef _DATABASE_H
#define _DATABASE_H

#include <sys/types.h>


/**
 * ExecCall Database Record
 */
typedef struct db_execCall {
    //Primary Key
    //commandName  <- Secondary Index
    //PID
    pid_t pid;
    //PPID
    //TimeStamp
    //Arguments
}eCallRecord;

/**
 * OpenCall Database Record
 */
typedef struct db_openCall {
    //ExecCall Key
    //FilePath <- Secondary Index  (Seems like a bad idea)
    //TimeStamp
    //Flag
    int flag;
}oCallRecord;

/**
 * @brief Getting all opencalls matching input file with execCalls
 */
void retrieveRecords_Path(void);


/**
 * @brief create database
 */
void createDatabase(void);

/**
 * @brief add execCall
 */
void db_add_execCall(eCallRecord* db_eCall );

/**
 * @brief add openCall
 */
void db_add_openCall(oCallRecord* db_oCall );









#endif
