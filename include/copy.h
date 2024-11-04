#ifndef COPY_H
#define COPY_H

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

ssize_t convert_copy(int fd, size_t size, int *err);
ssize_t nwrite(const char *buffer, int fd, size_t size, int *err);

#endif    // COPY_H
