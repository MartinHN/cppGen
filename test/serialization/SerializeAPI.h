#include <string>
#include <vector>

struct IMember {
  int i;
};
struct RootAPI {
  int a = 0;
  unsigned int b = 0;
  float c = 0;
  double d = 0;
  short l = 0;
  std::string string = {};
  std::vector<int> vec = {};
  std::vector<IMember> vecM = {};
  int lastInt = 0;
};
