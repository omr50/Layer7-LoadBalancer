// place holder
#include <http-parser>
#include <vector>


class Connection {
	int client_fd;
	int server_fd;
	http_parser parser;
	std::vector<unsigned char> buffer;

	Connection();
};
