// place holder
#include <http_parser.h>
#include <vector>

enum class State {
	READING_REQUEST,
	WRITING_REQUEST,
	READING_RESPONSE,
	WRITING_RESPONSE,
	CLOSED
};


class Connection {
	public:
		int client_fd = -1;
		int server_fd = -1;
		State state = State::READING_REQUEST;
		http_parser request_parser;
		http_parser_settings request_settings;
		http_parser response_parser;
		http_parser_settings response_settings;
		std::vector<unsigned char> request_buffer;
		std::vector<unsigned char> response_buffer;
		
		Connection(int client_fd);
		void close_connection(int epoll_fd);
		static int on_request_complete(http_parser* parser);
		static int on_response_complete(http_parser* parser);

};
