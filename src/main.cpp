#include <thread>
#include "../headers/Worker.hpp"


int main() {
	auto n = std::thread::hardware_concurrency();

	for (int i = 1; i < n; ++i) {
		pid_t pid = fork();
		if (pid < 0) perror("fork"), exit(1);
		// child exits
		if (pid == 0) break;
	}

	worker_main();
	return 0;
}
