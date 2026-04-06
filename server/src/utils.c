#define _POSIX_C_SOURCE 200112L

#include "utils.h"
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

t_log* logger;

int iniciar_servidor(void)
{
    int socket_servidor;

    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // getaddrinfo
    if (getaddrinfo(NULL, PUERTO, &hints, &servinfo) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // socket
    socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socket_servidor == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // reutilizar puerto
    int yes = 1;
    if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // bind
    if (bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("bind");
        close(socket_servidor);
        exit(EXIT_FAILURE);
    }

    // listen
    if (listen(socket_servidor, SOMAXCONN) == -1) {
        perror("listen");
        close(socket_servidor);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(servinfo);

    log_trace(logger, "Listo para escuchar a mi cliente");

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
    int socket_cliente = accept(socket_servidor, NULL, NULL);

    if (socket_cliente == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    log_info(logger, "Se conecto un cliente!");

    return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
    int cod_op;

    if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
        return cod_op;
    else
    {
        close(socket_cliente);
        return -1;
    }
}

void* recibir_buffer(int* size, int socket_cliente)
{
    void* buffer;

    if (recv(socket_cliente, size, sizeof(int), MSG_WAITALL) <= 0)
        return NULL;

    buffer = malloc(*size);
    if (buffer == NULL)
        return NULL;

    if (recv(socket_cliente, buffer, *size, MSG_WAITALL) <= 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

void recibir_mensaje(int socket_cliente)
{
    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);

    if (buffer == NULL)
        return;

    log_info(logger, "Me llego el mensaje %s", buffer);

    free(buffer);
}

t_list* recibir_paquete(int socket_cliente)
{
    int size;
    int desplazamiento = 0;
    void* buffer;
    t_list* valores = list_create();
    int tamanio;

    buffer = recibir_buffer(&size, socket_cliente);
    if (buffer == NULL)
        return valores;

    while (desplazamiento < size)
    {
        memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
        desplazamiento += sizeof(int);

        char* valor = malloc(tamanio);
        memcpy(valor, buffer + desplazamiento, tamanio);
        desplazamiento += tamanio;

        list_add(valores, valor);
    }

    free(buffer);
    return valores;
}