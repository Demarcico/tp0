#define _POSIX_C_SOURCE 200112L

#include "utils.h"
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void* serializar_paquete(t_paquete* paquete, int bytes)
{
    void* magic = malloc(bytes);
    int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
    desplazamiento += paquete->buffer->size;

    return magic;
}

int crear_conexion(char *ip, char* puerto)
{
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // getaddrinfo
    if (getaddrinfo(ip, puerto, &hints, &server_info) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // socket
    int socket_cliente = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);

    if (socket_cliente == -1) {
        perror("socket");
        freeaddrinfo(server_info);
        exit(EXIT_FAILURE);
    }

    // connect
    if (connect(socket_cliente,
                server_info->ai_addr,
                server_info->ai_addrlen) == -1) {
        perror("connect");
        close(socket_cliente);
        freeaddrinfo(server_info);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(server_info);

    return socket_cliente;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = MENSAJE;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->size);

    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

    int bytes = paquete->buffer->size + 2 * sizeof(int);

    void* a_enviar = serializar_paquete(paquete, bytes);

    if (send(socket_cliente, a_enviar, bytes, 0) == -1) {
        perror("send");
    }

    free(a_enviar);
    eliminar_paquete(paquete);
}

void crear_buffer(t_paquete* paquete)
{
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(void)
{
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = PAQUETE;
    crear_buffer(paquete);
    return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
    paquete->buffer->stream = realloc(
        paquete->buffer->stream,
        paquete->buffer->size + tamanio + sizeof(int)
    );

    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
    memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

    paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
    int bytes = paquete->buffer->size + 2 * sizeof(int);
    void* a_enviar = serializar_paquete(paquete, bytes);

    if (send(socket_cliente, a_enviar, bytes, 0) == -1) {
        perror("send");
    }

    free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void liberar_conexion(int socket_cliente)
{
    close(socket_cliente);
}