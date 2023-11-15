/*
 * DISCLAIMER:
 * This library is pretty much a partial copy of the RIO library provided 
 * by Computer Systems - A programmer's perspective by Bryant and O'Hallaron.
 * source: 
 * https://csapp.cs.cmu.edu/3e/ics3/code/src/csapp.c 
 * and http://csapp.cs.cmu.edu/2e/ics2/code/include/csapp.h
 *
*/

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/* Misc constants */
#define	MAXLINE	 8192  /* Max text line length */
#define MAXBUF   8192  /* Max I/O buffer size */
#define LISTENQ  1024  /* Second argument to listen() */

/* Persistent state for the robust I/O (Rio) package */
#define IO_BUFSIZE 8192
typedef struct {
    int io_assist_fd;                /* Descriptor for this internal buf */
    int io_assist_cnt;               /* Unread bytes in internal buf */
    char *io_assist_bufptr;          /* Next unread byte in internal buf */
    char io_assist_buf[IO_BUFSIZE]; /* Internal buffer */
} io_assist_state_t;

/* Robust read and writes */
ssize_t io_assist_readn(int fd, void *usrbuf, size_t n);
ssize_t io_assist_writen(int fd, void *usrbuf, size_t n);
void io_assist_readinitb(io_assist_state_t *rp, int fd); 
ssize_t	io_assist_readnb(io_assist_state_t *rp, void *usrbuf, size_t n);
ssize_t	io_assist_readlineb(io_assist_state_t *rp, void *usrbuf, size_t maxlen);

/* Reentrant protocol-independent client/server helpers */
int io_assist_open_clientfd(char *hostname, char *port);
int io_assist_open_listenfd(char *port);
