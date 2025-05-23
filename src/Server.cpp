#include "../headers/Server.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

Server::Server(int id) {
	server_id = id;
	epoll_fd = epoll_create1();
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	int one = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,  &one, sizeof(one));
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
	fcntl(fd, F_SETFL, O_NONBLOCK);
	sockaddr_in addr {AF_INET, htons(port), INADDR_ANY};
	bind(fd, (sockaddr*)&addr, sizeof(addr));
	listen(fd, SOMAXCONN);
	listen_fd = fd;
	connections[listen_fd] = new Connection();
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
				Connection* conn = nullptr;
				try {
					Connection* conn = connections[fd];
				} catch (e) {
					perror("error with finding connection\n");
					continue;
				}
				
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
}

void Server::handle_read(Connection* conn) {
	int fd = (conn->status == Status::READING_REQUEST) ? conn->client_socket : conn->server_socket;
	char buff[4096];
	http_parser* parser = (conn->status == Status::READING_REQUEST) ? &conn->request_parser : &conn->response_parser;
	http_parser_settings* parser_settings = (conn->status == Status::READING_REQUEST) ? &conn->request_settings: &conn->response_settings;
	std::vector<unsigned char> vec_buffer = (conn->status == Status::READING_REQUEST) ? &conn->request_buffer : &conn->response_buffer;

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
					parser_settings
					buff,
					buff+n);
		} 
		else if (n == -1 && (errno == EAGAIN|| errno == EWOULDBLOCK)) {
			break;
		}
		// n = 0 or other errors
		else {
			close_connection(conn);
		}
}

void Server::handle_write(Connection* conn) {
	return;
}


void Server::close_connection(Connection* conn) {
	conn->close_connection(epoll_fd);	
}

