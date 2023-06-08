#include <iostream>

using uapi::traits::Vec;
#define UAPI_HAS_DUMP 1

namespace uapi {
namespace debug {

constexpr bool includeType = false;
int indentStep = 4;

template <typename T> void dumpValue(const T &obj, int) { std::cout << obj; }

template <Vec T> void dumpValue(const T &obj, int indent) {
  std::cout << "[ ";
  size_t i = 0;
  for (const auto &o : obj) {
    dumpValue(o, indent);
    if (i != obj.size() - 1)
      std::cout << ", ";
    i++;
  }
  std::cout << " ]";
}

} // namespace debug
} // namespace uapi
