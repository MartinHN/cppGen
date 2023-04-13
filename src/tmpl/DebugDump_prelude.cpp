#include <iostream>

#define UAPI_HAS_DUMP 1

namespace reflect {
namespace debug {

constexpr bool includeType = false;
int indentStep = 4;

template <typename T> void dumpValue(const T &obj, int) { std::cout << obj; }

template <class T> struct is_vector : std::false_type {};
template <class T, class A>
struct is_vector<std::vector<T, A>> : std::true_type {};
template <class T, size_t A>
struct is_vector<std::array<T, A>> : std::true_type {};
template <typename T>
concept Vec = is_vector<T>::value;

template <Vec T> void dumpValue(const T &obj, int indent) {
  std::cout << "[ ";
  int i = 0;
  for (const auto &o : obj) {
    dumpValue(o, indent);
    if (i != obj.size() - 1)
      std::cout << ", ";
    i++;
  }
  std::cout << " ]";
}

} // namespace debug
} // namespace reflect
