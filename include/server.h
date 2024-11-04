#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>

// Constants
#define BUFSIZE 128
#define PORT 9999
#define BACKLOG 5
#define TEST 10
#define MISSING_OPTION_MESSAGE_LEN 35
#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define ERR_NONE 0
#define ERR_NO_DIGITS 1
#define ERR_OUT_OF_RANGE 2
#define ERR_INVALID_CHARS 3

// Struct to store socket address
struct options
{
    char     *message;
    char     *inaddress;
    char     *outaddress;
    in_port_t inport;
    in_port_t outport;
    char     *conversion_type;
};

#endif    // SERVER_H
