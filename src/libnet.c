#define _XOPEN_SOURCE 700

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "libnet.h"
#include "vm.h"

void lat_peticion(lat_vm* vm)
{
    lat_objeto* o = lat_desapilar(vm);
    if (o->type == T_STR || o->type == T_LIT)
    {
        char buffer[BUFSIZ];
        enum CONSTEXPR { MAX_REQUEST_LEN = 1024};
        char request[MAX_REQUEST_LEN];
        char request_template[] = "Get / HTTP/1.1\r\nHost: %s\r\n\r\n\r\n";
        struct protoent *protoent;
        char *hostname = "example.com";
        in_addr_t in_addr;
        int request_len;
        int socket_file_descriptor;
        ssize_t nbytes_total, nbytes_last;
        struct hostent *hostent;
        struct sockaddr_in sockaddr_in;
        unsigned short server_port = 80;

        hostname = lat_obtener_cadena(o);

        request_len = snprintf(request, MAX_REQUEST_LEN, request_template, hostname);
        if (request_len >= MAX_REQUEST_LEN)
        {
            fprintf(stderr, "request length large: %d\n", request_len);
            exit(EXIT_FAILURE);
        }

        /* Build the socket. */
        protoent = getprotobyname("tcp");
        if (protoent == NULL)
        {
            perror("getprotobyname");
            exit(EXIT_FAILURE);
        }
        socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
        if (socket_file_descriptor == -1)
        {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        /* Build the address. */
        hostent = gethostbyname(hostname);
        if (hostent == NULL)
        {
            fprintf(stderr, "error: el host solicitado no existe(\"%s\")\n", hostname);
            exit(EXIT_FAILURE);
        }
        in_addr = inet_addr(inet_ntoa(*(struct in_addr*) * (hostent->h_addr_list)));
        if (in_addr == (in_addr_t) - 1)
        {
            fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
            exit(EXIT_FAILURE);
        }
        sockaddr_in.sin_addr.s_addr = in_addr;
        sockaddr_in.sin_family = AF_INET;
        sockaddr_in.sin_port = htons(server_port);

        /* Actually connect. */
        if (connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1)
        {
            perror("connect");
            exit(EXIT_FAILURE);
        }

        /* Send HTTP request. */
        nbytes_total = 0;
        while (nbytes_total < request_len)
        {
            nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
            if (nbytes_last == -1)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
            nbytes_total += nbytes_last;
        }

        while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0)
        {
            write(STDOUT_FILENO, buffer, nbytes_total);
        }
        if (nbytes_total == -1)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

        close(socket_file_descriptor);
        exit(EXIT_SUCCESS);

    }
    else
    {
        lat_registrar_error("se requiere una cadena como parametro\n");
    }
}

