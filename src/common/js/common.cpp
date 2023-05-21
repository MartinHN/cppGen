#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
static Dbg dbgObj("[jso]");
#define dbg dbgObj

struct JsObjWrapper {
  JsObjWrapper(const emscripten::val &o) : jsObj(o) {}

  template <typename RetT = void, typename... Args>
  void call(const char *s, Args... args) {
    auto methPtr = jsObj[std::string(s)];
    if (methPtr == emscripten::val::undefined()) {
      dbg.print("method not found", s);
      return;
    }
    // dbg.print("found", s);
    jsObj.call<RetT>(s, args...);
  }
  emscripten::val jsObj;
};

#undef dbg
