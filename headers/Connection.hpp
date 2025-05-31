// place holder
#pragma once
#include <http_parser.h>
#include <vector>
#include "./ConnectionPool.hpp"

#define BUFF_SIZE 8192 

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
		char request_buffer[BUFF_SIZE];
		int req_bytes_read = 0;
		int req_bytes_written = 0;
		char response_buffer[BUFF_SIZE];
		int res_bytes_read = 0;
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
