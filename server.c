#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "database.h"
#include "sharedmemory.h"
#include "server.h"
#include "base64.h"
#include "cJSON.h"

#define BACKLOG 10
#define TIMER_INTERVAL 9e+8


unsigned int poll_redis_data_white_list;
unsigned int send_poll_to_clients;

pthread_mutex_t whiteListReq_mutex;


void *shmem;

/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

void SetupSignalHandler();

int CreateServerSocket(int port);

pthread_t CreateDataBaseConnection();


void setTimer()
{
    struct itimerval tv;
    tv.it_interval.tv_sec = 0;
    tv.it_interval.tv_usec = TIMER_INTERVAL;  // when timer expires, reset to 15 min
    tv.it_value.tv_sec = 0;
    tv.it_value.tv_usec = TIMER_INTERVAL;   // 15 min == 9e+8 us
    setitimer(ITIMER_REAL, &tv, NULL);
}

int main(int argc, char *argv[])
{
    int port, socket_fd, new_socket_fd;
    pthread_attr_t pthread_client_attr, pthread_database_attr;
    pthread_arg_t *pthread_arg;
    pthread_t pthread;
    socklen_t client_address_len;

    /* Get port from command line arguments or stdin. */
    port = argc > 1 ? atoi(argv[1]) : 0;
    if (!port) {
        printf("Enter Port: ");
        scanf("%d", &port);
    }
    /*Create the shared memory */
    shmem = create_shared_memory(SHM_SIZE);

    /*Create the thread for the Database */
    pthread = CreateDataBaseConnection();

    /*Create the server socket */
    socket_fd = CreateServerSocket(port);

    /*Setup the signal handler*/
    SetupSignalHandler();

    /*Setup timer for the 15 min*/
    setTimer();

    /*Setup mutex for the whitelistFile write operation*/
    pthread_mutex_init(&whiteListReq_mutex, NULL);

    /* Initialise pthread attribute to create detached threads. */
    if (pthread_attr_init(&pthread_client_attr) != 0) {
        perror("pthread_attr_init");
        exit(1);
    }
    if (pthread_attr_setdetachstate(&pthread_client_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("pthread_attr_setdetachstate");
        exit(1);
    }

    while (1) {
        /* Create pthread argument for each connection to client. */
        /* TODO: malloc'ing before accepting a connection causes only one small
         * memory when the program exits. It can be safely ignored.
         */
        pthread_arg = (pthread_arg_t *)malloc(sizeof *pthread_arg);
        if (!pthread_arg) {
            perror("malloc");
            continue;
        }

        /* Accept connection to client. */
        client_address_len = sizeof pthread_arg->client_address;
        new_socket_fd = accept(socket_fd, (struct sockaddr *)&pthread_arg->client_address, &client_address_len);
        if (new_socket_fd == -1) {
            perror("accept");
            free(pthread_arg);
            continue;
        }

        /* Initialise pthread argument. */
        pthread_arg->new_socket_fd = new_socket_fd;
        /* TODO: Initialise arguments passed to threads here. See lines 22 and
         * 139.
         */

        printf("Client connected\n");
        /* Create thread to serve connection to client. */
        if (pthread_create(&pthread, &pthread_client_attr, pthread_routine, (void *)pthread_arg) != 0) {
            perror("pthread_create");
            free(pthread_arg);
            continue;
        }
    }

    /* close(socket_fd);
     * TODO: If you really want to close the socket, you would do it in
     * signal_handler(), meaning socket_fd would need to be a global variable.
     */
    return 0;
}

pthread_t CreateDataBaseConnection() {
    /* Initialise pthread attribute to create detached threads. */
    pthread_t database_thread;
    pthread_attr_t pthread_database_attr;
    if (pthread_attr_init(&pthread_database_attr) != 0) {
        perror("pthread_attr_init");
        exit(1);
    }
    if (pthread_attr_setdetachstate(&pthread_database_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("pthread_attr_setdetachstate");
        exit(1);
    }
    if (pthread_create(&database_thread, &pthread_database_attr, pthread_database_routine, (void *)NULL) != 0) {
        perror("pthread_create");
    }
    return database_thread;
}

int CreateServerSocket(int port)
{
    struct sockaddr_in address;
    int socket_fd;

    /* Initialise IPv4 address. */
    memset(&address, 0, sizeof (address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    /* Create TCP socket. */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    /* Bind address to socket. */
    if (bind(socket_fd, (struct sockaddr *)&address, sizeof (address)) == -1) {
        perror("bind");
        exit(1);
    }

    /* Listen on socket. */
    if (listen(socket_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // Configure server socket
    int enable = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    return socket_fd;
}

void SetupSignalHandler() {/* Assign signal handlers to signals. */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    if (signal(SIGALRM,signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
}

bool hasPermission(char *type, char *permissions[10])
{
    for (int i = 0; ((i < 10) && (permissions[i] != NULL)); i++)
    {
        if (strcmp(type, permissions[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

char * CreateWhiteListReqJsonString(char* deviceId, char *ip_addr)
{
    cJSON *device = cJSON_CreateObject();

    cJSON_AddStringToObject(device, deviceId, deviceId);
    cJSON *ips = cJSON_AddArrayToObject(device, "ips");
    cJSON *ip = cJSON_CreateObject();
    cJSON_AddStringToObject(ip, "ip", ip_addr);
    cJSON_AddNumberToObject(ip, "timestamp", (unsigned long)time(NULL));
    cJSON_AddItemToArray(ips, ip);
    return cJSON_Print(device);
}

void *pthread_routine(void *arg)
{
    static unsigned int cache_send_poll_to_clients = 0;
    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int new_socket_fd = pthread_arg->new_socket_fd;

    struct sockaddr_in client_address;
    memcpy(&client_address, &pthread_arg->client_address, sizeof(struct sockaddr_in));

    free(arg);

    char *permissions[10] = {0};

    /*Get the device id (in base64)*/
    char message_buffer[1024];
    int read_len = read(new_socket_fd, message_buffer, sizeof(message_buffer));
    message_buffer[read_len] = '\0';

    /*This is expected to be a base-64 encoded deviceId*/
    size_t outlen;
    unsigned char * deviceId = base64_decode(message_buffer, read_len, &outlen);

    /* Check for the whitelist file */
    struct whitelist *whitelist = malloc(sizeof(struct whitelist) * MAX_WHITE_LIST_SIZE);
    sharedmem_readWhiteList(shmem, whitelist, sizeof(struct whitelist) * MAX_WHITE_LIST_SIZE);

    bool isDeviceWhiteListed = false;
    for (int i = 0; i < MAX_WHITE_LIST_SIZE; i++)
    {
        if (strncmp(deviceId, whitelist[i].deviceId,outlen) == 0)
        {
            isDeviceWhiteListed = true;
            for (int j = 0; j < 10; j++)
            {
                if (whitelist[i].permissions[j][0] !='\0') {
                    permissions[j] = (char *) malloc(sizeof(char) * 10);
                    memcpy(permissions[j], whitelist[i].permissions[j], 10);
                }
            }
        }
        if (isDeviceWhiteListed)
            break;
    }

    if (!isDeviceWhiteListed)
    {
        char *string = CreateWhiteListReqJsonString(deviceId, inet_ntoa(client_address.sin_addr));
        pthread_mutex_lock(&whiteListReq_mutex);
        FILE *fp = fopen("WhiteListReq.json","a");
        fprintf(fp,"%s",string);
        fclose(fp);
        pthread_mutex_unlock(&whiteListReq_mutex);

        //start clean-up
        free(whitelist);
        close(new_socket_fd);
        pthread_exit(NULL);
    }


    /* TODO: Get arguments passed to threads here. See lines 22 and 116. */
    struct database_cache history_client_storage[MAX_CACHE_SIZE] = {0};
    struct database_cache current_client_storage[MAX_CACHE_SIZE] = {0};

    free(arg);
    int found = 0;
    while (1)
    {
        sharedmem_ReadDatabase(shmem, current_client_storage, sizeof(current_client_storage));
        for (int i = 0; i < MAX_CACHE_SIZE; i++)
        {
            found = 0;
            for (int j = 0; j < MAX_CACHE_SIZE; j++)
            {
                if (current_client_storage[i].key == history_client_storage[j].key)
                {
                    found = 1;
                    break;
                }
            }

            if (!found && hasPermission(current_client_storage[i].type, permissions))
            {
                write(new_socket_fd, current_client_storage[i].message, strlen(current_client_storage[i].message));
            }
        }

        memcpy(history_client_storage, current_client_storage, sizeof(history_client_storage));
        if (cache_send_poll_to_clients < send_poll_to_clients)
        {
            write(new_socket_fd, "POLL", strlen("POLL"));
        }
    }

    close(new_socket_fd);
    return NULL;
}

void signal_handler(int signal_number)
{
    if (signal_number == SIGALRM)
    {
        poll_redis_data_white_list++;
        send_poll_to_clients++;
    }
    else
    {
            /* TODO: Put exit cleanup code here. */
            exit(0);
    }
}
