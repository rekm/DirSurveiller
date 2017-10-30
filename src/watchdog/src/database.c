#include <stdlib.h>
#include <stdio.h>
#include "database.h"
#include "DirSurveillerConfig.h"
#include "string.h"


//  #####  Marshalled Object #####  //

void m_object_init(m_object *m_obj){
    m_obj->buffer = NULL;
    m_obj->size = 0;
}

void m_object_destroy(m_object *this){
    free(this->buffer);
    this->buffer= NULL;
}

void m_object_add(m_object* this, void* data, size_t size){
    memcpy(this->buffer+this->size, data, size);
    this->size += size;
}

int m_object_buffer_getstring(char** target, char* m_obj_buffer){
    size_t length = 0;
    length = strlen(m_obj_buffer)+1;
    if(*target)
       free(*target);
    *target = calloc( length, sizeof(char));
    if(!*target){
        return -1;
    }
    memcpy(*target, m_obj_buffer, length);
    return length;
}

//  ##### DB-ExecCall #####  //

void db_execCall_init(eCallRecord *this){
    this->time_stamp.tv_sec = 0;
    this->time_stamp.tv_usec = 0;
    this->parentEcall.pid = 0;
    this->parentEcall.time_stamp.tv_sec = 0;
    this->parentEcall.time_stamp.tv_usec =0;
    this->pid = 0;
    this->ppid = 0;
    this->cmd_name = NULL;
    this->args = NULL;
    this->env_args = NULL;
}


int db_execCall_to_jsonstringb(stringBuffer* jsonBuffer,
                               eCallRecord* eCall){
    int ret = 0;
    int charsformated = 0;
    char buffer[300];
    ret = sb_append(jsonBuffer,"{");
    charsformated = snprintf(buffer, 300,
                             "\"time_stamp\": %li.%li, "
                             "\"pid\": %i, "
                             "\"ppid\": %i",
                             eCall->time_stamp.tv_sec,
                             eCall->time_stamp.tv_usec,
                             eCall->pid,
                             eCall->ppid);
    ret = sb_append(jsonBuffer,buffer);
    ret = sb_append(jsonBuffer, ", \"args\": \"");
    ret = sb_append(jsonBuffer, eCall->args);
    ret = sb_append(jsonBuffer, "\"");
    ret = sb_append(jsonBuffer,"}");
    return ret;
}

void db_execCall_genKey(db_eCallKey* eCallKey, eCallRecord* eCall){
    eCallKey->time_stamp = eCall->time_stamp;
    eCallKey->pid = eCall->pid;
}

int db_execCall_marshall(m_object* m_obj, eCallRecord* eCall){
    if(!m_obj->buffer){
        size_t nSize = sizeof(eCall->time_stamp)
                     + sizeof(eCall->pid)
                     + sizeof(eCall->ppid)
                     + strlen(eCall->cmd_name) + 1
                     + strlen(eCall->args)     + 1
                     + strlen(eCall->env_args) + 1;
        m_obj->buffer = malloc(nSize);
        if(!m_obj){
            return 1;
        }
        m_object_add(m_obj,&eCall->time_stamp,sizeof(eCall->time_stamp));
        m_object_add(m_obj,&eCall->pid,sizeof(eCall->pid));
        m_object_add(m_obj,&eCall->ppid,sizeof(eCall->ppid));
        m_object_add(m_obj,eCall->cmd_name,strlen(eCall->cmd_name)+1);
        m_object_add(m_obj,eCall->args,strlen(eCall->args)        +1);
        m_object_add(m_obj,eCall->env_args,strlen(eCall->env_args)+1);
        return 0;
    }
    return -1;
}

int db_execCall_unmarshall(eCallRecord* eCall ,m_object* m_obj){
    int strlength = 0;
    size_t bufpos = 0;
    eCall->time_stamp = *((struct timeval*)m_obj->buffer); //timestamp
    bufpos += sizeof(eCall->time_stamp);
    eCall->pid= *(pid_t*)(m_obj->buffer  + bufpos); //pid
    bufpos += sizeof(eCall->pid);
    eCall->ppid= *(pid_t*)(m_obj->buffer + bufpos); //ppid
    bufpos += sizeof(eCall->ppid);
    strlength = m_object_buffer_getstring(&eCall->cmd_name,
                                         m_obj->buffer+ bufpos);
    if(strlength == -1) return 1;
    bufpos += strlength;
    strlength = m_object_buffer_getstring(&eCall->args,
                                         m_obj->buffer+ bufpos);
    if(strlength == -1) return 1;
    bufpos += strlength;
    strlength = m_object_buffer_getstring(&eCall->env_args,
                                         m_obj->buffer+ bufpos);
    if(strlength == -1) return 1;
    bufpos += strlength;
    return 0;
}




// ########## OpenCalls ########## //

void db_openCall_init(oCallRecord *this){
    this->eCallKey.time_stamp.tv_usec=0;
    this->eCallKey.time_stamp.tv_sec=0;
    this->flag = 0;
    this->time_stamp.tv_sec=0;
    this->time_stamp.tv_usec=0;
    this->filepath = NULL;
}

void gen_filepath_suffix(char* suffix, char* filepath){
    size_t path_length;
    size_t start_pos;
    path_length = strlen(filepath) + 1;
    if(path_length < MaxPathSuffixLength){
        snprintf(suffix,
                 MaxPathSuffixLength,
                 "%s",filepath);
    }
    else{
        start_pos = path_length - MaxPathSuffixLength;
        snprintf(suffix,
                 MaxPathSuffixLength,
                 "%s",filepath+start_pos);
    }
}

void db_openCall_genKey(db_oCallKey* oCallKey, oCallRecord* oCall){
    oCallKey->time_stamp = oCall->time_stamp;
    gen_filepath_suffix(oCallKey->filepath_suffix,
                        oCall->filepath);
}

int db_openCall_marshall(m_object* m_obj, oCallRecord* oCall){
    if(!m_obj->buffer){
        size_t nSize = sizeof(oCall->time_stamp)
                     + sizeof(oCall->flag)
                     + sizeof(oCall->eCallKey)
                     + strlen(oCall->filepath) + 1;
        m_obj->buffer = malloc(nSize);
        if(!m_obj){
            return 1;
        }
        m_object_add(m_obj,&oCall->time_stamp,sizeof(oCall->time_stamp));
        m_object_add(m_obj,&oCall->flag,sizeof(oCall->flag));
        m_object_add(m_obj,&oCall->eCallKey,sizeof(oCall->eCallKey));
        m_object_add(m_obj,oCall->filepath,strlen(oCall->filepath)+1);
        return 0;
    }
    return -1;
}

int db_openCall_unmarshall(oCallRecord* oCall ,m_object* m_obj){
    int strlength = 0;
    size_t bufpos = 0;
    oCall->time_stamp = *((struct timeval*)m_obj->buffer); //timestamp
    bufpos += sizeof(oCall->time_stamp);
    oCall->flag= *(int*)(m_obj->buffer  + bufpos); //flag
    bufpos += sizeof(oCall->flag);
    oCall->eCallKey= *(db_eCallKey*)(m_obj->buffer + bufpos); //eCallKey
    bufpos += sizeof(oCall->eCallKey);
    strlength = m_object_buffer_getstring(&oCall->filepath, //filepath
                                          m_obj->buffer+ bufpos);
    if(strlength == -1) return 1;
    bufpos += strlength;
    return 0;
}


// ################ DB Manager ################ //


int db_man_init(db_manager* this, const char* database_dir){
    this->errorFileP = stderr;
    this->stdFileP = stdout;
    this->openCallDBp = NULL;
    this->openCallFilePathDBp = NULL;
    this->execCallDBp = NULL;
    this->watchlistDBp = NULL;
    this->database_dir = database_dir;
    return 0;
}


// ######## Secondary Index Callbacks ########## //

int db_openCalls_filepathCallback(DB* dbp,
                                  const DBT* key,
                                  const DBT* data,
                                  DBT* oCallFilepathKey){
    size_t start_pos = 0;
    size_t path_size = 0;
    start_pos += sizeof(((oCallRecord*)0)->time_stamp) +
                 sizeof(((oCallRecord*)0)->flag) +
                 sizeof(((oCallRecord*)0)->eCallKey);
    path_size = strlen(data->data+start_pos) + 1;

    oCallFilepathKey->data = data->data + start_pos;
    oCallFilepathKey->size = path_size;
    return 0;
}

// ######## Creating databases ######### //

int createDatabase(db_manager* db_man){
    DB_ENV *g_dbenv = db_man->dbenv;
    int ret;
    ret = db_env_create(&db_man->dbenv, 0);
    if(ret){
        fprintf(db_man->errorFileP, "Error creating env handle: %s\n",
                db_strerror(ret));
        return -1;
    }
    //Setup Log
    g_dbenv->set_lg_dir(g_dbenv, DATABASE_DIR);
    //Handle open
    ret = g_dbenv->open(g_dbenv,DATABASE_DIR,
              DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN
              | DB_THREAD, 0644);
	if (ret) {
   		fprintf(db_man->errorFileP, "Environment open failed: %s",
                db_strerror(ret));
    	return -1;
	}

    //OpenCall Database Setup
        // Creation
    ret = db_create(&db_man->openCallDBp, g_dbenv,0);
    if (ret){
   		fprintf(db_man->errorFileP, "Error creating db: %s", db_strerror(ret));
    	return -1;
    }
        //Config

        //Opening handle openCall database
    ret = db_man->openCallDBp->open(db_man->openCallDBp, NULL, "openCalls.db",
                                    NULL, DB_BTREE,
                                    DB_AUTO_COMMIT | DB_CREATE | DB_THREAD,
                                    0644);
        //Creating Secondary database/Index for filenames
    ret = db_create(&db_man->openCallFilePathDBp, g_dbenv,0);
    if (ret){
        //ToDo
    }
    ret = db_man->openCallFilePathDBp->set_flags(db_man->openCallFilePathDBp,
                                                 DB_DUPSORT);
        //Opening secondary database handle
    ret = db_man->openCallFilePathDBp->open(db_man->openCallFilePathDBp,
                                            NULL, "openCallsFilepath.db",
                                            NULL, DB_BTREE,
                                            DB_AUTO_COMMIT | DB_CREATE |
                                                DB_THREAD,
                                            0644);
    if (ret){
        //ToDo
    }
        /* Associate openCallFilePath.db with openCall.db to create
         * a secondary index. This useses the filepath callback function
         */
    ret = db_man->openCallDBp->associate(db_man->openCallDBp, NULL,
                                         db_man->openCallFilePathDBp,
                                         db_openCalls_filepathCallback,
                                         DB_CREATE);
    //Setup OpenWatchlist database

    ret = db_create(&db_man->execCallDBp, g_dbenv,0);
    if (ret){
   		fprintf(db_man->errorFileP, "Error creating db: %s", db_strerror(ret));
    	return -1;
    }

    ret = db_man->watchlistDBp->open(db_man->watchlistDBp, NULL,
                                     "watchlist.db", NULL, DB_BTREE,
                                     DB_AUTO_COMMIT | DB_CREATE | DB_THREAD,
                                     0644);

    //Setup execCall database

    ret = db_create(&db_man->execCallDBp, g_dbenv,0) == 0;
    if (ret){
   		fprintf(db_man->errorFileP, "Error creating db: %s", db_strerror(ret));
    	return -1;
    }

    ret = db_man->execCallDBp->open(db_man->execCallDBp, NULL, "execCalls.db", NULL, DB_BTREE,
                    DB_AUTO_COMMIT | DB_CREATE | DB_THREAD, 0644) == 0;

    g_dbenv->close(g_dbenv,0);
    return 0;
}

int db_man_open(db_manager *this){

    return 0;
}

int db_man_close(db_manager *this){
    int ret = 0;
    //Closing OpenCall Database
        //ClosingSecondaryIndex
    if(this->openCallFilePathDBp){
        ret = this->openCallFilePathDBp->close(this->openCallFilePathDBp,0);
        if(ret){
            fprintf(this->errorFileP, "Error deleting db: %s", db_strerror(ret));
        }
    }
        //Closing primary Database
    if(this->openCallDBp){
        ret = this->openCallDBp->close(this->openCallDBp,0);
        if(ret){
            fprintf(this->errorFileP, "Error deleting db: %s", db_strerror(ret));
        }
    }
    //Closing ExecCall Database
    if(this->execCallDBp){
        ret = this->execCallDBp->close(this->execCallDBp,0);
        if(ret){
            fprintf(this->errorFileP, "Error deleting db: %s", db_strerror(ret));
        }
    }
    //Closing WatchList Database
    if(this->watchlistDBp){
        ret = this->watchlistDBp->close(this->watchlistDBp,0);
        if(ret){
            fprintf(this->errorFileP, "Error deleting db: %s", db_strerror(ret));
        }
    }
    return ret;
}

void retrieveRecords_Path(db_manager* db_man, const char* path){


}



int db_add_execCall(db_manager* db_man, eCallRecord* db_eCall){
    int ret;
    DBT key, data;
    //Generate Key data struct from Record data struct
    db_eCallKey eCallKey;
    db_execCall_genKey(&eCallKey, db_eCall);
    //Prepare Marshall Object
    m_object m_obj;
    m_object_init(&m_obj);
    ret = db_execCall_marshall( &m_obj, db_eCall);
    //Zero out key and data
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    //Fill key
    key.data = &eCallKey;
    key.size = sizeof(eCallKey);
    //Fill data
    data.data = m_obj.buffer;
    data.size = m_obj.size;

    ret = db_man->execCallDBp->put(db_man->execCallDBp, NULL, &key, &data,
                                   DB_NOOVERWRITE);
    m_object_destroy(&m_obj);
    return ret;
}

int db_add_openCall(db_manager* db_man, oCallRecord* db_oCall){
    int ret;
    DBT key, data;
    //Generate Key data struct from Record data struct
    db_oCallKey oCallKey;
    db_openCall_genKey(&oCallKey, db_oCall);
    //Prepare Marshall Object
    m_object m_obj;
    m_object_init(&m_obj);
    ret = db_openCall_marshall( &m_obj, db_oCall);
    //Zero out key and data
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    //Fill key
    key.data = &oCallKey;
    key.size = sizeof(oCallKey);
    //Fill data
    data.data = m_obj.buffer;
    data.size = m_obj.size;

    ret = db_man->openCallDBp->put(db_man->openCallDBp, NULL, &key, &data,
                                   DB_NOOVERWRITE);
    m_object_destroy(&m_obj);
    return ret;
}
