#include "network.hpp"

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <cerrno>

NetworkManager::NetworkManager()
    : epoll_fd_(-1), epoll_ready_(false)
{
}

NetworkManager::~NetworkManager() {
    cleanup();
}

bool NetworkManager::addMulticast(const char* mcast_ip, const char* mcast_port, const char* local_ip) {
    // Create socket
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Set non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Failed to set non-blocking: " << strerror(errno) << std::endl;
        close(fd);
        return false;
    }

    // Reuse port and address
    int val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0) {
        std::cerr << "Failed to set SO_REUSEADDR: " << strerror(errno) << std::endl;
        close(fd);
        return false;
    }
    
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(int)) < 0) {
        std::cerr << "Failed to set SO_REUSEPORT: " << strerror(errno) << std::endl;
        close(fd);
        return false;
    }

    // Bind to INADDR_ANY, not the multicast address
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // IMPORTANT: Bind to INADDR_ANY
    addr.sin_port = htons(std::atoi(mcast_port));

    if (bind(fd, (const sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind port " << mcast_port << ": " << strerror(errno) << std::endl;
        close(fd);
        return false;
    }

    // Join multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(mcast_ip);
    mreq.imr_interface.s_addr = inet_addr(local_ip);

    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        std::cerr << "Failed to join multicast " << mcast_ip << ": " << strerror(errno) << std::endl;
        close(fd);
        return false;
    }

    std::cout << "Joined multicast " << mcast_ip << ":" << mcast_port << " on interface " << local_ip << std::endl;
    sockets_.push_back(fd);
    return true;
}

bool NetworkManager::setupEpoll() {
    if (sockets_.empty()) {
        std::cerr << "No sockets to poll" << std::endl;
        return false;
    }

    epoll_fd_ = epoll_create(128);
    if (epoll_fd_ < 0) {
        std::cerr << "Failed to create epoll: " << strerror(errno) << std::endl;
        return false;
    }

    for (int sock : sockets_) {
        epoll_event event;
        event.data.fd = sock;
        event.events = EPOLLIN;

        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sock, &event) < 0) {
            std::cerr << "Failed to add socket to epoll: " << strerror(errno) << std::endl;
            close(epoll_fd_);
            epoll_fd_ = -1;
            return false;
        }
    }

    epoll_ready_ = true;
    return true;
}

ssize_t NetworkManager::poll(char* buf, std::size_t max_len, int& out_fd) {
    if (!epoll_ready_) {
        out_fd = -1;
        return -1;
    }

    epoll_event events[16];
    int nfds = epoll_wait(epoll_fd_, events, 16, 0);

    if (nfds < 0) {
        if (errno == EINTR) {
            out_fd = -1;
            return 0;  // Interrupted, try again
        }
        std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
        out_fd = -1;
        return -1;
    }

    if (nfds == 0) {
        out_fd = -1;
        return 0;  // No data available
    }

    // Read from first ready socket
    for (int i = 0; i < nfds; ++i) {
        ssize_t bytes = read(events[i].data.fd, buf, max_len);

        if (bytes > 0) {
            out_fd = events[i].data.fd;  // Set which socket we read from
            return bytes;
        }

        if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Read error: " << strerror(errno) << std::endl;
        }
    }

    out_fd = -1;
    return 0;
}

void NetworkManager::cleanup() {
    for (int sock : sockets_) {
        close(sock);
    }
    sockets_.clear();

    if (epoll_fd_ >= 0) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }

    epoll_ready_ = false;
}