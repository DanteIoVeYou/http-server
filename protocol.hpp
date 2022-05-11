#pragma once 
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utils.hpp"
#include "log.hpp"
namespace HttpServer {

struct HttpRequest {
  std::string request_line;
  std::string request_method;
  std::string request_path;
  std::string request_version;
  std::vector<std::string> request_header;
  std::unordered_map<std::string, std::string> request_header_kv;
  std::string request_content;
};

struct HttpResponse {

};

class EndPoint {
private:
  void ReadHttpRequestLine() {
    ReadLine(_sock, &_request.request_line);
    _request.request_line.resize(_request.request_line.size() - 1);
    //std::cout << _request.request_line << std::endl;
  }

  void ReadHttpRequestHeader() {
    std::string line;
    while(line != "\n") {
      line.clear();
      ReadLine(_sock, &line);
      if(line != "\n") {
        line.resize(line.size() - 1);
        _request.request_header.push_back(std::move(line));
      }
    }
    for(auto& e: _request.request_header) {
      std::cout << e << std::endl;
    }
  }

  void ParseHttpRequestLine() {
    std::stringstream ss;
    ss << _request.request_line;
    ss >> _request.request_method >> _request.request_path >> _request.request_version;
    //std::cout << _request.request_method << "=========" << _request.request_path << "============" << _request.request_version << std::endl;
  }
  void ParseHttpRequestHeader() {
    for(auto& header_line: _request.request_header) {
      std::string key;
      std::string value;
      size_t pos = header_line.find(": ");
      if(pos != std::string::npos) {
        key = header_line.substr(0, pos);
        value = header_line.substr(pos + 2);
        if(_request.request_header_kv.find(key) == _request.request_header_kv.end()) {
          _request.request_header_kv.insert(std::make_pair(key, value));
        }
        else {
          LOG(ERROR, "insert request header's hash map error");
        }
      }
      else {
        LOG(ERROR, "\': \' not found");
      }
    }
    //for(auto header_line: _request.request_header_kv) {
    //  std::cout << header_line.first << "===============" << header_line.second << std::endl;
    //}
  }
  
public:
  EndPoint(int sock): _sock(sock) {}
  void RecvHttpRequest() {
    ReadHttpRequestLine();
    ReadHttpRequestHeader();
  }

  void ParseHttpRequest() {
    ParseHttpRequestLine();
    ParseHttpRequestHeader();;
  }

  void BuildHttpResponse() {
    
  }

  void SendHttpResponse() {

  }

private:
  HttpRequest _request;
  HttpResponse _response;
  int _sock;
  std::unordered_map<std::string, std::string> _kv;
};

void* Handler(void* ptr_sock) {
  int sock = *(int*)ptr_sock;
  delete (int*)ptr_sock;

  EndPoint ep(sock);
  LOG(INFO, "receive http request success");
  ep.RecvHttpRequest();
  ep.ParseHttpRequest();
  ep.BuildHttpResponse();
  ep.SendHttpResponse();


  
  //char buffer[4096];
  //recv(sock, buffer, sizeof(buffer), 0);
  //std::cout << buffer << std::endl;
  // TODO
  
  close(sock);
  return nullptr;
}



} //namespace
