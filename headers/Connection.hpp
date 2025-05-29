// place holder
#pragma once
#include <http_parser.h>
#include <vector>
#include "./ConnectionPool.hpp"

enum class State {
	READING_REQUEST,
	WRITING_REQUEST,
	READING_RESPONSE,
	WRITING_RESPONSE,
	WAITING_FOR_CLOSE,
	CLOSED
};

class Server;

class Connection {
	public:
		int client_fd = -1;
		int server_fd = -1;
		Server* server;
		State state = State::READING_REQUEST;
		LL_Connection* backend_connection;
		http_parser request_parser;
		http_parser_settings request_settings;
		http_parser response_parser;
		http_parser_settings response_settings;
		std::vector<unsigned char> request_buffer;
		int req_bytes_written = 0;
		std::vector<unsigned char> response_buffer;
		int res_bytes_written = 0;
		bool conn_close_header_exists = false;

		Connection(Server* server);	
		Connection(Server* server, int client_fd);
		void initiate_write_state();
		void close_connection(int epoll_fd);
		static int on_request_complete(http_parser* parser);
		static int on_response_complete(http_parser* parser);
		void reset();
		void state_reset();
};
