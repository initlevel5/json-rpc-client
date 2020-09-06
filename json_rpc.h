/*
 * Copyright (c) 2020 initlevel5
 *
 * This module provides access to JSON-RPC services.
 */
#ifndef JSON_RPC_H_
#define JSON_RPC_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * This function connects to the JSON-RPC service and invokes the remote method
 * 'method' with the given parameters 'params'.
 *
 * When the response is received, HTTP header and body will be copied to
 * response buffer 'resp'.
 *
 * Returns -1 on failure and 0 on success.
 * 
 * Usage example:
 *
 *  char buf[1024];
 *
 *  if (json_rpc_request("localhost", 8000, "/api/json/v2", "sysinfo", "[]", 0,
 *                       buf, sizeof(buf)) != 0) {
 *    exit(EXIT_FAILURE);
 *  }
 *
 *  printf("response:\n%s\n", buf);
 */
int json_rpc_request(const char *host, int port, const char *path,
                     const char *method, const char *params, int id,
                     char *resp, int resp_sz);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* JSON_RPC_H_ */
