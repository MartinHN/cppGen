#pragma once
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

// clang-format off
EM_JS(void, log_em_value, (emscripten::EM_VAL val_handle), {
  var value = Emval.toValue(val_handle);
  console.log("logObj", value);
});

EM_JS(emscripten::EM_VAL, binMsgToHex_hdl, (emscripten::EM_VAL val_handle), {
  var value = Emval.toValue(val_handle);
  if (!value.buffer) {
    console.error("no buffer in bin message", value);
    return "";
  }
  var res = [...new Uint8Array(value.buffer)]
                  .map(x => x.toString(16).padStart(2, '0'))
                  .join("");
  return Emval.toHandle(res);
});
// clang-format on

static void logObj(const emscripten::val &v) { log_em_value(v.as_handle()); }
static std::string binMsgToHex(const emscripten::val &v) {
  auto rv = emscripten::val::take_ownership(binMsgToHex_hdl(v.as_handle()));
  return rv.as<std::string>();
}

typedef emscripten::val MessagesRawType;
emscripten::val strToJs(const std::string &str) {
  // returns array copy
  auto uArr = emscripten::val::global("Uint8Array");
  return uArr.new_(
      emscripten::typed_memory_view(str.size(), (uint8_t *)str.data()));
}
template <typename T>
MessagesRawType buildMemberModMessage(T &api, std::string member);

template <typename T>
MessagesRawType buildMemberGetMessage(T &api, std::string member);

template <typename T>
MessagesRawType buildGetRootStateMessageStr(T &api,
                                            const std::string &memberAddr);

// emscripten::val globalScope;
EMSCRIPTEN_BINDINGS(GlobalScope) {
  emscripten::function("binMsgToHex", binMsgToHex);
  emscripten::register_vector<char>("charVec");
};
