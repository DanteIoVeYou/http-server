#include "httpServer.hpp"
#include "utils.hpp" 
#include "log.hpp"
#include <fstream>
int main(int argc, char* argv[])
{
  if(argc != 2) {
    HttpServer::Manual(argv[0]);
    return 1;
  }
  HttpServer::HttpServer srv(atoi(argv[1]));
  srv.Loop();
  //std::string s = HttpServer::Time::GetCurrentTime();
  //std::cout << s << std::endl;
  //HttpServer::LOG(INFO, "hello");
  
  //std::ifstream s("wwwroot/index.html");
  //std::string line;
  //while(std::getline(s, line)) {
  //  std::cout << line << std::endl;
  //}
  return 0;
}
