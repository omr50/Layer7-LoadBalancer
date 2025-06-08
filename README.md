## Layer 7 loadbalancer that is HTTP 1.1 compatible.

- The load balancer uses epoll for non-blocking I/O
- Multiple worker processes forked
- port-reuse enabled to balance incoming request among available servers
- Connection pool created to allow long term connections between  balancer and backend http servers 
- Round Robin load balancing
- benchmarking and testing shows it handles above 100,000 connections per second
- Health checks (in progress)
