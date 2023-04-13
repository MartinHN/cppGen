#include <iostream>
#include <sstream> //for string to stream convs
#include <type_traits>

#define UAPI_HAS_BINSERIALIZE 1

namespace reflect {
namespace serialize {

using OutStr = std::ostream;
using InStr = std::istream;

typedef uint32_t str_size_t;
typedef uint32_t vec_size_t;
bool checkStream(std::ios &s) {
  if (s.good())
    return true;
  std::cout << "ERR: !!!!!" << __FILE__ << std::endl;
#ifdef __aarch64__
  asm volatile("BRK 0");
#endif
  exit(1);
  return false;
}

template <class T> struct is_vector : std::false_type {};
template <class T, class A>
struct is_vector<std::vector<T, A>> : std::true_type {};
template <class T, size_t A>
struct is_vector<std::array<T, A>> : std::true_type {};
template <typename T>
concept Vec = is_vector<std::decay_t<T>>::value;

// Tuples

template <typename T>
concept TupleLike = requires(T a) {
                      std::tuple_size<T>::value;
                      std::get<0>(a);
                    };

// Strs
template <typename T>
concept Str = std::is_same_v<std::decay_t<T>, std::string>;

// BasicType
template <typename T>
concept BasicType = std::is_arithmetic_v<T>;

template <BasicType T> size_t true_size(T o) {
  static_assert(sizeof(std::decay_t<T>) >= sizeof(char));
  return sizeof(std::decay_t<T>);
}

template <TupleLike T> size_t true_size(T o) {
  size_t res = 0;
  std::apply([&res](auto &&...args) { ((res += true_size(args)), ...); }, o);
  return res;
}

template <Vec T> size_t true_size(const T &obj) {
  if (obj.size())
    return obj.size() * true_size(obj[0]);
  return 0;
}

template <Str T> size_t true_size(const T &obj) {
  if (obj.size())
    return obj.size() * true_size<char>(obj[0]);
  return 0;
}

template <typename T> void write_value(OutStr &os, const T &o) {
  if (checkStream(os))
    os.write((char *)&o, true_size<std::decay_t<T>>(o));
}

template <> void write_value<std::string>(OutStr &os, const std::string &o) {
  write_value<str_size_t>(os, o.length());
  for (const auto &e : o) {
    write_value<char>(os, e);
  }
}

template <Vec T> void write_value(OutStr &os, const T &o) {
  write_value<vec_size_t>(os, o.size());
  for (const auto &e : o)
    write_value(os, e);
}

/////////////////////////////
// from_bin
///////////////////////////

template <typename T> struct IsRef {
  static bool const result = false;
};
template <typename T> struct IsRef<T &> {
  static bool const result = true;
};

template <typename T> void parse_value(T &obj, InStr &is) {
  static_assert(sizeof(std::decay_t<T>) >= sizeof(char));
  checkStream(is);
  static_assert(IsRef<T>::result == false);
  is.read((char *)&obj, sizeof(std::decay_t<T>));
}

template <Vec T> struct can_resize : std::false_type {};
template <typename T> struct can_resize<std::vector<T>> : std::true_type {};
template <typename T> constexpr auto can_resize_v = can_resize<T>::value;

template <Vec T> void parse_value(T &obj, InStr &is) {
  vec_size_t sz = 0;
  parse_value(sz, is);
  if constexpr (can_resize_v<T>)
    obj.resize(sz);
  for (vec_size_t i = 0; i < sz; i++) {
    parse_value(obj[i], is);
  }
}

template <> void parse_value<std::string>(std::string &obj, InStr &is) {
  str_size_t sz = 0;
  parse_value<str_size_t>(sz, is);
  obj.resize(sz);
  for (str_size_t i = 0; i < sz; i++) {
    parse_value<char>(obj[i], is);
  }
}

template <typename T> void from_bin(T &obj, InStr &);

template <typename T> void from_bin_str(T &obj, char *str, size_t length) {
  std::istringstream iss;
  iss.rdbuf()->pubsetbuf(str, length);
  from_bin<T>(obj, iss);
}

} // namespace serialize
} // namespace reflect