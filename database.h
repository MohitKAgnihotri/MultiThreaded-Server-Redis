//
// Created by 310165137 on 10/02/2021.
//

#ifndef SERVER_DATABASE_H
#define SERVER_DATABASE_H

#define MAX_CACHE_SIZE 40
#define VALID_CACHE_ENTRY 1
#define INVALID_CACHE_ENTRY 0

void *pthread_database_routine(void *arg);

struct database_cache
{
    int valid;
    int key;
    char type[10];
    char message[3000];
};

#endif //SERVER_DATABASE_H
