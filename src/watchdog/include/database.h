#ifndef _DATABASE_H
#define _DATABASE_H

#include <db.h>
#include <sys/types.h>
#include <sys/time.h>
#include <containers.h>
#define MaxPathSuffixLength 100

/**
 * Marshalled Struct
 */
typedef struct marshalled_object {
    int size;
    char* buffer;
}m_object;

void m_object_init(m_object *m_obj);
void m_object_destroy(m_object *m_obj);


typedef struct db_execCallKey{
    struct timeval time_stamp;
    pid_t pid;
}db_eCallKey;

/**
 * ExecCall Database Record
 * Primary Key: TimeStamp+PID
 */
typedef struct db_execCall {
    pid_t pid; //PID <- Primary1
    pid_t ppid; //PPID
    db_eCallKey parentEcall;
    struct timeval time_stamp; //TimeStamp  <- Primary0
    char* cmd_name; //commandName  <- Secondary Index
    char* args; //Arguments
    char* env_args; //EnvironmentArgs
}eCallRecord;

/**
 * Generates a key for an execCall Record
 */
void db_execCall_genKey(db_eCallKey* eCallKey, eCallRecord* eCall);

void db_execCall_init(eCallRecord *this);
int db_execCall_marshall(m_object* m_obj, eCallRecord* eCall);
int db_execCall_unmarshall(eCallRecord* eCall, m_object* m_obj);

// ######### OPEN CALLS ########## //


typedef struct db_openCallKey{
    struct timeval time_stamp;
    char filepath_suffix[MaxPathSuffixLength];
}db_oCallKey;



/**
 * OpenCall Database Record
 * Primary Key: TimeStamp+last20charsOfFilename
 */
typedef struct db_openCall {
    int flag; //Flag
    struct timeval time_stamp; //TimeStamp
    db_eCallKey eCallKey; //ExecCall Key
    char* filepath; //FilePath <- Secondary Index  (Seems like a bad idea)
}oCallRecord;

/**
 * Generates a key for an openCall Record
 */
void db_openCall_genKey(db_oCallKey* oCallKey, oCallRecord* oCall);

void db_openCall_init(oCallRecord *this);
int db_openCall_marshall(m_object* m_obj, oCallRecord* oCall);
int db_openCall_unmarshall(oCallRecord* oCall, m_object* m_obj);
/**
 * General Database Management struct
 */
typedef struct db_manager{
    const char* database_dir;
    DB_ENV *dbenv;
    FILE* errorFileP;
    FILE* stdFileP;
    DB *openCallDBp;
    DB *openCallFilePathDBp; //Secondary Index Database
    DB *execCallDBp;
    DB *watchlistDBp;
}db_manager;

int db_man_init(db_manager*, const char* database_dir);
int db_man_setOutputFiles(const char* stdFile, const char* errorFile);
int db_man_open(db_manager*);

/**
 * Closing databases
 */
int db_man_close(db_manager*);




/**
 * @brief Getting all opencalls matching input file with execCalls
 */
void retrieveRecords_Path(db_manager* db_man, const char* path);

/**
 * @brief getting all opencalls matching input and
 */
void retrieveRecords_PathTimestamp(void);

/**
 * @brief create database
 */
int createDatabase(db_manager*);

/**
 * @brief add execCall
 */
int db_add_execCall(db_manager* db_man, eCallRecord* db_eCall );

/**
 * @brief add openCall
 */
int db_add_openCall(db_manager* db_man, oCallRecord* db_oCall );









#endif
