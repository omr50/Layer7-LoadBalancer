#pragma once
#include <vector>
#include <unordered_set>
#include <unordered_map>

struct LL_Connection {
	int server;
	int fd;
	bool free;
};

class ConnectionPool {

	public:
		std::vector<LL_Connection> connections;
		std::unordered_set<int> connecting_fds;
		std::unordered_map<int,int> fd2idx;
		int conn_per_server;
		int total_connections;
		int curr_connected;
		int start_port;
		int epoll_fd;
		int curr_index;

		ConnectionPool(int start_port, int num_conn_per_server, int epoll_fd);
		void create_connections();
		bool conn_exists(int fd);
		LL_Connection* return_conn();

		void delete_conn(int conn);
		void replace_conn(int conn);
		void update_connection_status(int fd, bool status);


};
