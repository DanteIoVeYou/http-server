#pragma once 
#include <iostream>
#include <string>
//#include <cstdio>
//#include <cstdlib>
#include "utils.hpp"
namespace HttpServer {
#define LOG(level, message) Log::PrintLog(#level, message, __FILE__, __LINE__)
#define DEBUG 0
#define INFO 1
#define WARNING 2
#define ERROR 3
#define FATAL 4
// 日志级别
//enum Level{
//  INFO,
//  WARNING,
//  ERROR,
//  FATAL
//};
#define NONE         "\033[m"
#define RED          "\033[0;32;31m"
#define LIGHT_RED    "\033[1;31m"
#define GREEN        "\033[0;32;32m"
#define LIGHT_GREEN  "\033[1;32m""]"

class Log {
public:
  static void PrintLog(std::string level, std::string message, std::string filename, int line) {
    FILE* fp = fopen("srv.log", "a+");
    if(level == "DEBUG") {
      printf("%s    \033[0m\033[1;34m%s\033[0m [%s]\t<%d>\t%s\n", Time::GetCurrentTime().c_str(), level.c_str(), filename.c_str(), line, message.c_str());
      fprintf(fp, "%s    %s [%s]\t<%d>\t%s\n", Time::GetCurrentTime().c_str(), level.c_str(), filename.c_str(), line, message.c_str());
    }

    else if(level == "INFO") {
      printf("%s    \033[0m\033[1;32m%s\033[0m [%s]\t<%d>\t%s\n", Time::GetCurrentTime().c_str(), level.c_str(), filename.c_str(), line, message.c_str());
      fprintf(fp, "%s    %s [%s]\t<%d>\t%s\n", Time::GetCurrentTime().c_str(), level.c_str(), filename.c_str(), line, message.c_str());
      //std::cout << Time::GetCurrentTime() << "     " << level << " [" << filename << "]" << " <" << line << "> " << message << std::endl; 
    }
    else if(level == "ERROR") {
      printf("%s    \033[0m\033[1;31m%s\033[0m [%s]\t<%d>\t%s\n", Time::GetCurrentTime().c_str(), level.c_str(), filename.c_str(), line, message.c_str());
      fprintf(fp, "%s    %s [%s]\t<%d>\t%s\n", Time::GetCurrentTime().c_str(), level.c_str(), filename.c_str(), line, message.c_str());
    }
    else {
      std::cout << Time::GetCurrentTime() << "     " << level << " [" << filename << "]" << " <" << line << "> " << message << std::endl; 

    }
    fclose(fp);
  }

};
} //namespace
