#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "database.h"
#include "server.h"

#define BACKLOG 10


/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

void SetupSignalHandler();

int CreateServerSocket(int port);

pthread_t CreateDataBaseConnection();

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

    pthread = CreateDataBaseConnection();
    socket_fd = CreateServerSocket(port);
    SetupSignalHandler();

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
}

void *pthread_routine(void *arg) {
    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int new_socket_fd = pthread_arg->new_socket_fd;
    struct sockaddr_in client_address = pthread_arg->client_address;
    /* TODO: Get arguments passed to threads here. See lines 22 and 116. */

    free(arg);

    /* TODO: Put client interaction code here. For example, use
     * write(new_socket_fd,,) and read(new_socket_fd,,) to send and receive
     * messages with the client.
     */
    char message[] = "{\"adId\":\"442627441\",\"type\":\"pkw\",\"title\":\"Audi A3 Audi A3, S3 Klein-/ Kompaktwagen\",\"image\":\"https://cache.willhaben.at/mmo/1/442/627/441_1092933892.jpg\",\"price\":3999,\"price_display\":\"\\u20ac 3.999\",\"year\":\"08.2005\",\"km\":\"191.600\",\"kw\":77,\"ps\":\"104\",\"transmission\":\"Schaltgetriebe\",\"wheels\":\"Hinterrad\",\"location\":\"Taxach\",\"fuel\":\"Diesel\",\"plz\":5400,\"phonenumber\":\"\",\"phonenumber2\":\"\",\"phonenumberDesc\":\"\",\"link\":\"https://www.willhaben.at/iad/object?adId=442627441\",\"make\":\"Audi\",\"model\":\"A3\",\"autoCall\":false,\"scanProxy\":\"http://45.139.0.116:3128\",\"timeStart\":1612087924541,\"timeStartText\":\"Sun, 31 Jan 2021 11:12:04.541256\",\"timeScan\":1612087924725,\"timeScanText\":\"Sun, 31 Jan 2021 11:12:04.725259\",\"timeDetails\":1612087925355,\"timeDetailsText\":\"Sun, 31 Jan 2021 11:12:05.355257\",\"differencess\":{\"scan\":0.184,\"details\":0.181,\"total\":0.365}}";
    write(new_socket_fd,message,strlen(message));
    sleep(10);

    close(new_socket_fd);
    return NULL;
}

void signal_handler(int signal_number) {
    /* TODO: Put exit cleanup code here. */
    exit(0);
}