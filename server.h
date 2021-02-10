//
// Created by 310165137 on 09/02/2021.
//

#ifndef MULTITHREADED_SERVER_REDIS_SERVER_H
#define MULTITHREADED_SERVER_REDIS_SERVER_H

typedef struct pthread_arg_t {
    int new_socket_fd;
    struct sockaddr_in client_address;
    /* TODO: Put arguments passed to threads here. See lines 116 and 139. */
} pthread_arg_t;

#endif //MULTITHREADED_SERVER_REDIS_SERVER_H
