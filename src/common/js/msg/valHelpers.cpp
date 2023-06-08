
EM_JS(char *, getTypeOfRaw, (const emscripten::EM_VAL val_handle), {
  var value = Emval.toValue(val_handle);
  // console.log("getting type", typeof(value), value);
  return stringToNewUTF8(typeof(value));
});

static inline std::string getTypeOf(const emscripten::val &v) {
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
