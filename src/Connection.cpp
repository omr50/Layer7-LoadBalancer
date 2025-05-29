#include "../headers/Connection.hpp"
#include "../headers/Server.hpp"
#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

Connection::Connection(Server* server) : server(server)  {
        http_parser_init(&response_parser, HTTP_RESPONSE);
	memset(&request_settings, 0, sizeof(request_settings));
	memset(&response_settings, 0, sizeof(response_settings));

	http_parser_init(&request_parser, HTTP_REQUEST);
	request_parser.data = this;
        http_parser_init(&response_parser, HTTP_RESPONSE);
	response_parser.data = this;

	request_settings.on_message_complete = on_request_complete;
	response_settings.on_message_complete = on_response_complete;

	conn_close_header_exists = false;
}

Connection::Connection(Server* server, int client_fd) : server(server), client_fd(client_fd)  {
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

    // initiate write -> grab a pooled backend_fd, then register EPOLLOUT on it, etc.
    if (conn->server_fd == -1)
    	conn->backend_connection = conn->server->pool->return_conn();
    // printf("Server backend connection %p\n", conn->backend_connection);
    conn->server_fd = conn->backend_connection->fd; 
    // epoll_ctl(conn->server->epoll_fd, EPOLL_CTL_DEL, conn->client_fd, nullptr);
    epoll_event ev { .events = EPOLLOUT | EPOLLET, .data = { .fd = conn->server_fd } };
    epoll_ctl(conn->server->epoll_fd, EPOLL_CTL_ADD, conn->server_fd, &ev);
    conn->server->connections[conn->server_fd] = conn;
    conn->server->handle_write(conn);


    // modify vector to remove the Connection: close
    static const char* target = "Connection: close\r\n";
    static std::vector<unsigned char> target_vector(target, target + strlen(target));

    auto it = std::search(conn->request_buffer.begin(), conn->request_buffer.end(), target_vector.begin(), target_vector.end());

    if (it != conn->request_buffer.end()) {
	conn->conn_close_header_exists = true;
	conn->request_buffer.erase(it, it + target_vector.size());	
    }
    
    // conn->initiate_write_state();
    return 0;
}

int Connection::on_response_complete(http_parser* parser) {

    auto* conn = static_cast<Connection*>(parser->data);
    conn->state = State::WRITING_RESPONSE;

    // initiate write -> grab a pooled backend_fd, then register EPOLLOUT on it, etc.
    // epoll_ctl(conn->server->epoll_fd, EPOLL_CTL_DEL, conn->server_fd, nullptr);
    epoll_event ev { .events = EPOLLIN | EPOLLOUT | EPOLLET, .data = { .fd = conn->client_fd} };
    epoll_ctl(conn->server->epoll_fd, EPOLL_CTL_ADD, conn->client_fd, &ev);
    if (conn->conn_close_header_exists) {
	// put it back into the function
	static const char* header = "Connection: close\r\n";
	static const char* target = "\r\n\r\n";
    	static std::vector<unsigned char> insert_vector(header, header + strlen(header));
    	static std::vector<unsigned char> target_vector(target, target + strlen(target));


	std::vector<unsigned char>::iterator end_of_headers = std::search(conn->response_buffer.begin(), conn->response_buffer.end(), target_vector.begin(), target_vector.end());

	if (end_of_headers != conn->response_buffer.end()) {

		end_of_headers += 2;
		conn->response_buffer.insert(end_of_headers, target_vector.begin(), target_vector.end());
	}
    }
    // epoll event might trigger before we get to next iteration so trying to handle write here to see 
    // conn->initiate_write_state();
    conn->server->handle_write(conn);
    return 0;
}

void Connection::close_connection(int epoll_fd) {
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, server_fd, nullptr);


	if (client_fd != -1)
		close(client_fd);
	// printf("closed connection socket, and removed from epoll\n");

	// if (server_fd != -1)
		// pool->return_conn(server_fd);
}


void Connection::reset() {
	client_fd = -1;
	server_fd = -1;

	state = State::READING_REQUEST;

	request_buffer.clear();
	response_buffer.clear();
	request_buffer.reserve(128);
	// request_buffer.shrink_to_fit();
	// response_buffer.shrink_to_fit();
	req_bytes_written = 0;
	res_bytes_written = 0;
    	conn_close_header_exists = false;

	// http_parser_init(&request_parser, HTTP_REQUEST);
        // request_parser.data = this;
        // memset(&request_settings, 0, sizeof(request_settings));
        // request_settings.on_message_complete = on_request_complete;

        // http_parser_init(&response_parser, HTTP_RESPONSE);
        // response_parser.data = this;
        // memset(&response_settings, 0, sizeof(response_settings));
        // response_settings.on_message_complete = on_response_complete;
}

void Connection::state_reset() {

	state = State::READING_REQUEST;
	request_buffer.clear();
	response_buffer.clear();
	request_buffer.reserve(128);
	// request_buffer.shrink_to_fit();
	// response_buffer.shrink_to_fit();
	req_bytes_written = 0;
	res_bytes_written = 0;

	http_parser_init(&request_parser, HTTP_REQUEST);
    	request_parser.data = this;
    	http_parser_init(&response_parser, HTTP_RESPONSE);
    	response_parser.data = this;

	// restart reading again
	epoll_event ev { .events = EPOLLIN | EPOLLET, .data = { .fd = client_fd }};
	epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
	server->handle_read(this);
    	// conn_close_header_exists = false;
}
