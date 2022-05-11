#pragma once 

#include "tcpServer.hpp"

namespace HttpServer {
class HttpServer {
public:

  HttpServer(uint16_t port = DEFAULT_PORT): _port(port){}

  ~HttpServer(){}
  

  void Loop() {
    LOG(INFO, "HttpServer Loop Begin");
    int listen_sock = TcpServer::GetInstance(_port)->ListenSocket(); 
    for(;;) {
      sockaddr_in peer;
      socklen_t len = 0;
      memset(&peer, 0, sizeof(peer));
      int sock = accept(listen_sock, (struct sockaddr*)&peer, &len);
      if(sock < 0) {
        LOG(ERROR, "create socket failed...");
        continue;
      }
      LOG(INFO, "create socket success");
      pthread_t tid;
      int* ptr = new int(sock);
      pthread_create(&tid, nullptr, Handler, (void*)ptr);
      pthread_detach(tid);
      LOG(INFO, "get a new link");
    }
  }

private:
  uint16_t _port;
};

} //namespace
