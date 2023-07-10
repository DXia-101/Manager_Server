#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include <string.h>

#define CFG_PATH "../conf/cfg.json"

int main(){
    int ret = 0;
    FILE *fp = NULL;

    fp = fopen(CFG_PATH,"rb");
    if(fp == NULL){
        perror("fopen");
        ret = -1;
        goto END;
    }

END:
    if(fp != NULL){
        fclose(fp);
    }
    printf("ret=%d\n",ret);
    return 0;
}
