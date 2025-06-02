#include "../headers/Server.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <chrono>

Server::Server(int id) {
	server_id = id;
	client_pool.reserve(4000);
	for (int i = 0; i < 4000; i++) {
		client_pool.emplace_back(this);
		free_connections.push_back(&client_pool.back());
	}
	epoll_fd = epoll_create1(0);
	pool = new ConnectionPool(8080, 500, epoll_fd);
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	int one = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,  &one, sizeof(one));
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
	fcntl(fd, F_SETFL, O_NONBLOCK);
	uint16_t port = 8000;
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
	
	// using Clock = std::chrono::steady_clock;
	// auto last_stats = Clock::now();
	// auto const stats_interval = std::chrono::seconds(5);
	// main loop
	while (true) {
		/*
		auto now = Clock::now();
		if (now - last_stats >= stats_interval) {
			last_stats = now;
			int pool_used = 0;
			for (int i = 0; i < 8; i ++) {
				for (int j = 0; j < pool->conn_per_server; j ++) {
					if (!pool->connections[i][j].free)
						pool_used++;

				}

			}
			printf("Num connections: %ld\nNum pool used: %d\n", connections.size(), pool_used);	
		}
		*/
		int n = epoll_wait(epoll_fd, events.data(), events.size(), -1);

		for (int i = 0; i < n; i++) {
			int fd = events[i].data.fd;
			auto evts = events[i].events;
			if (fd == listen_fd) {
				accept_new_connection(); 
			}
			else if (pool->conn_exists(fd)) {
				int err = 0;
				socklen_t len = sizeof(err);
				pool->connecting_fds.erase(fd);
				getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
				if (err == 0) {
					pool->update_connection_status(fd, true);
					pool->curr_connected++;
					if (pool->curr_connected == pool->total_connections)
						printf("ALL SOCKETS SUCCESSFULLY CONNECTED TO BACKEND!\n");
				} else {

					pool->update_connection_status(fd, false);
					printf("FAILED TO CONNECT TO BACKEND, RETRY LATER!\n");
					close(fd);
				}
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
	if (free_connections.size()) {
		Connection* connection = free_connections[free_connections.size() - 1];	
		free_connections.pop_back();
		connection->client_fd = client_fd;
		connections[client_fd] = connection;
		// printf("Server %d CREATED A CONNECTION\n", server_id);
		handle_read(connection);
	}
	else {
		perror("failed to find an open connection!\n");
	}
}

void Server::handle_read(Connection* conn) {
	int fd = (conn->state == State::READING_REQUEST) ? conn->client_fd : conn->server_fd;

	http_parser* parser = (conn->state == State::READING_REQUEST) ? 
		&conn->request_parser 
		: &conn->response_parser;

	http_parser_settings* parser_settings = (conn->state == State::READING_REQUEST) ? 
		&conn->request_settings
		: &conn->response_settings;

	char* buffer = (conn->state == State::READING_REQUEST) ? conn->request_buffer: conn->response_buffer;
	int *bytes_read = (conn->state == State::READING_REQUEST) ? &conn->req_bytes_read : &conn->res_bytes_read;

	while (true) {
		char* curr_buff_end = buffer + (*bytes_read);
		size_t read_limit = BUFF_SIZE - (size_t)(curr_buff_end - buffer);
		// printf("read limit %d\n", read_limit);
		ssize_t n = read(fd, curr_buff_end, BUFF_SIZE - (size_t)(curr_buff_end - buffer));
		if (n > 0) {
			(*bytes_read) += n;
			size_t parsed = http_parser_execute(
					parser,	
					parser_settings,
					curr_buff_end,
					n);
		} 
		else if (n == -1 && (errno == EAGAIN|| errno == EWOULDBLOCK)) {
			break;
		}
		// n = 0 or other errors
		else {
			printf("Read EOF, CONNECTION CLOSING\n");
			close_connection(conn);
			return;
		}
	}
	//printf("\n");
	//printf("Request coming for server %d\n", server_id);
}

void Server::handle_write(Connection* conn) {
	int fd = (conn->state == State::WRITING_REQUEST) ? conn->server_fd: conn->client_fd;
	char *buffer = (conn->state == State::WRITING_REQUEST) ? conn->request_buffer : conn->response_buffer;
	int* bytes_written = (conn->state == State::WRITING_REQUEST) ? &conn->req_bytes_written : &conn->res_bytes_written;
	int* bytes_read = (conn->state == State::WRITING_REQUEST) ? &conn->req_bytes_read: &conn->res_bytes_read;

	while (true) {
		char* curr_buff_end = buffer + (*bytes_written);
		// printf("Amount: %d\n", BUFF_SIZE  - (size_t)(curr_buff_end - buffer));
		int bytes_left = (*bytes_read) - (size_t)(curr_buff_end - buffer);
		ssize_t n = write(fd, curr_buff_end, bytes_left); 
		(*bytes_written) += n;
		// printf("Bytes Written: %d\n", (*bytes_written));
		if ((*bytes_written) == (*bytes_read)) {
			// printf("WROTE ALL!\n");
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
			epoll_event ev { .events = EPOLLIN | EPOLLET, .data = { .fd = fd} };
			epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
			if (conn->state == State::WRITING_REQUEST)
				conn->state = State::READING_RESPONSE;
			else if (conn->state == State::WRITING_RESPONSE){
				// close_connection(conn);
				// printf("RESET?\n");
				conn->state_reset();
			}

			break;
		}
		else if (n > 0) {
			// printf("wrote something!\n");
		}
		else if (n == 0) {
			printf("N = 0 on WRITE?, handle this\n");
			break;
		}
		else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			else {
				perror("WRITE ERROR REQUEST -> SERVER\n");
				break;
			}
		}	

	}
}


void Server::close_connection(Connection* conn) {
	conn->close_connection(epoll_fd);	
	/* auto it = connections.find(conn->client_fd);
	if (it != connections.end()) {
		connections.erase(it);
	}
	it = connections.find(conn->server_fd);
	if (it != connections.end()) {
		connections.erase(it);
	}*/

	if (conn->server_fd != -1)
		conn->backend_connection->free = true;
	conn->backend_connection = nullptr;
	conn->reset();
	free_connections.push_back(conn);	
	// printf("Deleted Connection!\n");
}

