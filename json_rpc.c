/*
 * Copyright (c) 2020 initlevel5
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "json_rpc.h"

#define ST_HOST 0
#define ST_PORT 1
#define ST_PATH 2

#define INVALID_SOCKET (-1)

#define FD_CLOSE(x) do {                                                      \
    while (close((x)) == -1 && errno == EINTR);                               \
    (x) = INVALID_SOCKET;                                                     \
} while (0)

static char http_header_templ[] = "POST %s HTTP/1.1\r\nHost: %s\r\nAccept: \
application/json\r\nConnection: close\r\nContent-Type: application/json\r\n\
Content-Length: %d\r\n\r\n";

static char jsonrpc_templ[] = "{\"jsonrpc\": \"2.0\", \"method\": \"%s\", \
\"params\": %s, \"id\": %d}";

#define HTTP_HEADER_LEN (strlen(http_header_templ) - 6)
#define JSONRPC_TEMPL_LEN (strlen(jsonrpc_templ) - 6)

static int do_connect(const char *host, unsigned int port, int timeout) {
  fd_set wfds;
  int fd = INVALID_SOCKET, err, flags, n, err_len = sizeof(err);
  struct sockaddr_in addr;
  int addr_len = sizeof(addr);
  struct hostent *he = NULL;
  struct timeval tv = {timeout, 0};

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    err = errno;
    printf("socket(): %s (%d)\n", strerror(err), err);
    return -1;
  }

  if ((he = gethostbyname(host)) == NULL) {
    printf("gethostbyname(): %d\n", h_errno);
    goto _err;
  }

  memset(&addr, 0, addr_len);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  memcpy(&addr.sin_addr.s_addr, he->h_addr_list[0], he->h_length);

  if ((flags = fcntl(fd, F_GETFL, 0)) == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    err = errno;
    printf("make nonblocking: %s (%d)", strerror(err), err);
    goto _err;
  }

  if (connect(fd, (struct sockaddr *)&addr, addr_len) != 0) {
    err = errno;
    if (err != EINPROGRESS) {
      printf("connect(): %s (%d)\n", strerror(err), err);
      goto _err;
    }

    /* connection now in progress */

    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);

    for (;;) {
      /* TODO if inerrupted with EINTR start timeout at time has already elapsed */

      n = select(fd + 1, NULL, &wfds, NULL, &tv);

      if (n == -1) {
        err = errno;
        if (err != EINTR) {
          printf("select(): %s (%d)\n", strerror(err), err);
          goto _err;
        }
        continue;
      }

      if (n == 1) {
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t *)&err_len) != 0) {
          printf("getsockopt(): %s (%d)\n", strerror(err), err);
          goto _err;
        }
        if (err) {
          printf("connect(): %s (%d)\n", strerror(err), err);
          goto _err;
        }
        break;
      }

      printf("connect(): timeout\n");
      goto _err;
    }
  }

  if ((flags = fcntl(fd, F_GETFL, 0)) == -1 || fcntl(fd, F_SETFL, flags &= (~O_NONBLOCK)) == -1) {
    err = errno;
    printf("make blocking: %s (%d)", strerror(err), err);
    goto _err;
  }

  return fd;

_err:
  FD_CLOSE(fd);
  return -1;
}

static int do_write(int fd, char *buf) {
  int err, n, to_write = strlen(buf), written = 0;

  do {
    if ((n = write(fd, buf + written, to_write - written)) < 1) {
      if (n == -1) {
        err = errno;
        printf("write(): %s (%d)\n", strerror(err), err);
      } else {
        printf("write(): connection closed by peer\n");
      }
      return -1;
    }

    written += n;

  } while (written < to_write);

  return 0;
}

static int do_read(int fd, char *buf, int buf_sz) {
  int err = 0, n, to_read = buf_sz - 1, avail = 0;

  do {
    if ((n = read(fd, buf + avail, to_read - avail)) < 1) {
      if (n == -1) {
        err = errno;
        printf("read(): %s (%d)\n", strerror(err), err);
      }
      break;
    }

    avail += n;

    /* TODO realloc buf if needed */

  } while (avail < to_read);

  return err != 0 ? -1 : 0;
}

/*
 * Address format: [PROTO://]HOST[:PORT][PATH]
 *
 * PROTO should be 'http' only (if defined).
 * HOST could be IPv4 address or a host name.
 * Default values for PORT and PATH are 80 and '/', respectively.
 *
 * Returns -1 if parse error occured, otherwise 0.
 */
static int parse_address(const char *addr,
                         char **host, unsigned int *port, char **path) {
  char *p = NULL, *phost = NULL, *pport = NULL, *ppath = NULL, prev = '/';
  int state = ST_HOST;
  size_t len;

  if (addr == NULL || host == NULL || port == NULL || path == NULL) return -1;

  if ((p = strstr(addr, "://")) != NULL) {
    if ((len = p - addr) != 4 || strncmp(addr, "http", len)) return -1;
    p += 3;
  } else p = (char *)addr;

  if (*p == '\0') return -1;

  phost = p;

  for (;;) {
    /* remove whitespaces */
    if (isspace((int)*p)) {
      char *dst = p, *src = p + 1;
      while((*dst++ = *src++));
      p--;
    } else {
      switch (state) {
        case ST_HOST:
          if (*p == ':' || *p == '/') {
            if (p == phost) return -1;
            if (*p == ':') {
              pport = p + 1;
              state = ST_PORT;
            } else if (*p == '/') {
              ppath = p + 1;
              state = ST_PATH;
            }
            *p = '\0';
          } else if (!isalnum((int)*p) && *p != '-' && *p != '.') return -1;
          break;
        case ST_PORT:
          if (!isdigit((int)*p)) {
            if (*p == '/') {
              *p = '\0';
              ppath = p + 1;
              state = ST_PATH;
              break;
            }
            return -1;
          }
          break;
        case ST_PATH:
          if ((!isalpha((int)*p) && !isdigit((int)*p) && *p != '/') ||
              (*p == '/' && prev == '/')) return -1;
          prev = *p;
          break;
      }
    }

    if (!*(p + 1)) break;

    p++;
  }

  if (pport) {
    if (!isdigit((int)*pport) || (*port = atoi(pport)) >= 0xffffUL) return -1;
  } else {
    *port = 80;
  }

  len = strlen(phost);
  if ((*host = malloc(len + 1)) == NULL) return -1;
  memcpy(*host, phost, len);
  (*host)[len] = '\0';

  len = ppath ? strlen(ppath) : 0;
  if ((*path = malloc(len + 2)) == NULL) {
    free(*host);
    *host = NULL;
    return -1;
  }
  **path = '/';
  memcpy(*path + 1, ppath, len);
  (*path)[len + 1] = '\0';

#if 0
  printf("%s\n%u\n%s\n", *host, *port, *path);
  return -1;
#endif

  return 0;
}

int json_rpc_request(const char *addr,
                     const char *method, const char *params, int id,
                     char *resp, int resp_sz, int timeout) {
  int fd = -1, rc = 0;
  unsigned int port = 0;
  size_t len, jsonrpc_len, header_len;
  char *host = NULL, *path = NULL, *buf = NULL, *s = NULL;

  if (addr == NULL ||
      method == NULL || params == NULL ||
      resp == NULL || resp_sz <= 0) {
    printf("json_rpc_request(): invalid argument\n");
    return -1;
  }

  /* parse the address string */
  if (parse_address(addr, &host, &port, &path) != 0) {
    printf("json_rpc_request(): can't parse address\n");
    rc = -1;
    goto _end;
  }

  /* connect to the JSON-RPC service */
  if ((fd = do_connect(host, port, timeout)) == -1) return -1;

  /* calculate and check length of the request */
  jsonrpc_len = JSONRPC_TEMPL_LEN +
                strlen(method) + strlen(params) +
                snprintf(NULL, 0, "%d", id);

  header_len = HTTP_HEADER_LEN +
               strlen(path) +
               strlen(host) +
               snprintf(NULL, 0, "%lu", jsonrpc_len);

  len = header_len + jsonrpc_len;

  /* allocate space for the request buffer */
  if ((buf = calloc(len + 1, sizeof(char))) == NULL) {
    FD_CLOSE(fd);
    return -1;
  }

  /* build HTTP header */
  snprintf(buf, header_len + 1, http_header_templ, path, host, jsonrpc_len);

  /* build JSON-RPC request */
  if ((s = malloc(jsonrpc_len + 1)) == NULL) {
    rc = -1;
    goto _end;
  }
  snprintf(s, jsonrpc_len + 1, jsonrpc_templ, method, params, id);
  strcat(buf + header_len, s);

#if 1
  printf("request:\r\n%s\r\n\r\n", buf);
#endif

  /* send request */
  if ((rc = do_write(fd, buf)) != 0) goto _end;
  
  memset(resp, 0, resp_sz);
  
  /* get response */
  rc = do_read(fd, resp, resp_sz);

_end:
  free(host);
  free(path);
  free(buf);
  free(s);
  FD_CLOSE(fd);

  return rc;
}
