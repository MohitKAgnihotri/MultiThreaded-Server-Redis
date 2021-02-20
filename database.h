//
// Created by 310165137 on 10/02/2021.
//

#include <stdbool.h>

#ifndef SERVER_DATABASE_H
#define SERVER_DATABASE_H

#define MAX_CACHE_SIZE 40
#define MAX_WHITE_LIST_SIZE 200
#define VALID_CACHE_ENTRY 1
#define INVALID_CACHE_ENTRY 0

void *pthread_database_routine(void *arg);

struct database_cache
{
    int valid;
    int key;
    bool auto_call;
    char type[10];
    char message[3000];
};

struct whitelist
{
    char deviceId[16];
    char permissions[10][10];
};


int load_file_into_memory(const char *filename, char **buffer, long *bufflen);
#endif //SERVER_DATABASE_H
