#pragma once

#include <iostream>
#include <sys/stat.h>
#include <sys/socket.h>
#include <math.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>

#define	MAXLINE	4096			/* max line length */
#define SERV_PORT 8888
#define	SA	struct sockaddr

void
/* $f err_sys $ */
err_sys(const char *fmt, ...);
static void
err_doit(int errnoflag, const char *fmt, va_list ap);
int
Socket(int family, int type, int protocol);
void
Bind(int fd, const struct sockaddr *sa, socklen_t salen);
ssize_t
Recvfrom(int fd, void *ptr, size_t nbytes, int flags,
struct sockaddr *sa, socklen_t *salenptr);
void
Sendto(int fd, const void *ptr, size_t nbytes, int flags,
const struct sockaddr *sa, socklen_t salen);
void
Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);
void
Setnonblocking(int sockfd);