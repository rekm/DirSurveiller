#ifndef _DATABASE_H
#define _DATABASE_H






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
void db_add_execCall( *struct db_execCall );

/**
 * @brief add openCall
 */
void db_add_openCal( *struct db_openCall );




/**
 * ExecCall Database Record
 */
struct db_execCall {
    //Primary Key
    //commandName  <- Secondary Index
    //PID
    //PPID
    //TimeStamp
    //Arguments
};

/**
 * OpenCall Database Record
 */
struct db_openCall {
    //ExecCall Key
    //FilePath <- Secondary Index  (Seems like a bad idea)
    //TimeStamp
    //Flag
};






#endif
