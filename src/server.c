#include "../include/server.h"
#include "../include/copy.h"
#include "../include/open.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

// Functions dealing with arguments
static void           parse_arguments(int argc, char *argv[], struct options *opts);
static void           check_arguments(const char *binary_name, const struct options *opts);
_Noreturn static void usage(const char *program_name, int exit_code, const char *message);

// Help functions for get client and server
static int       get_server(const struct options *opts, int *err);
static in_port_t convert_port(const char *str, int *err);

int main(int argc, char *argv[])
{
    // Initialize variables
    struct options opts;
    int            server_fd;
    int            err;
    ssize_t        result;

    // Assign values to these variables
    memset(&opts, 0, sizeof(opts));
    opts.inport          = PORT;
    opts.outport         = PORT;
    opts.conversion_type = NULL;

    // Get address and coversion type from argv
    parse_arguments(argc, argv, &opts);

    // Check arguments
    check_arguments(argv[0], &opts);

    // get input file descriptor
    err       = 0;
    server_fd = get_server(&opts, &err);

    // check if input descriptor has error
    if(server_fd < 0)
    {
        const char *msg;

        msg = strerror(err);
        printf("Error initializing server: %s\n", msg);
        goto err_in;
    }
    printf("Server listening on %s | PORT: %d\n", opts.inaddress, opts.inport);

    while(true)
    {
        int   client_fd;
        pid_t pid;

        // Accept server to get client descriptor
        client_fd = accept(server_fd, NULL, 0);

        if(client_fd == -1)
        {
            perror("Failed to accept client connection");
            continue;
        }

        // Fork a new process to handle the client
        pid = fork();
        if(pid < 0)
        {
            perror("Fork failed");
            close(client_fd);
            continue;
        }

        if(pid == 0)
        {
            // In child process

            // Close server_fd in child process (not affect parent process's server_fd)
            close(server_fd);

            // Set up network socket and get output file descriptor
            result = convert_copy(client_fd, BUFSIZE, &err);

            if(result == -1)
            {
                perror("Memory allocation error");
            }
            else if(result == -2)
            {
                perror("Read error");
            }
            else if(result == -3)
            {
                perror("Write error");
            }
            close(client_fd);    // Close client socket
            exit(0);             // Terminate child process
        }
        // In the parent process
        close(client_fd);
    }

err_in:
    close(server_fd);
    return EXIT_SUCCESS;
}

static void parse_arguments(int argc, char *argv[], struct options *opts)
{
    /*
    struct option saved all possible options of the server program
     */
    static struct option long_options[] = {
        {"address", required_argument, NULL, 'a'},
        {"port",    required_argument, NULL, 'p'},
        {"help",    no_argument,       NULL, 'h'},
        {NULL,      0,                 NULL, 0  }
    };
    int opt;
    int err;

    opterr = 0;

    while((opt = getopt_long(argc, argv, "ha:p:", long_options, NULL)) != -1)
    {
        switch(opt)
        {
            case 'a':
            {
                opts->inaddress  = optarg;
                opts->outaddress = optarg;
                break;
            }
            case 'p':
            {
                opts->inport  = convert_port(optarg, &err);
                opts->outport = convert_port(optarg, &err);
                if(err != ERR_NONE)
                {
                    usage(argv[0], EXIT_FAILURE, "inport most be between 0 and 65535");
                }
                break;
            }
            case 'h':
            {
                usage(argv[0], EXIT_SUCCESS, NULL);
            }
            // If option is unknown
            case '?':
            {
                if(optopt == 'a' || optopt == 'p')
                {
                    char message[MISSING_OPTION_MESSAGE_LEN];

                    snprintf(message, sizeof(message), "Option '-%c' requires an address.", optopt);
                    usage(argv[0], EXIT_FAILURE, message);
                }
                else
                {
                    char message[UNKNOWN_OPTION_MESSAGE_LEN];

                    snprintf(message, sizeof(message), "Unknown option '-%c'.", optopt);
                    usage(argv[0], EXIT_FAILURE, message);
                }
            }
            default:
            {
                usage(argv[0], EXIT_FAILURE, NULL);
            }
        }
    }
}

static void check_arguments(const char *binary_name, const struct options *opts)
{
    if(!opts->inaddress || !opts->outaddress)
    {
        usage(binary_name, EXIT_FAILURE, "An address is required");
    }
}

_Noreturn static void usage(const char *program_name, int exit_code, const char *message)
{
    // Print Error message
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }

    // Print the Usage message
    fprintf(stderr, "Usage: %s [-h] [-a <address>] [-p <port>] \n", program_name);
    fputs("Options:\n", stderr);
    fputs("  -h, --help                           Display this help message\n", stderr);
    fputs("  -a <address>, --address <address>    Network socket <address>\n", stderr);
    fputs("  -p <port>, --address <address>       Network socket (PORT) <address>\n", stderr);
    exit(exit_code);
}

static int get_server(const struct options *opts, int *err)
{
    int server_fd;

    if(opts->inaddress != NULL)
    {
        server_fd = listen_network_socket_client(opts->inaddress, opts->inport, BACKLOG, err);
    }
    else
    {
        server_fd = -1;
    }

    return server_fd;
}

static in_port_t convert_port(const char *str, int *err)
{
    in_port_t port;
    char     *endptr;
    long      val;

    *err  = ERR_NONE;
    port  = 0;
    errno = 0;
    val   = strtol(str, &endptr, 10);    // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

    // Check if no digits were found
    if(endptr == str)
    {
        *err = ERR_NO_DIGITS;
        goto done;
    }

    // Check for out-of-range errors
    if(val < 0 || val > UINT16_MAX)
    {
        *err = ERR_OUT_OF_RANGE;
        goto done;
    }

    // Check for trailing invalid characters
    if(*endptr != '\0')
    {
        *err = ERR_INVALID_CHARS;
        goto done;
    }

    port = (in_port_t)val;

done:
    return port;
}
