#pragma once
#include <vector>
#include <unordered_set>

struct LL_Connection {
	int server;
	int fd;
	bool free;
};

class ConnectionPool {

	public:
		std::vector<std::vector<LL_Connection>> connections;
		std::unordered_set<int> connecting_fds;
		int conn_per_server;
		int total_connections;
		int curr_connected;
		int start_port;
		int epoll_fd;

		ConnectionPool(int start_port, int num_conn_per_server, int epoll_fd);
		void create_connections();
		bool conn_exists(int fd);
		void return_conn(int conn);
		void delete_conn(int conn);
		void replace_conn(int conn);
		void update_connection_status(int fd, bool status);


};
