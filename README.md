# json-rpc-client

This is a simple, dependency free REST API client that provides access to JSON-RPC services.

#### Usage

1. Build and run mock-server
    ```
    cd mock-server && go run main.go
    ```

2. Build client application
    ```
    $ cd ../ && make
    ```

3. Send a request to the JSON-RPC server
    ```
    $ ./client -a http://localhost:8000/api/json/v2 -m Test.SayHello -p '{"Name":"Jesse Pinkman"}'
	
	request:
	POST /api/json/v2 HTTP/1.1
	Host: localhost
	Accept: application/json; charset=UTF-8
	Connection: close
	Content-Type: application/json; charset=UTF-8
	Content-Length: 91

	{"jsonrpc": "2.0", "method": "Test.SayHello", "params": {"Name": "Jesse Pinkman"}, "id": 0}

	response:
	HTTP/1.1 200 OK
	Content-Type: application/json
	Date: Fri, 04 Sep 2020 14:08:10 GMT
	Content-Length: 56
	Connection: close

	{"jsonrpc":"2.0","id":0,"result":"Hello Jesse Pinkman"}

    ```

#### TODO:
- [x] Url parser
- [ ] Grow recv buffer dynamicaly
- [ ] Timeouts
- [ ] Tests