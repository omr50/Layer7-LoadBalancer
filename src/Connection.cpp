#include "../headers/Connection.hpp"
#include <sys/epoll.h>

Connection::Connection(int client_fd) : client_fd(client_fd)  {
	http_parser_init(&request_parser, HTTP_REQUEST);
        http_parser_init(&response_parser, HTTP_RESPONSE);
}

Connection::close_connection(int epoll_fd) {
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr); 
}

