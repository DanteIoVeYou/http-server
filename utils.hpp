#pragma once 
#include <iostream>
#include <string>
#include <ctime>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace HttpServer {

void Manual(std::string proc) {
  printf("invalid command\n\t Try \'%s [port]\'\n", proc.c_str());

}

class Time {
public:
  static std::string GetCurrentTime() {
    time_t timestamp = time(nullptr);
    struct tm* timestruct;
    timestruct = localtime(&timestamp);
    char buffer[64];
    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", timestruct);
    std::string format_time = buffer;
    return format_time;
  }
}; 

void ReadLine(int sock, std::string* line) {
  char ch = '\3';
  while(ch != '\n') {
    recv(sock, &ch, 1, 0);
    if(ch != '\n') {
      if(ch != '\r') {
        // 普通字符
        (*line).push_back(ch);
      }
      else {
        // 试探一下
        recv(sock, &ch, 1, MSG_PEEK);
        if(ch != '\n') {
          // 以'\r'作为分隔符
          ch = '\n';
          (*line).push_back(ch);
        }
        else {
          // 以'\r\n'作为分隔符
          recv(sock, &ch, 1, 0);
          (*line).push_back(ch);
        }
      }
    }
    else {
      // 以'\n'作为分隔符
      (*line).push_back(ch);
    }
  }
}

} //namespace
