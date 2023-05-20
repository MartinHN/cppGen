
#include <iostream>
#include <sstream> //for string to stream convs
#include <type_traits>

#define UAPI_HAS_PROXY 1

namespace uapi {
namespace proxy {

struct ProxySetter {};
// forward declarations
struct MethodCallInfo;

struct MemberContainer {

  virtual ~MemberContainer() = default;
  // callbacks
  std::function<void(const std::string &name, variants::AnyMemberRefVar member,
                     ProxySetter *from)>
      onMemberChange;
  std::function<void(const std::string &name, variants::AnyMemberRefVar member,
                     ProxySetter *from)>
      onMemberGet;
  std::function<void(MethodCallInfo *method, ProxySetter *from)> onMethodCall;

  virtual void memberModifiedCb(const std::string &memberName,
                                variants::AnyMemberRefVar memberVal,
                                ProxySetter *from) {
    if (onMemberChange) {
      onMemberChange(memberName, memberVal, from);
    }
  };

  virtual void memberGetCb(const std::string &memberName,
                           variants::AnyMemberRefVar memberVal,
                           ProxySetter *from) {
    if (onMemberGet) {
      onMemberGet(memberName, memberVal, from);
    }
  };

  virtual void methodCalledCb(MethodCallInfo *callInfo, ProxySetter *from) {
    if (onMethodCall) {
      onMethodCall(callInfo, from);
    }
  }
};

// MemberImpl

template <typename T> struct MemberImpl {
  MemberImpl(const std::string &_name, T &_obj, MemberContainer &_container)
      : name(_name), obj(_obj), container(_container) {}
  const T &get(ProxySetter *from = nullptr) const {
    container.memberGetCb(name, variants::AnyMemberRefVar(obj), from);
    return obj;
  }

  void set(const T &o, ProxySetter *from = nullptr) {
    obj = o;
    notifyChange(from);
  }

  void notifyChange(ProxySetter *from = nullptr) {
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
    uapi::serialize::write_value(oss, funcName);
    uapi::serialize::write_value(oss, args);
  }
  std::string funcName;
  variants::AnyMethodArgsTuple args;
};

} // namespace proxy
} // namespace uapi
