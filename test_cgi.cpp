#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <unordered_map>
bool CutString(const std::string& str, std::string* pkey, std::string* pvalue, const std::string& sep) {
  auto pos = str.find(sep);
  if(pos != str.npos) {
    (*pkey) = str.substr(0, pos);
    (*pvalue) = str.substr(pos + 1);
    return true;
  }
  else {
    std::cerr << "cut failed" << std::endl;
    return false;
  }
}
bool CutParamenter(std::unordered_map<std::string, std::string>* para_map, const std::string& paramenter) {
  std::vector<std::string> query_vector;
  size_t start = 0;
  while(true) {
    auto pos = paramenter.find("&", start);
    if(pos != paramenter.npos) {
      query_vector.push_back(paramenter.substr(start, pos - start));
      start = pos + 1;
    }
    else {
      query_vector.push_back(paramenter.substr(start));
      break;
    }
  }
  for(auto query: query_vector) {
    std::string key;
    std::string value;
    auto ret = CutString(query, &key, &value, "=");
    if(!ret) {
      std::cerr << "cannot find =" << std::endl;
      return false;
    }
    (*para_map).insert(std::make_pair(key, value));
  }
  return true;
}

int main()
{
  std::cerr << "main===========================" << std::endl;
  std::string method_env = getenv("METHOD");
  std::cerr << method_env << std::endl;

  if(method_env == "GET") {
    std::string paramenter_env = getenv("PARA");
    std::cerr << "paramenter: " << paramenter_env << std::endl;
    std::cerr << "&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
    std::unordered_map<std::string, std::string> query_map;
    CutParamenter(&query_map, paramenter_env);
    for(auto e: query_map) {
      std::cerr << "key: " << e.first << " " << "value: " << e.second << std::endl;
      std::cout << "key: " << e.first << " " << "value: " << e.second << std::endl;
    }
    std::cerr << "cut done" << std::endl;
  }

  else if(method_env == "POST") {
    std::cerr << "endvvvvv===========================" << std::endl;
    std::string request_body;
    std::string content_length_env = getenv("LENGTH");
    size_t length = atoi(content_length_env.c_str());
    std::cerr << "length  ====" << length << std::endl;
    for(size_t i = 0; i < length; i++) {
      char ch = '1';
      read(0, (void*)&ch, 1);
      std::cerr << ch << std::endl;
      request_body += ch;
      std::cerr << " read " << request_body << "enddd" << std::endl;
      sleep(1);
    }
    std::cerr << "body: " << request_body << std::endl;
  }
  else {
    // other method
  }
  return 0;
}
