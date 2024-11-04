#include "../include/copy.h"
#include "../include/open.h"
#include "../include/server.h"
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

// Help functions for get input and output
static int       get_output(const struct options *opts, int *err);
static in_port_t convert_port(const char *str, int *err);

int main(int argc, char *argv[])
{
    // Initialize variables
    struct options opts;
    int            out_fd;
    int            err;
    ssize_t        result;
    char           buffer[BUFSIZ];

    // Assign values to these variables
    memset(&opts, 0, sizeof(opts));
    opts.inport          = PORT;
    opts.outport         = PORT;
    opts.conversion_type = NULL;

    // Get address and coversion type from argv
    parse_arguments(argc, argv, &opts);

    // Check arguments
    check_arguments(argv[0], &opts);

    // Get server descriptor
    err    = 0;
    out_fd = get_output(&opts, &err);

    // check if server descriptor has error
    if(out_fd < 0)
    {
        const char *msg;

        msg = strerror(err);
        printf("Error opening output: %s\n", msg);
        goto err_out;
    }

    // Combine conversion type and message save in the buffer
    printf("Message sent to server: %s|%s\n", opts.conversion_type, opts.message);
    snprintf(buffer, BUFSIZ, "%s|%s", opts.conversion_type, opts.message);

    // copy data from buffer and send to server
    result = nwrite(buffer, out_fd, BUFSIZ, &err);

    if(result == -1)
    {
        perror("Error write to server");
        close(out_fd);
        exit(EXIT_FAILURE);
    }

    // Receive converted data from server
    result = read(out_fd, buffer, BUFSIZ);

    if(result < 0)
    {
        perror("Error receive data from server");
        close(out_fd);
        exit(EXIT_FAILURE);
    }

    if(result > 0)
    {
        buffer[result] = '\0';
        printf("Message received from server: %s\n", buffer);
    }

    close(out_fd);

err_out:
    return EXIT_SUCCESS;
}

static void parse_arguments(int argc, char *argv[], struct options *opts)
{
    /*
    struct option saved all possible options of the server program
     */
    static struct option long_options[] = {
        {"inaddress",  required_argument, NULL, 'n'},
        {"outaddress", required_argument, NULL, 'N'},
        {"inport",     required_argument, NULL, 't'},
        {"outport",    required_argument, NULL, 'T'},
        {"convert",    required_argument, NULL, 'c'},
        {"help",       no_argument,       NULL, 'h'},
        {NULL,         0,                 NULL, 0  }
    };
    int opt;
    int err;

    opterr = 0;

    while((opt = getopt_long(argc, argv, "ha:p:m:c:", long_options, NULL)) != -1)
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
            case 'm':
            {
                opts->message = optarg;
                break;
            }
            case 'c':
            {
                opts->conversion_type = optarg;
                break;
            }
            case 'h':
            {
                usage(argv[0], EXIT_SUCCESS, NULL);
            }
            // If option is unknown
            case '?':
            {
                if(optopt == 'a' || optopt == 'p' || optopt == 'm' || optopt == 'c')
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
        usage(binary_name, EXIT_FAILURE, "an network address is required");
    }

    if(opts->conversion_type != NULL)
    {
        if(strcmp(opts->conversion_type, "upper") != 0 && strcmp(opts->conversion_type, "lower") != 0 && strcmp(opts->conversion_type, "none") != 0)
        {
            usage(binary_name, EXIT_FAILURE, "conversion type can only be upper, lower, or none");
        }
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
    fprintf(stderr, "Usage: %s [-a <address>] [-p <port>] [-m <message>] [-c <conversion>]\n", program_name);
    fputs("Options:\n", stderr);
    fputs("  -h, --help                           Display help message\n", stderr);
    fputs("  -a <address>, --inaddress <address>  Network socket <address>\n", stderr);
    fputs("  -p <port>, --inaddress <address>     Network socket (PORT) <address>\n", stderr);
    fputs("  -m, --message                        Message to convert\n", stderr);
    fputs("  -c, --conversion type 				  Conversion type (upper, lower, or none)\n", stderr);
    exit(exit_code);
}

static int get_output(const struct options *opts, int *err)
{
    int fd;

    if(opts->outaddress != NULL)
    {
        fd = open_network_socket_client(opts->outaddress, opts->outport, err);
    }
    else
    {
        fd = -1;
    }

    return fd;
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
