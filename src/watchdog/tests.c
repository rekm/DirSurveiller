#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "containers.h"


// Test for StringBuffer
int test_sb(){
    int ret = NOMINAL;
    stringBuffer sbuffer;
    ret = sb_init(&sbuffer, 16);
    if(ret){
        perror("Stringbuffer init failed");
        goto endfun;
    }
    ret = sb_append(&sbuffer, "This is a test string \t with a tab\nand extra");
    if(ret){
        perror("Stringbuffer append failed");
        goto destroyBuffer;
    }
    char* newString = NULL;
    ret = sb_stringCpy(&newString, &sbuffer);
    if(ret){
        perror("Stringbuffer to char* copy failed to allocate");
        zfree(&newString);
        goto destroyBuffer;
    }
    ret = strcmp(newString, sbuffer.string);
    if(ret){
        perror("sb_stringCpy did not copy string correctly");
        goto freeNewString;
    }
    stringBuffer sb_array[2];
    ret = sb_init(sb_array,8);
    ret = sb_init(sb_array+1,8);
    int curr_field = 0;
    int lfieldpos = 0;
    for(int i=0; i<sbuffer.end_pos; i++){
        switch (sbuffer.string[i]) {
            case '\t':
                sbuffer.string[i]='\0';
                sb_append(sb_array+curr_field,sbuffer.string+lfieldpos);
                lfieldpos = i+1;
                curr_field++;
                break;
            case '\n':
                sbuffer.string[i]='\0';
                sb_append(sb_array+curr_field,sbuffer.string+lfieldpos);
                lfieldpos = i+1;
                curr_field = 0;
                break;

        }
    }
    sb_deletehead(&sbuffer,lfieldpos);
    lfieldpos = 0;
    printf("fields: %s , %s\n buf: %s\n size: %zu, pos: %i\n",
            sb_array[0].string, sb_array[1].string, sbuffer.string,
            sbuffer.size, sbuffer.end_pos);
    sb_flush(sb_array);
    sb_flush(sb_array+1);
    printf("fields (after flush):\t%s,\t%s\n pos:\t%i\t%i\n",
            sb_array[0].string, sb_array[1].string,
            sb_array[0].end_pos, sb_array[1].end_pos);


    ret = sb_append(&sbuffer, "This is also a test string with and all\t\n hello");

    for(int i=0; i<sbuffer.end_pos; i++){
        switch (sbuffer.string[i]) {
            case '\t':
                sbuffer.string[i]='\0';
                sb_append(sb_array+curr_field,sbuffer.string+lfieldpos);
                lfieldpos = i+1;
                curr_field++;
                break;
            case '\n':
                sbuffer.string[i]='\0';
                sb_append(sb_array+curr_field,sbuffer.string+lfieldpos);
                lfieldpos = i+1;
                curr_field = 0;
                break;

        }
    }
    sb_deletehead(&sbuffer,lfieldpos);
    lfieldpos = 0;
    printf("fields: %s , %s\n buf: %s\n size: %zu, pos: %i\n",
            sb_array[0].string, sb_array[1].string, sbuffer.string,
            sbuffer.size, sbuffer.end_pos);
    sb_flush(sb_array);
    sb_flush(sb_array+1);
    printf("fields (after flush):\t%s,\t%s\n pos:\t%i\t%i\n",
            sb_array[0].string, sb_array[1].string,
            sb_array[0].end_pos, sb_array[1].end_pos);
    sb_destroy(sb_array);
    sb_destroy(sb_array+1);

freeNewString:
    zfree(&newString);
destroyBuffer:
    sb_destroy(&sbuffer);
endfun:
    return ret;
}

// Test for general Ringbuffers
int test_grb(){

    return NOMINAL;
}


int main(void){
    int ret = NOMINAL;

    ret = test_grb();
    printf("General RingBuffer Test: %s\n", ret ? "failed" : "succeeded");
    ret = test_sb();
    printf("StringBuffer Test: %s\n", ret ? "failed" : "succeeded");

    return EXIT_SUCCESS;
}
