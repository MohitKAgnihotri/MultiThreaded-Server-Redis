//
// Created by 310165137 on 09/02/2021.
//
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include "client.h"
#include "cJSON.h"


#define SERVER_NAME_LEN_MAX 255

void PrintTime(void);

int main(int argc, char *argv[])
{
    char server_name[SERVER_NAME_LEN_MAX + 1] = { 0 };
    int server_port, socket_fd;
    struct hostent *server_host;
    struct sockaddr_in server_address;

    /* Get server name from command line arguments or stdin. */
    if (argc > 1)
    {
        strncpy(server_name, argv[1], SERVER_NAME_LEN_MAX);
    }
    else
    {
        printf("Enter Server Name: ");
        scanf("%s", server_name);
    }

    /* Get server port from command line arguments or stdin. */
    server_port = argc > 2 ? atoi(argv[2]) : 0;
    if (!server_port)
    {
        printf("Enter Port: ");
        scanf("%d", &server_port);
    }

    /* Get server host from server name. */
    server_host = gethostbyname(server_name);

    /* Initialise IPv4 server address with server host. */
    memset(&server_address, 0, sizeof server_address);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    memcpy(&server_address.sin_addr.s_addr, server_host->h_addr, server_host->h_length);

    /* Create TCP socket. */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    /* Connect to socket with server address. */
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof server_address) == -1) {
        perror("connect");
        exit(1);
    }

    /* TODO: Put server interaction code here. For example, use
     * write(socket_fd,,) and read(socket_fd,,) to send and receive messages
     * with the client.
     */

    write(socket_fd, "YjQ0ZjQyOTJiNjUzMjM0ZQ==", strlen("YjQ0ZjQyOTJiNjUzMjM0ZQ=="));

    char message_received[3000];
    while(1) {
        memset(message_received, 0x00, sizeof(message_received));
        read(socket_fd, message_received, sizeof(message_received));

        printf("%s\n", message_received);
        cJSON *json = cJSON_ParseWithLength(message_received, sizeof message_received);
        char *string = cJSON_Print(json);
        printf("%s\n",string);

        PrintTime();

    }

    close(socket_fd);
    return 0;
}

void PrintTime(void)
{
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    puts(buffer);

}
