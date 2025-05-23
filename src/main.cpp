#include <thread>
#include "../headers/Server.hpp"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int main() {
	auto n = std::thread::hardware_concurrency();
	int id = 0;
	for (int i = 1; i < n; ++i) {
		pid_t pid = fork();
		if (pid < 0) perror("fork"), exit(1);
		// child exits
		if (pid == 0) {
			id = i;
			break;
		}
	}
	Server server(id);
	server.worker_main();
	return 0;
}
