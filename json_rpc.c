#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "json_rpc.h"

extern int h_errno;

static char *http_header_templ = "POST %s HTTP/1.1\r\nHost: %s\r\nAccept: \
application/json; charset=UTF-8\r\nConnection: close\r\nContent-Type: \
application/json; charset=UTF-8\r\nContent-Length: %d\r\n\r\n";

static char *jsonrpc_templ = "{\"jsonrpc\": \"2.0\", \"method\": \"%s\", \
\"params\": %s, \"id\": %d}";

#define HTTP_HEADER_LEN (strlen(http_header_templ) - 6)
#define JSONRPC_TEMPL_LEN (strlen(jsonrpc_templ) - 6)

static int do_connect(const char *host, int port) {
  int fd = -1, err;
  struct sockaddr_in addr;
  int addr_len = sizeof(addr);
  struct hostent *he = NULL;

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    err = errno;
    printf("socket(): %s (%d)\n", strerror(err), err);
    return -1;
  }

  if ((he = gethostbyname(host)) == NULL) {
    printf("gethostbyname(): %d\n", h_errno);
    close(fd);
    return -1;
  }

  memset(&addr, 0, addr_len);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  memcpy(&addr.sin_addr.s_addr, he->h_addr_list[0], he->h_length);

  if (connect(fd, (struct sockaddr *)&addr, addr_len) != 0) {
    err = errno;
    printf("connect(): %s (%d)\n", strerror(err), err);
    close(fd);
    return -1;
  }

  return fd;
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

int json_rpc_request(const char *host, int port, const char *path,
                     const char *method, const char *params, int id,
                     char *resp, int resp_sz) {
  int fd, rc = 0;
  size_t len, jsonrpc_len, header_len;
  char *buf = NULL, *s = NULL;

  if (host == NULL || port <= 0 || path == NULL ||
      method == NULL || params == NULL ||
      resp == NULL || resp_sz <= 0) {
    printf("json_rpc_request(): invalid argument\n");
    return -1;
  }

  /* connect to the JSON-RPC service */
  if ((fd = do_connect(host, port)) == -1) return -1;

  /* calculate andcheck length of the request */
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
    close(fd);
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

  if ((rc = do_write(fd, buf)) != 0) goto _end;
  
  memset(resp, 0, resp_sz);
  
  rc = do_read(fd, resp, resp_sz);

_end:
  free(buf);
  free(s);
  close(fd);

  return rc;
}
