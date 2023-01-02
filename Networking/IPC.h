#ifndef _IPC
#define _IPC


#include <pthread.h>
#include <signal.h>
#include <string>


// Used to communicate with other processes on the same server
class IPCSocket {
    public:
  static IPCSocket* Instance();

  bool start();
  void stop();
  void set_socket_path(const std::string& path);
//   void set_socket_action(void (*action)(int, siginfo_t*, void*));

  enum {
    MAX_BACKLOG = 20,
    BUFFER_SIZE = 8192
  };

private:
  IPCSocket();
  ~IPCSocket();

  static void* thread_starter(void* obj);
  static void thread_stopper(int sig, siginfo_t* siginfo, void* context);
  void* run_server();
  void handle_signals();
  void cleanup_socket();

  pthread_t pid_;
  pthread_mutex_t mutex_;
  std::string socket_path_;
  int sockfd_;
  int curr_sock_fd_;
  static IPCSocket* instance_;
};

#endif