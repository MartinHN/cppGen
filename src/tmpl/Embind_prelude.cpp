#pragma once
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

EM_JS(void, log_em_value, (emscripten::EM_VAL val_handle), {
  var value = Emval.toValue(val_handle);
  console.log("logObj", value);
});

static void logObj(const emscripten::val &v) { log_em_value(v.as_handle()); }

typedef emscripten::val MessagesRawType;
emscripten::val strToJs(const std::string &str) {
  return emscripten::val(emscripten::typed_memory_view(str.size(), str.data()));
}
template <typename T>
MessagesRawType buildMemberModMessage(T &api, std::string member);

template <typename T>
MessagesRawType buildMemberGetMessage(T &api, std::string member);

MessagesRawType buildGetRootStateMessageStr() {
  std::string modBuf;
  uapi::buildGetRootStateMessage(modBuf);
  return strToJs(modBuf);
}

// emscripten::val globalScope;
EMSCRIPTEN_BINDINGS(GlobalScope) {
  emscripten::function("buildGetRootStateMessage",
                       &buildGetRootStateMessageStr);
  emscripten::register_vector<char>("charVec");
};
