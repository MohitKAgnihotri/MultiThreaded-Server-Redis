//
// Created by 310165137 on 10/02/2021.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hiredis/hiredis.h"
#include "database.h"
#include "json.h"


#define MAX_CACHE_SIZE 40
#define VALID_CACHE_ENTRY 1
#define INVALID_CACHE_ENTRY 0

struct database_cache
{
    int valid;
    int seen;
    int key;
    char *type;
    char *message;
};

struct database_cache history[MAX_CACHE_SIZE], current[MAX_CACHE_SIZE];

redisContext * ConnectServer(const char *hostname, const int port_num);

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
        /*Get the number of the elements in the database*/
        sleep(1);

        /* Reset the state */
        memcpy(history, current, sizeof(struct database_cache) * MAX_CACHE_SIZE);
        memset(current, 0x00, sizeof(struct database_cache) * MAX_CACHE_SIZE);

        /* PING server */
        reply = redisCommand(c,"LLEN %s", database_key);
        PrasenPrint_ServerResponse(reply);
        freeReplyObject(reply);

        /*We got the number of elements in the database*/
        for (int i = 0; i < 30; i++)
        {
            /* PING server */
            reply = redisCommand(c,"LINDEX %s %d", database_key, i);
            PrasenPrint_ServerResponse(reply);

            json_value *parsed_json = json_parse(reply->str,reply->len);
            if (parsed_json)
            {
                json_print_parsed(parsed_json, 0);

                /* Save message */
                if (reply->type == REDIS_REPLY_STRING) {
                    current[i].message = (char *) malloc(sizeof(char) * reply->len);
                }

                /* Get the key : adId*/
                struct _json_value *value = json_find_key_value(parsed_json, "adId", 0);
                if (value && value->type == json_string) {
                    current[i].key = atoi(value->u.string.ptr);
                }

                /* Get the permissions : type*/
                value = json_find_key_value(parsed_json, "type", 0);
                if (value && value->type == json_string) {
                    current[i].type = (char *) malloc(sizeof(char) * value->u.string.length);
                    memcpy(current[i].type, value->u.string.ptr, value->u.string.length);
                }
                current[i].valid = 1;
                current[i].seen = 0;

                /*Check if the reterived key is already seen in the history*/
                for (int cache_loop = 0; cache_loop < MAX_CACHE_SIZE; cache_loop++)
                {
                    if (history[cache_loop].valid == VALID_CACHE_ENTRY && history[cache_loop].key == current[i].key)
                    {
                        current[i].seen = 1;
                        break;
                    }
                    else
                    {
                        /* Send to the threads to communicate */
                    }
                }
            }
            freeReplyObject(reply);
        }
    }
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



