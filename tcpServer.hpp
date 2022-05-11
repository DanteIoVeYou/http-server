#pragma once 

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "protocol.hpp"
#include "log.hpp"
#define DEFAULT_PORT 8081
namespace HttpServer {
class TcpServer {
public:
  static TcpServer* GetInstance(uint16_t port) {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    if(_tcp_server == nullptr) {
      pthread_mutex_lock(&lock);
      if(_tcp_server == nullptr) {
        _tcp_server = new TcpServer;
        _tcp_server->TcpServerInit(port);
      }
      pthread_mutex_unlock(&lock);
    }
    return _tcp_server;
  }
  int ListenSocket() {
    return _listen_sock;
  }

private:
  TcpServer(): _listen_sock(-1) {}

  ~TcpServer() {}

  TcpServer(const TcpServer& srv) = delete;

  TcpServer operator=(const TcpServer& srv) = delete;

  void SocketCreate() {
    // 1. create socket
    _listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(_listen_sock < 0) {
      LOG(FATAL, "create listen socket failed...");
      exit(1);
    }
    LOG(INFO, "create listen socket success");
    int opt = 1;
    setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  }

  void SocketBind() {
    // 2. bind socket
    sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(_port);
    local.sin_addr.s_addr = INADDR_ANY;
    if(bind(_listen_sock, (struct sockaddr*)&local, sizeof(local)) < 0) {
      LOG(FATAL, "bind socket failed...");
      exit(2);
    }
    LOG(INFO, "bind socket success");
  }

  void Listen() {
    // 3. listen
    if(listen(_listen_sock, 5) < 0) {
      LOG(FATAL, "listen failed...");
      exit(3);
    }
    LOG(INFO, "listen success");
  }

  void TcpServerInit(uint16_t port) {
    _port = port;
    SocketCreate();
    SocketBind();
    Listen();
    LOG(INFO, "initialize HttpServer success");
  }

public:
  void TcpServerStart() {
  }

private:
  uint16_t _port;
  int _listen_sock;
  static TcpServer* _tcp_server;
};

TcpServer* TcpServer::_tcp_server = nullptr;

} // namespace
