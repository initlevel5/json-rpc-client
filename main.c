/*
 * Copyright (c) 2020 initlevel5
 *
 * This is a simple REST API client that provides access to JSON-RPC services.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "json_rpc.h"

#define BUF_SIZE (1024)
#define ADDR_SIZE (256)
#define METHOD_SIZE (32)
#define PARAM_SIZE (64)

#define STR_CPY(dst, src) do {                                                 \
  strncpy((dst), (src), sizeof((dst)) - 1);                                    \
  (dst)[sizeof((dst)) - 1] = '\0';                                             \
} while (0)

int main(int argc, char const *argv[]) {
  int id, opt;
  char addr[ADDR_SIZE], method[METHOD_SIZE], params[PARAM_SIZE], *resp = NULL;
  
  /* set defaults */
  STR_CPY(addr, "http://localhost/");
  *method = '\0';
  STR_CPY(params, "[]");
  id = 0;
  
  /* get options */
  while ((opt = getopt(argc, (char *const *)argv, "a:m:p:i:")) != -1) {
    switch (opt) {
      case 'a': STR_CPY(addr, optarg); break;
      case 'm': STR_CPY(method, optarg); break;
      case 'p': STR_CPY(params, optarg); break;
      case 'i': id = atoi(optarg); break;
      default:
        fprintf(stderr, "Usage: %s [-a] address [-m] method\
 [-p] params [-i] id\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }
  if (strlen(method) == 0) {
    fprintf(stderr, "method name required");
    exit(EXIT_FAILURE);
  }

  /* allocate buffer for response */
  if ((resp = malloc(BUF_SIZE)) == NULL) exit(EXIT_FAILURE);

  /* Send a request to the JSON-RPC server */ 
  if (json_rpc_request(addr,
                       method, params, id,
                       resp, BUF_SIZE) != 0) {
    fprintf(stderr, "json_rpc_request() failed\n");
    free(resp);
    exit(EXIT_FAILURE);
  }

  printf("response:\n%s\n", resp);

  free(resp);

  return EXIT_SUCCESS;
}
