package main

import (
	    "flag"		
	    "fmt"
	    "net/http"
	)
func main() {
	// Define a “port” flag, default 8080
	port := flag.Int("port", 8080, "TCP port to listen on")
	flag.Parse()

	addr := fmt.Sprintf(":%d", *port)
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprint(w, "OK\n")
	})

	fmt.Printf("Listening on http://localhost%s\n", addr)
	if err := http.ListenAndServe(addr, nil); err != nil {
		panic(err)
	}
}
