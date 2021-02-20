//
// Created by 310165137 on 10/02/2021.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hiredis/hiredis.h"
#include "database.h"
#include "cJSON.h"
#include "base64.h"
#include "sharedmemory.h"


extern void *shmem;
extern unsigned int poll_redis_data_white_list;

struct database_cache history[MAX_CACHE_SIZE] = {0}, current[MAX_CACHE_SIZE] = {0};
struct whitelist whlist_cache[MAX_WHITE_LIST_SIZE] = {0};

redisContext * ConnectServer(const char *hostname, const int port_num);

void ParseWhiteListFile(const char *buffer, long bufferlen);

void ProcessWhiteList();

void PrasenPrint_ServerResponse(redisReply *reply)
{
    if (NULL == reply)
    {
        return;
    }
    switch (reply->type) {
        case REDIS_REPLY_STRING:
            printf("Reply: %s \n",reply->str);
            break;
        case REDIS_REPLY_ARRAY:
            break;
        case REDIS_REPLY_INTEGER:
            printf("Reply: %lld",reply->integer);
            break;
        case REDIS_REPLY_NIL:
            break;
        case REDIS_REPLY_STATUS:
            break;
        case REDIS_REPLY_ERROR:
            break;
        case REDIS_REPLY_DOUBLE:
            break;
        case REDIS_REPLY_BOOL:
            break;
        case REDIS_REPLY_MAP:
            break;
        case REDIS_REPLY_SET:
            break;
        case REDIS_REPLY_ATTR:
            break;
        case REDIS_REPLY_PUSH:
            break;
        case REDIS_REPLY_BIGNUM:
            break;
        case REDIS_REPLY_VERB:
            break;
        default:
            printf("Unknown type \n");
            break;

    }
}

void *pthread_database_routine(void *arg)
{
    static unsigned int cache_poll_redis_data_white_list = 0;
    unsigned int j, isunix = 0;
    redisContext *c;
    redisReply *reply;
    const char *hostname =  "localhost";
    const int port_num = 6379;
    const char *database_key = "carList";
    c = ConnectServer( hostname, port_num);

    /* PING server */
    reply = redisCommand(c,"PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);



    while(1)
    {
        if (cache_poll_redis_data_white_list <  poll_redis_data_white_list)
        {
            ProcessWhiteList();
            cache_poll_redis_data_white_list = poll_redis_data_white_list;
        }

        int number_of_records = 0;
        /*Get the number of the elements in the database*/
        sleep(1);

        /* Reset the state */
        memcpy(history, current, sizeof(struct database_cache) * MAX_CACHE_SIZE);
        memset(current, 0x00, sizeof(struct database_cache) * MAX_CACHE_SIZE);

        /* PING server */
        reply = redisCommand(c,"LLEN %s", database_key);
        PrasenPrint_ServerResponse(reply);
        if (reply && reply->type == REDIS_REPLY_INTEGER)
        {
            number_of_records = reply->integer;
        }
        freeReplyObject(reply);

        /*We got the number of elements in the database*/
        for (int i = 0; i < number_of_records; i++)
        {
            /* PING server */
            reply = redisCommand(c,"LINDEX %s %d", database_key, i);
            if (reply)
            {
                cJSON *json = cJSON_ParseWithLength(reply->str, reply->len);
                char *string = cJSON_Print(json);
                if (json)
                {
                    /* Save message */
                    if (reply->type == REDIS_REPLY_STRING) {
                        memcpy(current[i].message, reply->str, reply->len);
                    }

                    const cJSON *adId = NULL;
                    /* Get the key : adId*/
                    adId = cJSON_GetObjectItemCaseSensitive(json, "adId");
                    if (cJSON_IsString(adId) && (adId->valuestring != NULL)) {
                        current[i].key = atoi(adId->valuestring);
                    }


                    const cJSON *type = NULL;
                    /* Get the permissions : type*/
                    type = cJSON_GetObjectItemCaseSensitive(json, "type");
                    if (cJSON_IsString(type) && (type->valuestring != NULL)) {
                        memcpy(current[i].type, type->valuestring, strlen(type->valuestring));
                    }

                    const cJSON *autoCall = NULL;
                    /* Get the permissions : type*/
                    autoCall = cJSON_GetObjectItemCaseSensitive(json, "autoCall");
                    if (cJSON_IsBool(autoCall)) {
                        current[i].auto_call = autoCall->valueint;
                    }

                    current[i].valid = 1;
                }
                cJSON_Delete(json);
                freeReplyObject(reply);
            }
        }
        sharedmem_writeDatabase(shmem, current, sizeof(current));
    }
}

void ProcessWhiteList() {
    char *buffer = NULL;
    long bufferlen = 0;
    if(load_file_into_memory("getWhitelist.json", &buffer, &bufferlen) == 0)
    {
        ParseWhiteListFile(buffer, bufferlen);
        sharedmem_writeWhiteList(shmem, whlist_cache, sizeof(whlist_cache));
    }
}

void ParseWhiteListFile(const char *buffer, long bufferlen) {
    int deviceCount = 0;
    cJSON *iterator = NULL;
    cJSON *json = cJSON_ParseWithLength(buffer,bufferlen);
    char *string = cJSON_Print(json);
    printf("%s\n",string);

    cJSON_ArrayForEach(iterator, json) {
        char permission[10][10] = {{0u}};
        int percount = 0;
        if (cJSON_IsObject(iterator))
        {
            cJSON *permissions_array = cJSON_GetObjectItem(iterator, "permissions");
            cJSON *permissions_iterator = NULL;
            cJSON_ArrayForEach(permissions_iterator, permissions_array)
            {
                if (cJSON_IsString(permissions_iterator))
                {
                   memcpy(permission[percount], permissions_iterator->valuestring, 10);
                   percount++;
                }
            }

            cJSON *devices_array = NULL;
            cJSON *devices_iterator = NULL;
            devices_array = cJSON_GetObjectItem(iterator, "devices");
            cJSON_ArrayForEach(devices_iterator, devices_array)
            {
                if (cJSON_IsObject(devices_iterator))
                {
                    const cJSON *deviceId = NULL;
                    deviceId = cJSON_GetObjectItemCaseSensitive(devices_iterator, "id");
                    if (cJSON_IsString(deviceId))
                    {
                        memcpy(whlist_cache[deviceCount].deviceId, deviceId->valuestring, 16);
                        for (int i = 0; i < percount; i++)
                        {
                            memcpy(whlist_cache[deviceCount].permissions[i], permission[i],10);
                        }
                        deviceCount++;
                    }
                }
            }
        }
    }
    cJSON_Delete(json);
}


int load_file_into_memory(const char *filename, char **buffer, long *bufflen)
{
    *bufflen = 0;
    *buffer = NULL;
    char *source = NULL;

    int retval = -1;
    FILE *fp = fopen(filename, "r");
    if (fp != NULL)
    {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0)
        {
            /* Get the size of the file. */
            long bufsize = ftell(fp);
            if (bufsize >= 0)
            {
                /* Allocate our buffer to that size. */
                source = (char*) malloc(sizeof(char) * (bufsize + 1 ));
                *bufflen = bufsize + 1;
                if (source)
                {
                    /* Go back to the start of the file. */
                    if (fseek(fp, 0L, SEEK_SET) == 0)
                    {
                        /* Read the entire file into memory. */
                        size_t newLen = fread(source, sizeof(char), bufsize, fp);
                        if (ferror(fp) != 0)
                        {
                            fputs("Error reading file", stderr);
                        }
                        else
                        {
                            source[newLen] = '\0'; /* Just to be safe. */
                            retval = 0;
                        }
                    }
                }
            }
        }
        fclose(fp);
    }
    *buffer = source;
    return retval;
}



redisContext * ConnectServer(const char *hostname, const int port_num)
{
    redisContext *c;
    redisReply *reply;
    struct timeval timeout = {1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port_num, timeout);
    if (c == NULL || c->err)
    {
        if (c)
        {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        }
        else
        {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    reply = redisCommand(c, "AUTH %s", "z6gXOlix0bhCJUDdlBBtePfrBoFGGoFs");
    if (reply->type == REDIS_REPLY_ERROR) {
        /* Authentication failed */
    }
    freeReplyObject(reply);
    return c;
}



