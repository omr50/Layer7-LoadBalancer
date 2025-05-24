#include "../headers/Connection.hpp"
#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

Connection::Connection(int client_fd) : client_fd(client_fd)  {
        http_parser_init(&response_parser, HTTP_RESPONSE);
	memset(&request_settings, 0, sizeof(request_settings));
	memset(&response_settings, 0, sizeof(response_settings));

	http_parser_init(&request_parser, HTTP_REQUEST);
	request_parser.data = this;
        http_parser_init(&response_parser, HTTP_RESPONSE);
	response_parser.data = this;

	request_settings.on_message_complete = on_request_complete;
	response_settings.on_message_complete = on_response_complete;

}

int Connection::on_request_complete(http_parser* parser) {
    auto* conn = static_cast<Connection*>(parser->data);
    conn->state = State::WRITING_REQUEST;

    printf("FINISHED READING CLIENT REQUEST!!!!!!!!\n");
    // initiate write -> grab a pooled backend_fd, then register EPOLLOUT on it, etc.
    conn->initiate_write_state();
    return 0;
}

int Connection::on_response_complete(http_parser* parser) {
	return 0;
}

void Connection::close_connection(int epoll_fd) {
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, server_fd, nullptr);

	if (client_fd != -1)
		close(client_fd);
	printf("closed connection socket, and removed from epoll\n");

	// if (server_fd != -1)
		// pool->return_conn(server_fd);
}
