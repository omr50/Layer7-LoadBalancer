#include <vector>

struct LL_Connection {
	int fd;
	bool free;
}

class ConnectionPool {

	public:
		std::vector<std::vector<LL_Connection>> connections;

		ConnectionPool(int num_conn_per_server);
		void create_connections();
		void return_conn(int conn);
		void delete_conn(int conn);
		void replace_conn(int conn);
};
