#include "../headers/Connection.hpp"
#include <sys/epoll.h>

Connection::Connection(int client_fd) : client_fd(client_fd)  {
	http_parser_init(&request_parser, HTTP_REQUEST);
        http_parser_init(&response_parser, HTTP_RESPONSE);

}

Connection::close_connection(int epoll_fd) {
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, server_fd, nullptr);

	if (client_fd != -1)
		close(client_fd)

	// if (server_fd != -1)
		// pool->return_conn(server_fd);
	delete this;
}
