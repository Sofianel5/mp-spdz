#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include "IPC.h"

IPCSocket* IPCSocket::instance_ = NULL;

IPCSocket* IPCSocket::Instance() {
  if (instance_ == NULL) {
    instance_ = new IPCSocket();
  }
  return instance_;
}

IPCSocket::IPCSocket() : 
    pid_(0), 
    socket_path_("/tmp/mp-spdz.sock"), 
    sockfd_(0), 
    curr_sock_fd_(0) {
  pthread_mutex_init(&mutex_, NULL);
}

IPCSocket::~IPCSocket() {
  stop();
}

bool IPCSocket::start() {
    if (pthread_create(&pid_, NULL, &IPCSocket::thread_starter, this) != 0) {
        fprintf(stderr, "Failed to create IPC thread");
        return false;
    }
    fprintf(stderr, "Created IPC thread");
    handle_signals();
    return true;
}

void IPCSocket::stop() {
    fprintf(stdout, "Stopping IPC thread");
    pthread_mutex_destroy(&mutex_);
    if (pid_) {
        pthread_cancel(pid_);
        pid_ = 0;
    }
    if (sockfd_) {
        close(sockfd_);
        sockfd_ = 0;
    }
    cleanup_socket();
}

void IPCSocket::set_socket_path(const std::string& path) {
    socket_path_ = path;
}

void *IPCSocket::thread_starter(void* obj) {
    IPCSocket* ipc = static_cast<IPCSocket*>(obj);
    return ipc->run_server();
}

void IPCSocket::thread_stopper(int sig, siginfo_t *siginfo, void *context) {
    fprintf(stdout, "received signal %d %d %p", sig, siginfo->si_signo, context);
    Instance()->stop();
    exit(0);
}

void *IPCSocket::run_server() {
    fprintf(stderr, "Starting IPC thread");
    if (pthread_mutex_trylock(&mutex_) != 0) {
        fprintf(stderr, "Failed to lock mutex");
        return NULL;
    }

    cleanup_socket();

    if ((sockfd_ = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed to create socket");
        return NULL;
    }

    struct sockaddr_un addr;
    bzero(&addr, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path_.c_str());
    if (bind(sockfd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        fprintf(stderr, "Failed to bind socket");
        return NULL;
    }

    if (listen(sockfd_, MAX_BACKLOG) == -1) {
        fprintf(stderr, "Failed to listen on socket");
        return NULL;
    }

    struct sockaddr_un client_addr;
    char buffer[BUFFER_SIZE + 1];
    while (true) {
        socklen_t client_addr_len = sizeof(client_addr);
        if ((curr_sock_fd_ = accept(sockfd_, (struct sockaddr*)&client_addr, &client_addr_len)) == -1) {
            fprintf(stderr, "Failed to accept connection");
            continue;
        }
        bzero(buffer, BUFFER_SIZE + 1);
        if (recv(curr_sock_fd_, buffer, BUFFER_SIZE, 0) == -1) {
            fprintf(stderr, "Failed to receive data");
            continue;
        }
        fprintf(stdout, "=> read buffer [%s]\n", buffer);
        //close(curr_sock_fd_);
    }
}

void IPCSocket::handle_signals() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = &IPCSocket::thread_stopper;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "Failed to handle SIGINT");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        fprintf(stderr, "Failed to handle SIGTERM");
    }
}

void IPCSocket::cleanup_socket() {
    if (access(socket_path_.c_str(), F_OK) != -1) {
        fprintf(stdout, "Cleanup socket\n");
        unlink(socket_path_.c_str());
    }
}