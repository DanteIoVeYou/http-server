#pragma once 
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utils.hpp"
#include "log.hpp"

namespace HttpServer {

#define OK 200
#define NOT_FOUND 404
#define ROOTPATH "wwwroot"

struct HttpRequest {
  HttpRequest(): request_content_length(0), cgi(false) {}
  // 请求行
  std::string request_line;
  std::string request_method;
  std::string request_uri;
  std::string request_path;
  std::string request_paramenter;
  std::string request_version;
  // 请求报头
  std::vector<std::string> request_header;
  std::unordered_map<std::string, std::string> request_header_kv;
  size_t request_content_length;
  // 请求体
  std::string request_body;
  bool cgi;
};

struct HttpResponse {
  HttpResponse(): response_content_type("Content-Type: "), response_content_length("Content-Length: "), response_spaceline("\r\n") {}
  int status_code;
  std::string response_line;
  std::vector<std::string> response_header;
  std::string response_content_type;
  std::string response_content_length;
  std::string response_spaceline;
  std::string response_body;
};

class EndPoint {
private:

  void ErrorHandler() {
    _response.response_line += "HTTP/1.0";
    _response.response_line += " ";
    _response.response_line += std::to_string(NOT_FOUND);
    _response.response_line += " ";
    _response.response_line += _status_code_map[NOT_FOUND];
    _response.response_line += "\n";
  }

  void ReadHttpRequestLine() {
    ReadLine(_sock, &_request.request_line);
    _request.request_line.resize(_request.request_line.size() - 1);
    std::cout << _request.request_line << std::endl;
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
    //for(auto& e: _request.request_header) {
    //  std::cout << e << std::endl;
    //}
  }

  void ParseHttpRequestLine() {
    std::stringstream ss;
    ss << _request.request_line;
    ss >> _request.request_method >> _request.request_uri >> _request.request_version;
    //LOG(DEBUG, _request.request_line.c_str());
    
    // 把请求方法统一转为大写
    std::transform(_request.request_method.begin(), _request.request_method.end(), _request.request_method.begin(), ::toupper);
    //LOG(DEBUG, _request.request_method.c_str());
    // 判断请求方法是GET还是POST
    if(_request.request_method == "GET") {
      //LOG(DEBUG, _request.request_method.c_str());
      //判断GET是否带有参数
      if(CutString(_request.request_uri, &_request.request_path, &_request.request_paramenter, "?") == false) {
        _request.request_path = _request.request_uri;
      }
      else {
        _request.cgi = true;
      }
      LOG(DEBUG, _request.request_path.c_str());
    }
    else if(_request.request_method == "POST") {
      //LOG(DEBUG, _request.request_method.c_str());
      _request.cgi = true;
      _request.request_path = _request.request_uri;
    }
    else {
      // other request method
    }
  }

  void ParseHttpRequestHeader() {
    // 建立请求报头K/V关系
    for(auto& header_line: _request.request_header) {
      std::string key;
      std::string value;
      CutString(header_line, &key, &value, ": ");
      if(_request.request_header_kv.find(key) == _request.request_header_kv.end()) {
        _request.request_header_kv.insert(std::make_pair(key, value));
      }
      else {
        LOG(ERROR, "insert request header's hash map error");
      }
    }
    //for(auto header_line: _request.request_header_kv) {
    //  std::cout << header_line.first << "===============" << header_line.second << std::endl;
    //}
  }
   
  void ReadHttpRequestBody() {
    // 根据请求方法是GET还是POST来判断是否有请求体
    if(_request.request_method == "POST") {
      // 读取请求体
      if(_request.request_header_kv.find("Content-Length") != _request.request_header_kv.end()) {
        _request.request_content_length = atoi(_request.request_header_kv["Content-Length"].c_str());
        ReadLine(_sock, &_request.request_body);
        //  去掉换行
        _request.request_body.resize(_request.request_body.size() - 1);
        if(_request.request_body.size() != _request.request_content_length) {
          std::cout << "body: " << _request.request_body.size() << " " << "length: " << _request.request_content_length << std::endl;
          LOG(ERROR, "read request body error");
        }
      }
    }
  }

  
  int NoCgiHttpResponse() {
    std::ifstream in(_request.request_uri.c_str(), std::ios::in | std::ios::binary);
    if(in.is_open()) {
      std::string line;
      while(std::getline(in, line)) {
        _response.response_body += line;
      }
      LOG(DEBUG, "read file success");
      return 200;
    }
    else {
      LOG(ERROR, "open file failed");
      ErrorHandler();
      return 404;
    }
  }

  void CgiHttpResponse() {
    LOG(INFO, "Cgi start");
    int input[2];
    int output[2];
    if(pipe(input) < 0) {
      LOG(ERROR, "create pipe failed");
      ErrorHandler();
    }
    if(pipe(output) < 0) {
      LOG(ERROR, "create pipe failed");
      ErrorHandler();
    }
    pid_t pid = fork();
    if(pid < 0) {
      LOG(ERROR, "fork failed");
      ErrorHandler();
    }
    else if(pid == 0) {
      // child
      close(input[0]);
      close(output[1]);
      // 将请求方法导入环境变量
      std::string method_env = "METHOD=";
      method_env += _request.request_method;
      LOG(DEBUG, _request.request_method.c_str());
      putenv((char*)method_env.c_str());
      // GET的参数以环境变量导入，POST的参数以匿名管道的方式传递
      if(_request.request_method == "GET") {
        std::string paramenter_env = "PARA=";
        paramenter_env += _request.request_paramenter;
        putenv((char*)paramenter_env.c_str());
        LOG(INFO, paramenter_env.c_str());
        LOG(INFO, "child GET METHOD");
      }
      else if(_request.request_method == "POST") {
        LOG(INFO, "child POST METHOD");
        std::string content_length_env = "LENGTH=";
        content_length_env += std::to_string(_request.request_content_length);
        LOG(DEBUG, ("cllllll:" + std::to_string(_request.request_content_length)).c_str());
        putenv((char*)content_length_env.c_str());

        //char* start = (char*)_request.request_body.c_str();
        //for(size_t i = 0; i < _request.request_body.size(); i++) {
        //  write(input[1], start + i, 1);
        //  std::cout << "write: " << *(start+i) << std::endl;
        //}
      }
      else {
        // other method
      }
      // 重定向
      int dup_write = dup2(input[1], 0);
      int dup_read = dup2(output[0], 1);
      if(dup_write == -1 || dup_read == -1) {
        LOG(ERROR, "dup2 error");
        ErrorHandler();
      }
      execl(_request.request_uri.c_str(), _request.request_uri.c_str(), nullptr);
    }
    else {
      // father
      close(input[1]);
      close(output[0]);
      if(_request.request_method == "POST") {
        char* start = (char*)_request.request_body.c_str();
        for(size_t i = 0; i < _request.request_body.size(); i++) {
          write(output[1], start + i, 1);
          std::cout << "write: " << *(start+i) << std::endl;
        }
      }
      int status;
      waitpid(pid, &status, 0);
      status = WIFEXITED(status);
      std::string info = "wait success, status code: ";
      info += std::to_string(status);
      LOG(INFO, info.c_str());
      if(WIFSIGNALED(status)) {
        // 被信号中断
        ErrorHandler();
      }
    }
  }

  void BuildHttpResponseLine() {
    LOG(DEBUG, _request.request_path.c_str());
    std::string resource_path;
    struct stat st;
    resource_path += ROOTPATH;
    resource_path += _request.request_path;
    if(_request.request_path.back() == '/') {
      // 1.请求根目录或者带/的目录下的index.html
      resource_path += "index.html";
      LOG(DEBUG, resource_path.c_str());
    }
    else {
      LOG(DEBUG, resource_path.c_str());
      if(stat(resource_path.c_str(), &st) == 0) {
        if(S_ISDIR(st.st_mode)) {
          // 2.请求的是一个目录，不带/，默认是请求该目录下的index.html
          resource_path += "/index.html";
          LOG(DEBUG, resource_path.c_str());
        }
        else {
          // 3.请求的不是目录，是具体资源
          LOG(INFO, resource_path.c_str());
        }
      }
      else {
        // 4. 请求的资源不存在
        ErrorHandler();
        return;
      }
    }
    // 构建http响应
    // 根据请求方法进行不同的处理
    // 由于对于uri进行了处理，此时的uri可能与刚开始的uri不同，故文件的st需要重新获取
    _request.request_uri = resource_path;
    stat(_request.request_uri.c_str(), &st);
    if(_request.request_method == "GET") {
      LOG(DEBUG, "GET METHOD");
      if(_request.request_paramenter.size() == 0) {
        // GET方法不带参数
        if((st.st_mode&S_IXUSR) || (st.st_mode&S_IXGRP) || (st.st_mode&S_IXOTH)) {
          CgiHttpResponse();
        }
        else {
          _response.status_code = NoCgiHttpResponse();
          _response.response_line += "HTTP/1.0";
          _response.response_line += " ";
          _response.response_line += std::to_string(_response.status_code);
          _response.response_line += " ";
          _response.response_line += _status_code_map[_response.status_code];
          _response.response_line += "\n";
        }
      }
      else {
        // GET方法带参数
        CgiHttpResponse();
      }
    }
    else if(_request.request_method == "POST") {
        CgiHttpResponse();
    }
    else {
      // other method
      ErrorHandler();
      return;
    }
  }


  void BuildHttpResponseHeader() {
    _response.response_content_type += "text/html";
    _response.response_content_type += "\n";
    _response.response_content_length += std::to_string(_response.response_body.size());
    _response.response_content_length += "\n";
  }

  void SendRespToBuffer() {
    send(_sock, _response.response_line.c_str(), _response.response_line.size(), 0);
    send(_sock, _response.response_content_type.c_str(), _response.response_content_type.size(), 0);
    send(_sock, _response.response_content_length.c_str(), _response.response_content_length.size(), 0);
    send(_sock, _response.response_spaceline.c_str(), _response.response_spaceline.size(), 0);
    send(_sock, _response.response_body.c_str(), _response.response_body.size(), 0);
    LOG(INFO, "send response success");
  }
  
  
public:
  EndPoint(int sock): _sock(sock), _status_code_map({{200, "OK"}, {404, "NOT FOUND"}}) {}
  void RecvHttpRequest() {
    ReadHttpRequestLine();
    ReadHttpRequestHeader();
  }

  void ParseHttpRequest() {
    ParseHttpRequestLine();
    ParseHttpRequestHeader();;
    ReadHttpRequestBody();
  }

  void BuildHttpResponse() {
    BuildHttpResponseLine();
    BuildHttpResponseHeader();
    
  }

  void SendHttpResponse() {
    SendRespToBuffer();
  }

private:
  HttpRequest _request;
  HttpResponse _response;
  int _sock;
  std::unordered_map<std::string, std::string> _kv;
  std::unordered_map<int, std::string> _status_code_map;
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
