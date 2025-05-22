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
			Connection* conn = connections[fd];
			if (fd == listen_fd) {
				accept_new_connection(); 
			}
			else {
				if (evts & EPOLLIN) handle_read(conn);
				else if (evts & EPOLLOUT) handle_write(conn);
				else if (evts & EPOLLHUP | EPOLLERR) close_connection(conn);
			}
		}

	}

}

void Server::accept_new_connection() {

}

void Server::handle_read(Connection* conn) {

}

void Server::handle_write(Connection* conn) {

}

void Server::close_connection(Connection* conn) {

}
