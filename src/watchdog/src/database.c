
#include <db.h>
#include "database.h"
#include "DirSurveillerConfig.h"
#include "string.h"
void createDatabase(void){
    DB_ENV *dbenv;
    DB *dbp;
    int ret;
    ret = db_env_create(&dbenv, 0) == 0;
    //Setup Log
    dbenv->set_lg_dir(dbenv, DATABASE_DIR);
    //Handle open
    ret = dbenv->open(dbenv,DATABASE_DIR,
              DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN
              | DB_THREAD, 0644);

    ret = db_create(&dbp, dbenv,0) == 0;
    //Config

    ret = dbp->open(dbp, NULL, "openCalls.db", NULL, DB_BTREE,
                    DB_AUTO_COMMIT | DB_CREATE | DB_THREAD, 0644) == 0;
    DBT oCall_key_dbt, oCall_data_dbt;

    memset(&oCall_key_dbt, 0, sizeof(oCall_key_dbt));
    memset(&oCall_key_dbt, 0, sizeof(oCall_key_dbt));
    dbenv->close(dbenv,0);
}

void retrieveRecords_Path(void){

}



void db_add_execCall(eCallRecord* db_eCall){}

void db_add_openCall(oCallRecord* db_oCall){}
