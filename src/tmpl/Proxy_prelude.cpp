#include <iostream>
#include <sstream> //for string to stream convs
#include <type_traits>

#define UAPI_HAS_PROXY 1

namespace reflect {
namespace proxy {

struct ProxySetter {};

struct MemberContainer {
  virtual void memberModifiedCb(const std::string &memberName,
                                variants::AnyMemberRefVar member,
                                ProxySetter *from) = 0;
};
template <typename T> struct MemberImpl {
  MemberImpl(const std::string &_name, T &_obj, MemberContainer &_container)
      : name(_name), obj(_obj), container(_container) {}
  const T &get() const { return obj; }
  void set(const T &o, ProxySetter *from = nullptr) {
    obj = o;
    notify(from);
  }
  void notify(ProxySetter *from) {
    container.memberModifiedCb(name, variants::AnyMemberRefVar(obj), from);
  }
  std::string name;
  T &obj;
  MemberContainer &container;
};

struct MethodCallInfo {
  template <typename T>
  MethodCallInfo(const std::string &fn, T argTuple)
      : funcName(fn), args(argTuple) {}
  virtual ~MethodCallInfo() = default;
  // virtual void to_bin(std::ostream &oss) = 0;
  void to_bin(std::ostream &oss) {
    reflect::serialize::write_value(oss, funcName);
    reflect::serialize::write_value(oss, args);
  }
  std::string funcName;
  variants::AnyMethodArgsValue args;
};

} // namespace proxy
} // namespace reflect