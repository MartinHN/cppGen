#include <iostream>
#include <string>
#include <vector>

struct IMember {
  int i = 888;
  void funcM(int m) { i = m; }
};

struct RootAPI {
  RootAPI(){};
  RootAPI(const RootAPI &) = delete;
  int a = 0;
  unsigned int b = 0;
  float c = 0;
  double d = 0;
  short l = 0;
  std::string string = {};
  std::vector<int> vec = {};
  std::vector<IMember> vecM = {};

  IMember member;
  int lastInt = 0;

  void func(int a) {
    // std::cout << "calling func with value " << a << std::endl;
    functionValue = a;
  }

  int funcR(int a) { return a; }
  int functionValue = 0;
};
