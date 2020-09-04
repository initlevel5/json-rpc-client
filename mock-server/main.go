// 
// Simple JSON-RPC mock-server
//
package main

import (
	"io"
	"net"
	"net/http"
	"net/rpc"
	"github.com/powerman/rpc-codec/jsonrpc2"
)

type HttpConn struct {
	in io.Reader
	out io.Writer
}

func (c *HttpConn) Read(p []byte) (int, error) {
	return c.in.Read(p)
}

func (c *HttpConn) Write(p []byte) (int, error) {
	return c.out.Write(p)
}

func (c *HttpConn) Close() error {
	return nil
}

type Test struct{}

type Args struct {
	Name string
}

func (t *Test) SayHello(args *Args, reply *string) error {
	*reply = "Hello " + args.Name
	return nil
}

func main() {
	srv := rpc.NewServer()

	srv.Register(&Test{})

	l, err := net.Listen("tcp", "127.0.0.1:8000")

	if err != nil {
		panic(err)
	}

	defer l.Close()

	http.Serve(l, http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/api/json/v2" {
			codec := jsonrpc2.NewServerCodec(&HttpConn{in: r.Body, out: w}, srv)

			w.Header().Set("Content-type", "application/json")
			w.WriteHeader(200)

			if err2 := srv.ServeRequest(codec) ; err2 != nil {
				http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
				return
			}
		} else {
			http.Error(w, http.StatusText(http.StatusNotFound), http.StatusNotFound)
		}
	}))
}
