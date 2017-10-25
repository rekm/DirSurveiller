#include <stdlib.h>
#include <stdio.h>
#include "database.h"
#include "DirSurveillerConfig.h"
#include "string.h"


//  #####  Marshalled Object #####  //

void m_object_init(m_object *m_obj){
    m_obj->m_obj = NULL;
    m_obj->size = 0;
}

void m_object_destroy(m_object *this){
    free(this->m_obj);
    this->m_obj = NULL;
}
//  ##### DB-ExecCall #####  //

void m_object_add(m_object* this, void* data, size_t size){
    memcpy(this->m_obj+this->size, data, size);
    this->size += size;
}

int m_object_m_obj_getstring(char** target, char* m_obj){
    size_t length = 0;
    length = strlen(m_obj)+1;
    if(*target)
       free(*target);
    *target = calloc( length, sizeof(char));
    if(!*target){
        return -1;
    }
    memcpy(*target, m_obj, length);
    return length;
}

int db_execCall_marshall(m_object* m_obj, eCallRecord* eCall){
    if(!m_obj){
        size_t nSize = sizeof(eCall->time_stamp)
                     + sizeof(eCall->pid)
                     + sizeof(eCall->ppid)
                     + strlen(eCall->cmd_name) + 1
                     + strlen(eCall->args)     + 1
                     + strlen(eCall->env_args) + 1;
        m_obj = malloc(nSize);
        if(!m_obj){
            return 1;
        }
        m_object_add(m_obj,&eCall->time_stamp,sizeof(eCall->time_stamp));
        m_object_add(m_obj,&eCall->pid,sizeof(eCall->pid));
        m_object_add(m_obj,&eCall->ppid,sizeof(eCall->ppid));
        m_object_add(m_obj,&eCall->cmd_name,strlen(eCall->cmd_name)+1);
        m_object_add(m_obj,&eCall->args,strlen(eCall->args)        +1);
        m_object_add(m_obj,&eCall->env_args,strlen(eCall->env_args)+1);
        return 0;
    }
    return -1;
}

int db_execCall_unmarshall(eCallRecord* eCall ,m_object* m_obj){
    int strlength = 0;
    size_t bufpos = 0;
    eCall->time_stamp = *((struct timeval*)m_obj->m_obj); //timestamp
    bufpos += sizeof(eCall->time_stamp);
    eCall->pid= *((pid_t*)m_obj->m_obj                + bufpos); //pid
    bufpos += sizeof(eCall->pid);
    eCall->ppid= *((pid_t*)m_obj->m_obj               + bufpos); //ppid
    bufpos += sizeof(eCall->ppid);
    strlength = m_object_m_obj_getstring(&eCall->cmd_name,
                                         m_obj->m_obj + bufpos);
    if(strlength == -1) return 1;
    bufpos += strlength;
    strlength = m_object_m_obj_getstring(&eCall->args,
                                         m_obj->m_obj + bufpos);
    if(strlength == -1) return 1;
    bufpos += strlength;
    strlength = m_object_m_obj_getstring(&eCall->env_args,
                                         m_obj->m_obj + bufpos);
    if(strlength == -1) return 1;
    bufpos += strlength;
    return 0;
}

int db_man_init(db_manager* this, const char* database_dir){
    this->errorFileP = stderr;
    this->stdFileP = stdout;
    this->openCallDBp = NULL;
    this->execCallDBp = NULL;
    this->watchlistDBp = NULL;
    this->database_dir = database_dir;
    return 0;
}




int createDatabase(db_manager* db_man){
    DB_ENV *g_dbenv = db_man->dbenv;
    int ret;
    ret = db_env_create(&db_man->dbenv, 0);
    if(ret){
        fprintf(db_man->errorFileP, "Error creating env handle: %s\n", db_strerror(ret));
        return -1;
    }
    //Setup Log
    g_dbenv->set_lg_dir(g_dbenv, DATABASE_DIR);
    //Handle open
    ret = g_dbenv->open(g_dbenv,DATABASE_DIR,
              DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN
              | DB_THREAD, 0644);
	if (ret) {
   		fprintf(db_man->errorFileP, "Environment open failed: %s", db_strerror(ret));
    	return -1;
	}

    //OpenCall Database Setup

    ret = db_create(&db_man->openCallDBp, g_dbenv,0) == 0;
    if (ret){
   		fprintf(db_man->errorFileP, "Error creating db: %s", db_strerror(ret));
    	return -1;
    }
    //Config

    //Setup openCall database
    ret = db_man->openCallDBp->open(db_man->openCallDBp, NULL, "openCalls.db", NULL, DB_BTREE,
                    DB_AUTO_COMMIT | DB_CREATE | DB_THREAD, 0644) == 0;
    DBT oCall_key_dbt, oCall_data_dbt;

    memset(&oCall_key_dbt, 0, sizeof(oCall_key_dbt));
    memset(&oCall_data_dbt, 0, sizeof(oCall_data_dbt));

    //Setup OpenWatchlist database

    ret = db_create(&db_man->execCallDBp, g_dbenv,0);
    if (ret){
   		fprintf(db_man->errorFileP, "Error creating db: %s", db_strerror(ret));
    	return -1;
    }

    ret = db_man->watchlistDBp->open(db_man->watchlistDBp, NULL, "watchlist.db", NULL, DB_BTREE,
                    DB_AUTO_COMMIT | DB_CREATE | DB_THREAD, 0644);

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



void db_add_execCall(eCallRecord* db_eCall){

}

void db_add_openCall(oCallRecord* db_oCall){}
