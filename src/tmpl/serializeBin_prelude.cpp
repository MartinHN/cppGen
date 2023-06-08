#include <iostream>
#include <sstream> //for string to stream convs
#include <type_traits>

#define UAPI_HAS_BINSERIALIZE 1

using namespace uapi::traits;
namespace uapi {
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

// VarInt
template <typename T>
concept VarInt = std::is_unsigned_v<T> && sizeof(T) > 1;
typedef uint8_t VarIntAtomic;

template <typename T>
concept SVarInt = std::is_integral_v<T> && !
VarInt<T> && sizeof(T) > 1;
template <SVarInt T> uint64_t SVarIntToU(T v) {
  return v < 0 ? (-2 * v + 1) : 2 * v;
}
template <SVarInt T> T UToSVarInt(uint64_t v) {
  return v % 2 == 1 ? -(v - 1) / 2 : v / 2;
}

// BasicType
template <typename T>
concept BasicType = std::is_arithmetic_v<T>;

template <BasicType T> size_t true_size(T o) {
  static_assert(sizeof(std::decay_t<T>) >= sizeof(char));
  return sizeof(std::decay_t<T>);
}

template <VarInt T> size_t true_size(T o) {
  int ms = 1;
  while (o > 127) {
    o >>= 7;
    ms++;
  }
  return ms;
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
  if (checkStream(os)) {
    os.write((char *)&o, true_size<std::decay_t<T>>(o));
    //   size_t max = true_size<std::decay_t<T>>(o);
    //   for (size_t k = 0; k < max; k++) {
    //     auto i = getEndianIndex(k, max);
    //     os.write(((char *)&o)[k], 1);
    //   }
  }
}

template <VarInt T> void write_value(OutStr &os, const T &o) {
  if (checkStream(os)) {
    VarIntAtomic output[sizeof(T)];
    size_t outputSize = 0;
    uint64_t value = o;
    output[outputSize] = VarIntAtomic(value & 127);
    outputSize++;
    while (value > 127) {
      //|128: Set the next byte flag
      value >>= 7;
      output[outputSize] = VarIntAtomic(value & 127) | 128;

      // Remove the seven bits we just wrote
      outputSize++;
    }
    for (int i = outputSize - 1; i >= 0; i--) {
      // std::cout << "writing : ";
      // printf("%.2X\n", output[i]);
      write_value<VarIntAtomic>(os, output[i]);
    }
  }
}

template <SVarInt T> void write_value(OutStr &os, const T &o) {
  if (checkStream(os)) {
    write_value(os, SVarIntToU(o));
  }
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

template <TupleLike T> void write_value(OutStr &os, const T &o) {
  std::apply([&os](auto &&...args) { ((write_value(os, args)), ...); }, o);
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
  static_assert(sizeof(std::decay_t<T>) == sizeof(char) || BasicType<T>);
  static_assert(IsRef<T>::result == false);
  if (checkStream(is)) {
    is.read((char *)&obj, sizeof(std::decay_t<T>));
    // size_t max = sizeof<std::decay_t<T>>(o);
    // for (size_t k = 0; k < max; k++) {
    //   auto i = getEndianIndex(k, max);
    //   is.read(((char *)&o)[k], 1);
    // }
  }
}

template <VarInt T> void parse_value(T &obj, InStr &is) {
  VarIntAtomic input;
  parse_value<VarIntAtomic>(input, is);
  obj = 0;
  // std::cout << "parse VarInt : ";
  // printf("%.2X\n", input);
  while (input & 128) {
    // std::cout << "  slhifting : ";
    obj |= (input & 127);
    obj <<= 7;
    // printf(" %.2X : ", input);
    // printf(" %.2llX \n", uint64_t(obj));
    // If the next-byte flag is set
    parse_value<VarIntAtomic>(input, is);
  }
  obj |= input;
  // std::cout << "  lastB : ";
  // printf(" %.2X : ", input);
  // printf(" %.2llX \n", uint64_t(obj));
}

template <SVarInt T> void parse_value(T &obj, InStr &is) {
  uint64_t zz;
  parse_value(zz, is);
  obj = UToSVarInt<T>(zz);
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

template <TupleLike T> void parse_value(T &obj, InStr &is) {
  std::apply([&is](auto &&...args) { ((parse_value(args, is)), ...); }, obj);
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
} // namespace uapi
