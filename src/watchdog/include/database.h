#ifndef _DATABASE_H
#define _DATABASE_H

#include <containers.h>
#include <db.h>
#include <sys/types.h>
#include <sys/time.h>
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

int db_execCall_to_jsonstringb(stringBuffer* jsonBuffer,
                               eCallRecord* eCall);
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

int db_openCall_to_jsonstringb(stringBuffer* jsonBuffer,
                               oCallRecord* oCall);
/**
 * Full Records associated with openCall
 */
typedef struct db_full_records_openCall{
    oCallRecord openCall;
    vector execCalls;
}db_full_Record;



int db_full_Record_init(db_full_Record* this);

void db_full_Record_destroy(db_full_Record* this);
void db_void_full_Record_destroy(void* this);

int db_full_Record_jsonsb(db_full_Record* this, stringBuffer* sb);
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
 *
 * Recordlist will be filled with full open records which are constituted
 * by an openCallRecord and the associated execCalls, where th first execCall
 * is the one that called the open syscall and next are the parent processes.
 *
 * @param recordList: db_full_Record** non initialized where first pointer is
 *      Null ((recordList)->[NULL])
 * @returns: 0 on Success
 *           1 on Memory allocation error
 *           2 recordList non empty
 */
int retrieveRecords_Path(vector* recordList,
                         db_manager* db_man, char* path);

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

int db_get_execCall_by_key(db_manager* db_man, eCallRecord** db_eCall,
                           db_eCallKey* eCall_key);
/**
 * @brief add openCall
 */
int db_add_openCall(db_manager* db_man, oCallRecord* db_oCall );









#endif
