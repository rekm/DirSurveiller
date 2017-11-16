#include "containers.h"
#include <database.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>





int marshalling_test1(void){
    int ret = 0;
    eCallRecord eCall, n_eCall;
    db_execCall_init(&n_eCall);
    eCall.pid = 42;
    eCall.ppid = 21;
    eCall.parentEcall.pid = 21;
    eCall.parentEcall.time_stamp.tv_sec = 21;
    eCall.parentEcall.time_stamp.tv_usec =21;
    eCall.cmd_name = "answer";
    eCall.args = "to everything";
    eCall.env_args = "and more";
    eCall.time_stamp.tv_usec = 42;
    eCall.time_stamp.tv_sec = 42;
    m_object buffer;
    m_object_init(&buffer);
    db_execCall_marshall(&buffer, &eCall);
    db_execCall_unmarshall(&n_eCall, &buffer);
    ret = strcmp(eCall.env_args, n_eCall.env_args);
    m_object_destroy(&buffer);
    printf("%s\n", ret ? "FAILED":"PASSED");
    return ret;
}

int marshalling_test2(void){
    int ret;
    oCallRecord oCall, n_oCall;
    db_openCall_init(&n_oCall);
    oCall.flag = 1;
    oCall.eCallKey.time_stamp.tv_sec = 21;
    oCall.eCallKey.time_stamp.tv_usec =21;
    oCall.filepath = "answer/to/everything";
    oCall.time_stamp.tv_usec = 42;
    oCall.time_stamp.tv_sec = 42;
    m_object buffer;
    m_object_init(&buffer);
    db_openCall_marshall(&buffer, &oCall);
    db_openCall_unmarshall(&n_oCall, &buffer);
    ret = strcmp(oCall.filepath, n_oCall.filepath);
    m_object_destroy(&buffer);
    printf("%s\n", ret ? "FAILED":"PASSED");
    return ret;
}

int openCall2json_test1(void){
    int ret = 0;
    oCallRecord oCall, n_oCall;
    db_openCall_init(&n_oCall);
    oCall.flag = 1;
    oCall.eCallKey.time_stamp.tv_sec = 21;
    oCall.eCallKey.time_stamp.tv_usec =21;
    oCall.filepath = "answer/to/everything";
    oCall.time_stamp.tv_usec = 42;
    oCall.time_stamp.tv_sec = 42;
    stringBuffer sb;
    sb_init(&sb, 128);
    db_openCall_to_jsonstringb(&sb, &oCall);
    printf("\n%s\n", sb.string);
    printf("%s\n", ret ? "FAILED":"PASSED");
    sb_destroy(&sb);
    return ret;
}

int execCall2json_test1(void){
    int ret = 0;
    eCallRecord eCall,peCall;
    eCall.pid = 42;
    eCall.ppid = 21;
    eCall.parentEcall.pid = 21;
    eCall.parentEcall.time_stamp.tv_sec = 21;
    eCall.parentEcall.time_stamp.tv_usec =21;
    eCall.cmd_name = "answer";
    eCall.args = "to everything";
    eCall.env_args = "and more";
    eCall.time_stamp.tv_usec = 42;
    eCall.time_stamp.tv_sec = 42;

    peCall.pid = 21;
    peCall.ppid = 21;
    peCall.parentEcall.pid = 20;
    peCall.parentEcall.time_stamp.tv_sec = 20;
    peCall.parentEcall.time_stamp.tv_usec = 20;
    peCall.cmd_name = "what is";
    peCall.args = "the answer";
    peCall.env_args = "?";
    peCall.time_stamp.tv_usec = 21;
    peCall.time_stamp.tv_sec = 21;
    stringBuffer sb;

    sb_init(&sb, 128);
    db_execCall_to_jsonstringb(&sb, &eCall);
    sb_append(&sb,"\n");
    db_execCall_to_jsonstringb(&sb, &peCall);
    printf("\n%s\n", sb.string);
    printf("%s\n", ret ? "FAILED":"PASSED");
    sb_destroy(&sb);
    return ret;
}

int execCallRecord_keygen_test(){
    int ret = 0;
    eCallRecord eCall;
    db_eCallKey trueKey, generatedKey;
    eCall.pid = 42;
    eCall.ppid = 21;
    eCall.parentEcall.pid = 21;
    eCall.parentEcall.time_stamp.tv_sec = 21;
    eCall.parentEcall.time_stamp.tv_usec =21;
    eCall.cmd_name = "answer";
    eCall.args = "to everything";
    eCall.env_args = "and more";
    eCall.time_stamp.tv_usec = 42;
    eCall.time_stamp.tv_sec = 42;
    db_execCall_genKey(&generatedKey, &eCall);
    trueKey.pid = eCall.pid;
    trueKey.time_stamp = eCall.time_stamp;
    ret = !(trueKey.time_stamp.tv_sec == generatedKey.time_stamp.tv_sec
            & trueKey.time_stamp.tv_usec == generatedKey.time_stamp.tv_usec
            & trueKey.pid == generatedKey.pid);
    printf("%s\n", ret ? "FAILED":"PASSED");
    return ret;
}


int main(int argc, char** argv){
    int numArgs = 2;
    int ret = 0;
    if(argc != numArgs){
       printf("ERROR: Tests requiers %i arguments!\n", numArgs);
       exit(EXIT_FAILURE);
    }
    int choice = atoi(argv[1]);
    switch (choice){
        case 1:
            ret = marshalling_test1();
            break;
        case 2:
            ret = marshalling_test2();
            break;
        case 3:
            ret = openCall2json_test1();
            break;
        case 4:
            ret = execCall2json_test1();
            break;
        case 5:
            ret = execCallRecord_keygen_test();
            break;
        default:
            printf("ERROR: There is no TEST with number %i!\n",
                   choice);
            ret = 1;
    }

    if(ret)
        exit(EXIT_FAILURE);
    return 0;
}
