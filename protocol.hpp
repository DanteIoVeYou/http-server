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
#define PAGE404PATH "wwwroot/404.html"

struct HttpRequest {
  HttpRequest(): request_paramenter_length(0), request_body_length(0), cgi(false) {}
  // 请求行
  std::string request_line;
  std::string request_method;
  std::string request_uri;
  std::string request_path;
  std::string request_paramenter;
  size_t request_paramenter_length;
  std::string request_version;
  // 请求报头
  std::vector<std::string> request_header;
  std::unordered_map<std::string, std::string> request_header_kv;
  // 请求体
  std::string request_body;
  size_t request_body_length;
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

  void ErrorHandler(int status_code) {
    if(status_code == NOT_FOUND) {
      _response.response_line += "HTTP/1.0";
      _response.response_line += " ";
      _response.response_line += std::to_string(NOT_FOUND);
      _response.response_line += " ";
      _response.response_line += _status_code_map[NOT_FOUND];
      _response.response_line += "\n";
    }
  }

  void ReadHttpRequestLine() {
    // 从套接字读取浏览器发来的http请求行
    ReadLine(_sock, &_request.request_line);
    _request.request_line.resize(_request.request_line.size() - 1);
    std::cout << _request.request_line << std::endl;
  }

  void ReadHttpRequestHeader() {
    // 读取请求报头
    std::string line;
    while(line != "\n") {
      line.clear();
      ReadLine(_sock, &line);
      if(line != "\n") {
        line.resize(line.size() - 1);
        _request.request_header.push_back(std::move(line));
      }
    }
  }

  void ParseHttpRequestLine() {
    // 处理请求行
    // 提取请求行中的请求方法 请求uri http版本号
    std::stringstream ss;
    ss << _request.request_line;
    ss >> _request.request_method >> _request.request_uri >> _request.request_version;
    // 把请求方法统一转为大写
    std::transform(_request.request_method.begin(), _request.request_method.end(), _request.request_method.begin(), ::toupper);
    // 判断请求方法是GET还是POST
    if(_request.request_method == "GET") {
      //判断GET是否带有参数
      if(CutString(_request.request_uri, &_request.request_path, &_request.request_paramenter, "?") == false) {
        _request.request_path = _request.request_uri;
      }
      else {
        _request.request_paramenter_length = _request.request_paramenter.size();
        _request.cgi = true;
      }
      LOG(DEBUG, _request.request_path.c_str());
    }
    else if(_request.request_method == "POST") {
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
  }
   
  void ReadHttpRequestBody() {
    // 根据请求方法是GET还是POST来判断是否有请求体
    if(_request.request_method == "POST") {
      // 读取请求体
      if(_request.request_header_kv.find("Content-Length") != _request.request_header_kv.end()) {
        _request.request_body_length = atoi(_request.request_header_kv["Content-Length"].c_str());
        ReadLine(_sock, &_request.request_body);
        // 去掉换行符
        _request.request_body.resize(_request.request_body.size() - 1);
        // 确认请求体读取正确
        if(_request.request_body.size() != _request.request_body_length) {
          std::cout << "body: " << _request.request_body.size() << " " << "length: " << _request.request_body_length << std::endl;
          LOG(ERROR, "read request body error");
        }
      }
    }
  }

  int NoCgiHttpResponse() {
    int status_code = NOT_FOUND;
    std::ifstream in(_request.request_uri.c_str(), std::ios::in | std::ios::binary);
    if(in.is_open()) {
      std::string line;
      while(std::getline(in, line)) {
        _response.response_body += line;
      }
      LOG(DEBUG, "read file success");
      status_code = OK;
    }
    else {
      LOG(ERROR, "open file failed");
      status_code = NOT_FOUND;
    }
    return status_code;
  }

  int CgiHttpResponse() {
    LOG(INFO, "Cgi start");
    int input[2];
    int output[2];
    if(pipe(input) < 0) {
      LOG(ERROR, "create pipe failed");
      return 500;
    }
    if(pipe(output) < 0) {
      LOG(ERROR, "create pipe failed");
      return 500;
    }
    pid_t pid = fork();
    if(pid < 0) {
      LOG(ERROR, "fork failed");
      return 500;
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
        content_length_env += std::to_string(_request.request_paramenter_length);
        LOG(DEBUG, ("cllllll:" + std::to_string(_request.request_paramenter_length)).c_str());
        putenv((char*)content_length_env.c_str());
      }
      else {
        // other method
      }
      // 重定向
      int dup_write = dup2(input[1], 1);
      int dup_read = dup2(output[0], 0);
      if(dup_write == -1 || dup_read == -1) {
        LOG(ERROR, "dup2 error");
        exit(500);
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
      //if(WIFSIGNALED(status)) {
      //  // 被信号中断
      //  return 500;
      //}
      char ch;
      while(read(input[0], &ch, 1) > 0) {
        _response.response_body += ch;
      }
    }
    return OK;
  }


  int HttpParseRequest() {
    // 寻找所请求的资源，服务器进行处理，并返回状态码，即处理结果
    LOG(DEBUG, _request.request_path.c_str());
    std::string resource_path;
    struct stat st;
    resource_path += ROOTPATH;
    resource_path += _request.request_path;
    int status_code = 0;
    if(_request.request_path.back() == '/') {
      // 1.请求根目录或者带/的目录，默认是请求其下的index.html
      resource_path += "index.html";
      LOG(DEBUG, resource_path.c_str());
    }
    if(stat(resource_path.c_str(), &st) == 0) {
      // 请求的资源存在
      if(S_ISDIR(st.st_mode)) {
        // 2.请求的是一个目录，不带/，默认是请求该目录下的index.html
        resource_path += "/index.html";
        LOG(DEBUG, resource_path.c_str());
        // 判断该目录下是否有html文件
        if(stat(resource_path.c_str(), &st) != 0) {
          // 找不到默认的html文件
          LOG(ERROR, "default index.html not found");
          status_code = NOT_FOUND;
        }
        else {
          // 找到了默认的html文件
          _request.request_uri = resource_path;
          // 服务器进行处理请求
          status_code = HttpHandler();
        }
      }
      else {
        // 3.请求的不是目录，是具体资源
        _request.request_uri = resource_path;
        // 服务器进行处理请求
        LOG(INFO, resource_path.c_str());
        status_code = HttpHandler();
      }
    }
    else {
      // 4. 请求的资源不存在
      status_code = NOT_FOUND;
    }
    return status_code;
  }

  int HttpHandler() {
    // 构建http响应
    // 根据请求方法进行不同的处理
    // 由于对于uri进行了处理，此时的uri可能与刚开始的uri不同，故文件的st需要重新获取
    struct stat st;
    stat(_request.request_uri.c_str(), &st);
    int status_code = 0;
    if(_request.request_method == "GET") {
      LOG(DEBUG, "GET METHOD");
      if(_request.request_paramenter.size() == 0) {
        // GET方法不带参数
        if((st.st_mode&S_IXUSR) || (st.st_mode&S_IXGRP) || (st.st_mode&S_IXOTH)) {
          status_code = CgiHttpResponse();
        }
        else {
          status_code = NoCgiHttpResponse();
        }
      }
      else {
        // GET方法带参数
        status_code = CgiHttpResponse();
      }
    }
    else if(_request.request_method == "POST") {
        status_code = CgiHttpResponse();
    }
    else {
      // other method
      status_code = NOT_FOUND;
    }
    return status_code;
  }

  void Build200OKPage() {
    // 响应行
    _response.response_line += "HTTP/1.0";
    _response.response_line += " ";
    _response.response_line += std::to_string(_response.status_code);
    _response.response_line += " ";
    _response.response_line += _status_code_map[_response.status_code];
    _response.response_line += "\n";
    // 响应报头
    if(_request.request_method == "GET") {
      if(_request.cgi == true) {
        // GET方法带参数
        _response.response_content_type = "Content-Type: ";
        _response.response_content_type += "text/html";
        _response.response_content_type += "\n";
        _response.response_content_length = "Content-Length: ";
        _response.response_content_length += std::to_string(_response.response_body.size());
        _response.response_content_length += "\n";
      }
      else {
        // GET方法不带参数
        _response.response_content_type = "Content-Type: ";
        _response.response_content_type += _suffix_type_map[GetSuffix(_request.request_uri)];
        _response.response_content_type += "\n";
        _response.response_content_length = "Content-Length: ";
        _response.response_content_length += std::to_string(_response.response_body.size());
        _response.response_content_length += "\n";
      }

    }
    else if(_request.request_method == "POST") {
        _response.response_content_type = "text/html";
        _response.response_content_type += "\n";

    }
    else {
      // nothing
    }
  }

  void Build404NotFoundPage() {
    // 响应体
    std::ifstream in(PAGE404PATH);
    std::string line;
    while(std::getline(in, line)) {
      _response.response_body += line;
    }
    // 响应行
    _response.response_line += "HTTP/1.0";
    _response.response_line += " ";
    _response.response_line += std::to_string(_response.status_code);
    _response.response_line += " ";
    _response.response_line += _status_code_map[_response.status_code];
    _response.response_line += "\n";
    // 响应报头
    _response.response_content_type = "Content-Type: ";
    _response.response_content_type += "text/html";
    _response.response_content_type += "\n";
    _response.response_content_length = "Content-Length: "; 
    _response.response_content_length += std::to_string(_response.response_body.size());
    _response.response_content_length += "\n";
    LOG(INFO, "404 Page");
  }

  void BuildErrorPage(int status_code) {
    switch(status_code) {
      case NOT_FOUND:
        Build404NotFoundPage();
        break;
      default:
        break;
    }
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
  EndPoint(int sock): _sock(sock), _status_code_map({{200, "OK"}, {404, "NOT FOUND"}}) {
    _suffix_type_map.insert({"html", "text/html"});
    _suffix_type_map.insert({"css", "text/css"});
  }
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
    _response.status_code = HttpParseRequest();
    if(_response.status_code == OK) {
      Build200OKPage();
    }
    else {
      BuildErrorPage(_response.status_code);
    }
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
  std::unordered_map<std::string, std::string> _suffix_type_map;
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