#include "../include/open.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static void setup_network_address(struct sockaddr_storage *addr, socklen_t *addr_len, const char *address, in_port_t port, int *err);
static int  connect_to_server(struct sockaddr_storage *addr, socklen_t addr_len, int *err);
static int  accept_connection(const struct sockaddr_storage *addr, socklen_t addr_len, int backlog, int *err);
static int  listen_connection(const struct sockaddr_storage *addr, socklen_t addr_len, int backlog, int *err);

int open_keyboard(void)
{
    return STDIN_FILENO;
}

int open_stdout(void)
{
    return STDOUT_FILENO;
}

int open_network_socket_client(const char *address, in_port_t port, int *err)
{
    struct sockaddr_storage addr;
    socklen_t               addr_len;
    int                     fd;

    setup_network_address(&addr, &addr_len, address, port, err);

    if(*err != 0)
    {
        fd = -1;
        goto done;
    }

    fd = connect_to_server(&addr, addr_len, err);

done:
    return fd;
}

int listen_network_socket_client(const char *address, in_port_t port, int backlog, int *err)
{
    struct sockaddr_storage addr;
    socklen_t               addr_len;
    int                     server_fd;

    setup_network_address(&addr, &addr_len, address, port, err);

    if(*err != 0)
    {
        server_fd = -1;
        goto done;
    }

    server_fd = listen_connection(&addr, addr_len, backlog, err);

done:
    return server_fd;
}

int open_network_socket_server(const char *address, in_port_t port, int backlog, int *err)
{
    struct sockaddr_storage addr;
    socklen_t               addr_len;
    int                     server_fd;

    setup_network_address(&addr, &addr_len, address, port, err);

    if(*err != 0)
    {
        server_fd = -1;
        goto done;
    }

    server_fd = accept_connection(&addr, addr_len, backlog, err);

done:
    return server_fd;
}

static void setup_network_address(struct sockaddr_storage *addr, socklen_t *addr_len, const char *address, in_port_t port, int *err)
{
    in_port_t net_port;

    *addr_len = 0;
    net_port  = htons(port);
    memset(addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr->ss_family     = AF_INET;
        ipv4_addr->sin_port = net_port;
        *addr_len           = sizeof(struct sockaddr_in);
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr->ss_family      = AF_INET6;
        ipv6_addr->sin6_port = net_port;
        *addr_len            = sizeof(struct sockaddr_in6);
    }
    else
    {
        fprintf(stderr, "%s is not an IPv4 or an IPv6 address\n", address);
        *err = errno;
    }
}

static int listen_connection(const struct sockaddr_storage *addr, socklen_t addr_len, int backlog, int *err)
{
    int server_fd;
    int result;

    server_fd = socket(addr->ss_family, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)

    if(server_fd == -1)
    {
        *err = errno;
        goto done;
    }

    result = bind(server_fd, (const struct sockaddr *)addr, addr_len);

    if(result == -1)
    {
        *err = errno;
        goto done;
    }

    result = listen(server_fd, backlog);

    if(result == -1)
    {
        *err = errno;
        goto done;
    }

done:
    return server_fd;
}

static int accept_connection(const struct sockaddr_storage *addr, socklen_t addr_len, int backlog, int *err)
{
    int server_fd;
    int result;
    int client_fd;

    server_fd = socket(addr->ss_family, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)

    if(server_fd == -1)
    {
        client_fd = -1;
        *err      = errno;
        goto done;
    }

    result = bind(server_fd, (const struct sockaddr *)addr, addr_len);

    if(result == -1)
    {
        client_fd = -1;
        *err      = errno;
        goto done;
    }

    result = listen(server_fd, backlog);

    if(result == -1)
    {
        client_fd = -1;
        *err      = errno;
        goto done;
    }

    client_fd = accept(server_fd, NULL, 0);

    if(client_fd == -1)
    {
        *err = errno;
    }

done:
    close(server_fd);

    return client_fd;
}

static int connect_to_server(struct sockaddr_storage *addr, socklen_t addr_len, int *err)
{
    int fd;
    int result;

    fd = socket(addr->ss_family, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)

    if(fd == -1)
    {
        *err = errno;
        goto done;
    }

    result = connect(fd, (const struct sockaddr *)addr, addr_len);

    if(result == -1)
    {
        *err = errno;
        close(fd);
        fd = -1;
    }

done:
    return fd;
}
