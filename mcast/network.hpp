#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include <vector>
#include <string>
#include <cstddef>

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    
    bool addMulticast(const char* mcast_ip, const char* mcast_port, const char* local_ip);
    bool setupEpoll();
    ssize_t poll(char* buf, std::size_t max_len, int& out_fd);
    void cleanup();
    
    bool isReady() const { return epoll_ready_; }
    const std::vector<int>& getSockets() const { return sockets_; }

private:
    std::vector<int> sockets_;
    int epoll_fd_;
    bool epoll_ready_;
};

#endif // NETWORK_HPP