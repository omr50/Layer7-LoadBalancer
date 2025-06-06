#include "../headers/ConnectionPool.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>


ConnectionPool::ConnectionPool(int start_port, int num_conn_per_server, int epoll_fd) 

	: start_port(start_port), conn_per_server(num_conn_per_server), epoll_fd(epoll_fd)
{
	curr_connected = 0;
	curr_index = 0;
	total_connections = num_conn_per_server * 8; 
	create_connections();
}


void ConnectionPool::create_connections() {
	for (int i = 0; i < conn_per_server; i++) {
		for (int port = start_port;port < start_port + 8; port++) {

			int fd = socket(AF_INET, SOCK_STREAM, 0);
			if (fd < 0) throw std::runtime_error("socket: " + std::string(std::strerror(errno)));

			int flags = fcntl(fd, F_GETFL, 0);
			fcntl(fd, F_SETFL, flags | O_NONBLOCK);

			int yn = 1;
			if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yn, sizeof(yn)) < 0) {
				close(fd);
				throw std::runtime_error("setsockopt SO_KEEPALIVE: " + std::string(std::strerror(errno)));
			}

			int idle  = 60;    // start probes after 60s idle
			int intvl = 10;    // 10s between probes
			int cnt   = 5;     // give up after 5 failures
			setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,  &idle,  sizeof(idle));
			setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
			setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,   &cnt,   sizeof(cnt));

			// disable Nagle for lower latency
			int flag = 1;
			setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

			sockaddr_in backend_addr {AF_INET, htons(port), INADDR_ANY};
			if (inet_pton(AF_INET, "127.0.0.1", &backend_addr.sin_addr) != 1) {
				throw std::runtime_error("inet pton failed");
			}

			int res = connect(fd, (const sockaddr*)&backend_addr, sizeof(backend_addr));
			if (res < 0 && errno != EINPROGRESS) {
				close(fd);
				throw std::runtime_error("connect: " + std::string(std::strerror(errno)));
				continue;
			}
			epoll_event ev{ .events = EPOLLOUT | EPOLLERR, .data = {.fd = fd} };
			epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
			connecting_fds.insert(fd);
			// printf("fd is %d\n", fd);
			connections.push_back({ port, fd, false});
			fd2idx[fd] = connections.size() - 1;
		}
	}
	printf("Initiated all connections (still waiting to complete)\n");
}


bool ConnectionPool::conn_exists(int fd) {

	return connecting_fds.count(fd);
}

void ConnectionPool::update_connection_status(int fd, bool status) {
	auto it = fd2idx.find(fd);
	if (it != fd2idx.end()) {
		auto idx = it->second;
		connections[idx].free = status;
		// printf("updated conn fd %d status %b\n", connections[idx].fd, connections[idx].free);
		// connections[idx].server = start_port;
		connections[idx].fd = fd;
		// connections[port][i] = { start_port + port, fd, status };
		return;
	}
	printf("CONNECTION DOESN't EXIST!\n");
}


LL_Connection* ConnectionPool::return_conn() {
	auto conn = &connections[curr_index];
	// printf("Conn status %d %d %b\n", conn->server, conn->fd, conn->free);
	if (conn->free) {
		// printf("Got a new socket #%d from server %d\n", connection_fd, start_port + curr_server);
		curr_index = (curr_index + 1) % (8 * conn_per_server);
		// printf("Next server is: %d\n", start_port + curr_server);
		conn->free = false;	
		return conn;
	}
	else {
		perror("Didn't find any available connections!\n");
		return nullptr;
	}
	
}

void ConnectionPool::delete_conn(int conn) {
	return;
}
void ConnectionPool::replace_conn(int conn) {
	return;
}
