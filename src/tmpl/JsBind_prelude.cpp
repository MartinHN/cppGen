#pragma once
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
static Dbg dbgJs("[jsb]");
#define dbg dbgJs

RootAPI *jsBindCppObj = nullptr;

void initJsBindRootApi(RootAPI *api) { jsBindCppObj = api; }

EM_JS(char *, getTypeOfRaw, (const emscripten::EM_VAL val_handle), {
  var value = Emval.toValue(val_handle);
  console.log("getting type", typeof(value), value);
  return stringToNewUTF8(typeof(value));
});

static std::string getTypeOf(const emscripten::val &v) {
  auto jsStr = getTypeOfRaw(v.as_handle());
  std::string res(jsStr);
  free(jsStr);
  return res;
}

namespace e = emscripten;
using uapi::variants::AnyMemberRefVar;
using uapi::variants::AnyMethodArgsTuple;
using uapi::variants::AnyMethodReturnValue;

e::val anyMemberToVal(AnyMemberRefVar member) {
  e::val res = e::val::undefined();
  std::visit(
      [&res](auto &&m) {
        using ArgT = std::decay_t<decltype(m)>;
        using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
        res = e::val(TRUT(m));
      },
      member);

  return res;
};

AnyMemberRefVar valToAnyMember(e::val v) {
  // auto t = v.typeof();
  // TODO
  int a = 0;
  return AnyMemberRefVar(a);
};

template <typename T> T getValAs(const e::val &v) { return v.as<T>(); }

template <uapi::traits::Vec T> T getValAs(const e::val &v) {
  using innerT = uapi::traits::inner_type<T>::type;
  dbg.print("vectype", cppTypeOf<innerT>());
  return e::vecFromJSArray<innerT>(v);
}

struct NestedLocker {

  NestedLocker(e::val e) : jsO(e) {}
  void lock() { jsO.set("__fromServer", true); };
  void unlock() { jsO.set("__fromServer", false); };
  e::val jsO;
  uint32_t count = 0;
};

struct NestedCounter {
  NestedCounter(NestedLocker &c) : locker(c) {
    if (locker.count <= 0)
      locker.lock();
    locker.count++;
  }
  ~NestedCounter() {
    locker.count--;
    if (locker.count <= 0)
      locker.unlock();
  }
  NestedLocker &locker;
};

struct JsBindMessageProcessorHandler : public uapi::MessageProcessorHandler {
  JsBindMessageProcessorHandler(e::val g) : jsO(g), locker(g) {}

  void onMemberSet(const std::string &addr, AnyMemberRefVar &v) override {
    dbg.print("setting member::", addr);
    NestedCounter c(locker);
    logObj(jsO);
    auto addrVec = uapi::variants::strAddrFromStr(addr);
    auto lastM = addrVec.back();
    addrVec.pop_back();
    auto targetO = jsO;
    for (auto &a : addrVec) {
      targetO = targetO[a];
    }
    targetO.set(lastM, anyMemberToVal(v));
    logObj(jsO);
    if (nextH)
      nextH->onMemberSet(addr, v);
  }
  void onMemberGet(const std::string &addr, AnyMemberRefVar &v) override {
    // TODO
    if (nextH)
      nextH->onMemberGet(addr, v);
  }

  template <typename T> void applyOnJs(e::val j, T &c) {
    if constexpr (!(uapi::variants::isUserDefined<T>::value || Vec<T>)) {
      dbg.print("!!! applyOnJs , unsupported type", cppTypeOf<T>);
      return;
    }
    for (int i = 0; i < uapi::variants::getNumMembers<T>(c); i++) {
      auto m = uapi::variants::getMemberWithIdx(c, i);
      if (!m) {
        dbg.print("!!! err , no member found");
        continue;
      }
      auto mName = uapi::variants::getMemberNameForIdx<T>(i);
      std::visit(
          [this, &j, &mName](auto &&ma) {
            using ArgT = std::decay_t<decltype(ma)>;
            using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
            if constexpr (uapi::variants::isUserDefined<TRUT>::value ||
                          Vec<TRUT>) {
              dbg.print("entering from rootstate : ", mName);
              applyOnJs<TRUT>(j[mName], ma);
            } else {
              dbg.print("setting from rootstate : ", mName, TRUT(ma));
              jsO.set(mName, TRUT(ma));
            }
          },
          *m);
    }
  }
  void onRootStateSet() override {
    NestedCounter c(locker);
    applyOnJs(jsO, *jsBindCppObj);
    // TODO
    if (nextH)
      nextH->onRootStateSet();
  }
  void onRootStateGet() override {
    // TODO
    if (nextH)
      nextH->onRootStateGet();
  }
  void onFunctionCall(const std::string &name, AnyMethodArgsTuple &args,
                      AnyMethodReturnValue &res) override {

    // TODO
    if (nextH)
      nextH->onFunctionCall(name, args, res);
  }
  // the only unimplemented functino, it's app reponsability to know what to do
  void onFunctionResp(const std::string &name,
                      AnyMethodReturnValue &res) override {
    // TODO
    if (nextH)
      nextH->onFunctionResp(name, res);
  }
  e::val jsO;
  NestedLocker locker;
  MessageProcessorHandler *nextH = nullptr;
};

e::val buildJsBindModMessage(const std::string &jsAddr, const e::val &val) {
  std::string modBuf;
  auto &parentNode = *jsBindCppObj;
  auto aStr = uapi::variants::strAddrFromStr(jsAddr);
  auto intAddr = uapi::variants::addressStrToInt(parentNode, aStr);
  if (!intAddr) {
    dbg.print("no member found for ", jsAddr);
    return {};
  }
  auto member = uapi::variants::getMemberWithAddressInt(parentNode, *intAddr);
  if (!member) {
    dbg.print("no member found for ", jsAddr);
    return {};
  }

  bool isValid = false;
  std::visit(
      [&isValid, &modBuf, &jsAddr, &val, &parentNode](auto &&m) {
        using MT = std::decay_t<decltype(m)>;
        using TT = uapi::traits::unwrap_ref<MT>::type;
        auto vv = getValAs<TT>(val);
        dbg.print("visited", cppTypeOf<TT>);
        // if constexpr (std::is_arithmetic_v<TT>)
        isValid = uapi::buildModMessage(parentNode, jsAddr, vv, modBuf);
      },
      *member);
  if (!isValid) {
    dbg.print("not valid");
    return strToJs("");
  }

  dbg.print("valid");
  // // apply to internal value
  // std::string fkBuf;
  // uapi::processMessage(jsBindCppObj, modBuf.data(), modBuf.size(), fkBuf,
  //                         nullptr);
  return strToJs(modBuf);
}

template <typename T> bool parseOneJsMethodArg(const e::val &jsArg, T &cppArg) {
  cppArg = jsArg.as<T>();
  getTypeOf(jsArg);
  return true;
  // auto type = getTypeOf(jsArg);
  // if (type == "number") {
  // }
  // dbg.print("arg type not supported", type);
  // return {};
}
bool parseJsMethodArgs(const e::val &jsArgs,
                       uapi::variants::AnyMethodArgsTuple &toFill) {
  int jsSize = jsArgs["length"].as<int>();
  int cppSize = -1;
  std::visit(
      [&cppSize](auto &&a) {
        using TRUT = std::decay_t<decltype(a)>;
        if constexpr (std::is_same_v<uapi::variants::VoidArgs, TRUT>)
          cppSize = 0;
        else
          cppSize = std::tuple_size<TRUT>();
      },
      toFill);
  if (cppSize < 0) {
    dbg.print("big bug");
    return false;
  }
  if (cppSize != jsSize) {
    dbg.print("wrong number of args : ", jsSize, "but expected", cppSize);
    return false;
  }
  bool res = true;
  int i = 0;
  std::visit(
      [&jsArgs, &res, &i](auto &&a) {
        using TRUT = std::decay_t<decltype(a)>;
        if constexpr (!std::is_same_v<TRUT, uapi::variants::VoidArgs>) {
          std::apply(
              [&jsArgs, &res, &i](auto &&...cppArg) {
                ((res &=
                  parseOneJsMethodArg(jsArgs[std::to_string(i++)], cppArg)),
                 ...);
              },
              a);
        }
      },
      toFill);

  return true;
}

e::val buildJsBindCallMessage(const std::string &nodeAddr,
                              const std::string &methodName,
                              const e::val &args) {
  std::string modBuf;
  auto &parentNode = *jsBindCppObj;

  auto *method =
      uapi::variants::getMethodWithName(parentNode, methodName.c_str());
  if (!method) {
    dbg.print("not valid");
    return strToJs("");
  }

  auto argsToFill = method->getExpectedArgsTuple();
  bool isValid = parseJsMethodArgs(args, argsToFill);
  if (!isValid) {
    dbg.print("js args not valid");
    return strToJs("");
  }
  isValid = uapi::buildCallMessage(parentNode, nodeAddr, methodName, argsToFill,
                                   modBuf);

  if (!isValid) {
    dbg.print("call message not valid");
    return strToJs("");
  }

  dbg.print("valid");
  // // apply to internal value
  // std::string fkBuf;
  // uapi::processMessage(jsBindCppObj, modBuf.data(), modBuf.size(), fkBuf,
  //                         nullptr);
  return strToJs(modBuf);
}

EMSCRIPTEN_BINDINGS(JsBindScope) {
  // e::function("initForJsObj", &initForJsObj);
  e::function("buildJsBindModMessage", &buildJsBindModMessage);
  e::function("buildJsBindCallMessage", &buildJsBindCallMessage);

  // e::function("processMsgForJs", &processMsgForJs);
};

#undef dbg
