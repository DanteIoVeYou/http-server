#pragma once 
#include <iostream>
#include <string>
#include <ctime>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
//#include "log.hpp"

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

bool CutString(const std::string& sentence, std::string* first_string, std::string* last_string, std::string sep) {
  // 如果找到分隔符，就以分隔符为界填入前后内容至输出参数并返回true；否则，返回false
  size_t pos = sentence.find(sep);
  if(pos != std::string::npos) {
    *first_string = sentence.substr(0, pos);
    *last_string = sentence.substr(pos + sep.size());
    return true;
  }
  else {
    //LOG(ERROR, "sep not found");
    std::cout << "sep not found" << std::endl;
    return false;
  }
}

std::string GetSuffix(const std::string& str) {
  auto pos = str.rfind(".");
  if(pos != str.npos) {
    return str.substr(pos + 1);
  }
  else {
    return nullptr;
  }
}

} //namespace
