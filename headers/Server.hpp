#pragma once
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>
#include "./Connection.hpp"
#include "./ConnectionPool.hpp"
#include <stack>

class Server {

	public:
		int epoll_fd;
		int listen_fd;
		int server_id;
		std::unordered_map<int, Connection*> connections;	
		std::vector<epoll_event> events{1024};
		std::vector<Connection> client_pool;	
		std::vector<Connection*> free_connections;
		ConnectionPool* pool;

		Server(int id);
		void worker_main();
		void create_connection(int client_fd);
		void accept_new_connection();
		void handle_read(Connection* conn);
		void handle_write(Connection* conn);
		void close_connection(Connection* conn);
};
