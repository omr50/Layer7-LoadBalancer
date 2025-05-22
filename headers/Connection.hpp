// place holder
#include <http-parser>
#include <vector>

enum class State {
	READING_REQUEST,
	WRITING_REQUEST,
	READING_RESPONSE,
	WRITING_RESPONSE,
	CLOSED
};


class Connection {
	int client_fd = -1;
	int server_fd = -1;
	State state = State::READING_REQUEST;
	http_parser request_parser;
	http_parser response_parser;
	std::vector<unsigned char> request_buffer;
	std::vector<unsigned char> response_buffer;

	Connection(int client_fd);
	close_connection(int epoll_fd);
};
