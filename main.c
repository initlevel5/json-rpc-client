/*
 * Copyright (c) 2020 initlevel5
 *
 * This is a simple REST API client that
 * provides access to JSON-RPC services.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "json_rpc.h"

#define BUF_SIZE (1024)
#define HOST_SIZE (64)
#define PATH_SIZE (32)
#define METHOD_SIZE (32)
#define PARAM_SIZE (64)

#define STR_CPY(dst, src) do {                                                 \
  strncpy((dst), (src), sizeof((dst)) - 1);                                    \
  (dst)[sizeof((dst)) - 1] = '\0';                                             \
} while (0)

int main(int argc, char const *argv[]) {
  int id, opt, mfound = 0, port;
  char host[HOST_SIZE], path[PATH_SIZE],
       method[METHOD_SIZE], params[PARAM_SIZE],
       *resp = NULL;
  
  /* set defaults */
  STR_CPY(host, "localhost");
  port = 80;
  STR_CPY(path, "/");
  memset(method, 0, sizeof(method));
  STR_CPY(params, "[]");
  id = 0;
  
  while ((opt = getopt(argc, (char *const *)argv, "h:p:u:m:a:i:")) != -1) {
    switch (opt) {
      case 'h': STR_CPY(host, optarg); break;
      case 'p': port = atoi(optarg); break;
      case 'u': STR_CPY(path, optarg); break;
      case 'm': STR_CPY(method, optarg); mfound = 1; break;
      case 'a': STR_CPY(params, optarg); break;
      case 'i': id = atoi(optarg); break;
      default:
        fprintf(stderr, "Usage: %s [-h] hostname [-p] port [-u] uri [-m] method\
 [-a] arguments [-i] id\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (mfound == 0) {
    fprintf(stderr, "method name required");
    exit(EXIT_FAILURE);
  }

  if ((resp = malloc(BUF_SIZE)) == NULL) exit(EXIT_FAILURE);

  if (json_rpc_request(host, port, path,
                       method, params, id,
                       resp, BUF_SIZE) != 0) {
    fprintf(stderr, "json_rpc_request() failed\n");
    exit(EXIT_FAILURE);
  }

  printf("response:\n%s\n", resp);

  free(resp);

  (void)argc;
  (void)argv;

  return EXIT_SUCCESS;
}
