#include "../include/copy.h"
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void convert_case(char *message, const char *conversion_type)
{
    if(conversion_type == NULL || strcmp(conversion_type, "none") == 0)
    {
        return;
    }

    if(strcmp(conversion_type, "upper") == 0)
    {
        for(size_t i = 0; message[i]; i++)
        {
            message[i] = (char)toupper((unsigned char)message[i]);
        }
    }
    else if(strcmp(conversion_type, "lower") == 0)
    {
        for(size_t i = 0; message[i]; i++)
        {
            message[i] = (char)tolower((unsigned char)message[i]);
        }
    }
}

ssize_t convert_copy(int fd, size_t size, int *err)
{
    uint8_t    *buf;
    ssize_t     retval;
    ssize_t     nread;
    ssize_t     nwrote;
    const char *conversion_type;
    char       *message;
    char       *save;
    size_t      message_len;

    *err  = 0;
    errno = 0;
    buf   = (uint8_t *)malloc(size);

    if(buf == NULL)
    {
        *err   = errno;
        retval = -1;
        goto done;
    }

    // Read from the file descriptor
    nread = read(fd, buf, size - 1);    // Leave space for null terminator
    if(nread < 0)
    {
        *err   = errno;
        retval = -2;
        goto cleanup;
    }
    buf[nread] = '\0';    // Null-terminate the read data

    // Parse conversion type and message
    conversion_type = strtok_r((char *)buf, "|", &save);
    message         = strtok_r(NULL, "|", &save);

    if(!conversion_type || !message)
    {
        fprintf(stderr, "Invalid format. Expected format: <conversion>|<message>\n");
        retval = -3;
        goto cleanup;
    }
    printf("Message received from client: %s\n", message);

    // Perform the conversion
    convert_case(message, conversion_type);

    // Write the converted message back to fd
    nwrote      = 0;
    message_len = strlen(message);
    while(nwrote < (ssize_t)message_len)
    {
        ssize_t twrote = write(fd, message + nwrote, (message_len - (size_t)nwrote));
        if(twrote < 0)
        {
            *err   = errno;
            retval = -4;
            goto cleanup;
        }
        nwrote += twrote;
    }

    retval = nwrote;

cleanup:
    free(buf);

done:
    return retval;
}

ssize_t nwrite(const char *buffer, int fd, size_t size, int *err)
{
    ssize_t nwrote;
    ssize_t nread;

    nread  = (ssize_t)size;
    nwrote = 0;
    while(nwrote != nread)
    {
        ssize_t written;

        written = write(fd, buffer + nwrote, (size_t)(nread - nwrote));
        if(written < 0)
        {
            *err   = errno;
            nwrote = -1;
            goto done;
        }
        nwrote += written;
    }

done:
    return nwrote;
}
