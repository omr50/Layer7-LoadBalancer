#pragma once
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>
#include "./Connection.hpp"
#include "./ConnectionPool.hpp"

class Server {

	public:
		int epoll_fd;
		int listen_fd;
		int server_id;
		std::unordered_map<int, Connection*> connections;	
		std::vector<epoll_event> events{1024};
		ConnectionPool* pool;

		Server(int id);
		void worker_main();
		void create_connection(int client_fd);
		void accept_new_connection();
		void handle_read(Connection* conn);
		void handle_write(Connection* conn);
		void close_connection(Connection* conn);
};
