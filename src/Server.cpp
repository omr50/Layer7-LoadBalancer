#include "../headers/Server.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

Server::Server(int id) {
	server_id = id;
	epoll_fd = epoll_create1(0);
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	int one = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,  &one, sizeof(one));
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
	fcntl(fd, F_SETFL, O_NONBLOCK);
	uint16_t port = 8080;
	sockaddr_in addr {AF_INET, htons(port), INADDR_ANY};
	bind(fd, (sockaddr*)&addr, sizeof(addr));
	listen(fd, SOMAXCONN);
	listen_fd = fd;
	printf("Server %d's setup complete\n", server_id);
}

void Server::worker_main() {
	// register the listener
	epoll_event ev { .events=EPOLLIN, .data={.fd = listen_fd} };
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev); 
	
	// main loop
	while (true) {

		int n = epoll_wait(epoll_fd, events.data(), events.size(), -1);

		for (int i = 0; i < n; i++) {
			int fd = events[i].data.fd;
			auto evts = events[i].events;
			if (fd == listen_fd) {
				accept_new_connection(); 
			}
			else {
				auto it = connections.find(fd);
				if (it  == connections.end()) {
					printf("Can't find connection");
					continue;
				}
				Connection* conn = it->second;
				if (evts & EPOLLIN) handle_read(conn);
				else if (evts & EPOLLOUT) handle_write(conn);
				else if (evts & EPOLLHUP | EPOLLERR) close_connection(conn);
			}
		}

	}

}

void Server::accept_new_connection() {
	// edge triggered so keep accepting until no more
	while (true) {
		int client_fd = accept(listen_fd, nullptr, nullptr);
		if (client_fd < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) break;

			perror("accept");
			return;
		}
		fcntl(client_fd, F_SETFL, O_NONBLOCK);
		// set up event for epoll
		epoll_event ev { .events = EPOLLIN | EPOLLET, .data = { .fd = client_fd } };
		epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
		create_connection(client_fd);
	}
	
}

void Server::create_connection(int client_fd) {
	Connection* connection = new Connection(client_fd);	
	connections[client_fd] = connection;
	printf("Server %d CREATED A CONNECTION\n", server_id);
	handle_read(connection);
}

void Server::handle_read(Connection* conn) {
	int fd = (conn->state == State::READING_REQUEST) ? conn->client_fd : conn->server_fd;
	char buff[4096];
	http_parser* parser = (conn->state == State::READING_REQUEST) ? 
		&conn->request_parser 
		: &conn->response_parser;

	http_parser_settings* parser_settings = (conn->state == State::READING_REQUEST) ? 
		&conn->request_settings
		: &conn->response_settings;

	std::vector<unsigned char> *vec_buffer = (conn->state == State::READING_REQUEST) ? &conn->request_buffer : &conn->response_buffer;

	while (true) {
		
		ssize_t n = read(fd, buff, sizeof(buff));
		if (n > 0) {
			vec_buffer->insert(
					vec_buffer->end(),
					buff,
					buff + n
			);

			size_t parsed = http_parser_execute(
					parser,	
					parser_settings,
					buff,
					n);
		} 
		else if (n == -1 && (errno == EAGAIN|| errno == EWOULDBLOCK)) {
			break;
		}
		// n = 0 or other errors
		else {
			close_connection(conn);
			return;
		}
	}
	for (int i = 0 ; i < vec_buffer->size(); i++) {
		printf("%c", (*vec_buffer)[i]);
	}
	printf("\n");
	printf("Request coming for server %d\n", server_id);
}

void Server::handle_write(Connection* conn) {
	return;
}


void Server::close_connection(Connection* conn) {
	conn->close_connection(epoll_fd);	
	auto it = connections.find(conn->client_fd);
	if (it != connections.end()) {
		connections.erase(it);
	}
	printf("Deleted Connection!\n");
	delete conn;
}

