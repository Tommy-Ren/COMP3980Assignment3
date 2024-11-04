#ifndef OPEN_H
#define OPEN_H

#include <arpa/inet.h>

int open_keyboard(void);
int open_stdout(void);
int open_network_socket_client(const char *address, in_port_t port, int *err);
int listen_network_socket_client(const char *address, in_port_t port, int backlog, int *err);
int open_network_socket_server(const char *address, in_port_t port, int backlog, int *err);

#endif    // OPEN_H
