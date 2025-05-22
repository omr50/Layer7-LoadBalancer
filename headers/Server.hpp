#pragma once
#include <sys/epoll.h>
#include <unordered_map>
#include "./Connection.hpp"

class Server {

	public:
		int epoll_fd;
		int listen_fd;
		unordered_map<int, Connection*> connections;	


		void worker_main();
		void accept_new_connection();
		void handle_read(Connection* conn);
		void handle_write(Connection* conn);
		void close_connection(Connection* conn);

};
