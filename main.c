/*
 * Copyright (c) 2020 initlevel5
 *
 * This is a simple REST API client that
 * provides access to JSON-RPC services.
 */

#include <stdio.h>
#include <stdlib.h>

#include "json_rpc.h"

#define BUF_SIZE (1024)
#define ID (0)

static const char *host = "localhost"; //TODO use url parser
static int port = 8000;
static const char *path = "/api/json/v2";

int main(int argc, char const *argv[]) {
  char *resp = NULL;

  if ((resp = malloc(BUF_SIZE)) == NULL) exit(EXIT_FAILURE);

  /* TODO process command line arguments */
  
  if (json_rpc_request(host, port, path,
                       "Test.SayHello", "{\"Name\": \"Jesse Pinkman\"}", ID,
                       resp, BUF_SIZE) != 0) {
    printf("json_rpc_request() failed\n");
    exit(EXIT_FAILURE);
  }

  printf("response:\n%s\n", resp);

  free(resp);

  (void)argc;
  (void)argv;

  return EXIT_SUCCESS;
}
