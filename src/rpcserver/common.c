#include <execinfo.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#include "common.h"

#ifdef __APPLE__
#include <os/log.h>

struct os_log_s {
    int a;
};

struct os_log_s _os_log_default;
#endif// __APPLE__

bool g_stdout = false;
bool g_syslog = false;
FILE *g_file = NULL;

#define BT_BUF_SIZE (100)

void print_backtrace() {
    int nptrs;
    void *buffer[BT_BUF_SIZE];
    char **strings;

    nptrs = backtrace(buffer, BT_BUF_SIZE);
    trace("BACKTRACE", "backtrace() returned %d addresses", nptrs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
        would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        return;
    }

    for (int j = 0; j < nptrs; j++) {
        trace("BACKTRACE:\t", "%s", strings[j]);
    }

    free(strings);
}

void trace(const char *prefix, const char *fmt, ...) {
    if (!g_stdout && !g_syslog) {
        return;
    }

    char line[1022];
    char prefixed_line[1024];

    va_list args;
    va_start(args, fmt);
    vsprintf(line, fmt, args);
    va_end(args);

    sprintf(prefixed_line, "%s: %s", prefix, line);

    if (g_stdout) {
        puts(prefixed_line);
        fflush(stdout);
    }
    if (g_syslog) {
#ifdef __APPLE__
        os_log(&_os_log_default, "%{public}s", prefixed_line);
#else // __APPLE__
        syslog(LOG_DEBUG, "%s", prefixed_line);
#endif// !__APPLE__
    }
    if (g_file) {
        fprintf(g_file, "%s\n", prefixed_line);
        fflush(g_file);
    }
}

bool recvall(int sockfd, char *buf, size_t len) {
    bool ret = false;
    size_t total_bytes = 0;
    size_t bytes = 0;
    *disconnected = false;

    while (len > 0) {
        bytes = recv(sockfd, buf + total_bytes, len, 0);
        if (0 == bytes)
        {
            TRACE("client fd: %d disconnected", sockfd);
            *disconnected = true;
            return false;
        }
        CHECK(bytes > 0);
        total_bytes += bytes;
        len -= bytes;
    }

    return true;

error:
    return false;
}

bool recvall(int sockfd, char *buf, size_t len)
{
    bool disconnected;
    return recvall_ext(sockfd, buf, len, &disconnected);
}

bool sendall(int sockfd, const char *buf, size_t len)
{
    size_t total_bytes = 0;
    size_t bytes = 0;

    while (len > 0)
    {
        bytes = send(sockfd, buf + total_bytes, len, MSG_NOSIGNAL);
        CHECK(bytes != -1);

        total_bytes += bytes;
        len -= bytes;
    }

    return true;

error:
    return false;
}

bool writeall(int fd, const char *buf, size_t len) {
    size_t total_bytes = 0;
    size_t bytes = 0;

    while (len > 0) {
        bytes = write(fd, buf + total_bytes, len);
        CHECK(bytes != -1);

        total_bytes += bytes;
        len -= bytes;
    }

    return true;

error:
    return false;
}