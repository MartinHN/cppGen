// clang-format off
EM_JS(void, log_em_value, (emscripten::EM_VAL val_handle, emscripten::EM_VAL prefixStr), {
  var value = Emval.toValue(val_handle);
  var prefix = Emval.toValue(prefixStr);
  console.log("logJsObj",prefix,value);
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

static void logJsObj(const emscripten::val &v, const std::string &prefix = "") {
  emscripten::val pref(prefix);
  log_em_value(v.as_handle(), pref.as_handle());
}

static std::string binMsgToHex(const emscripten::val &v) {
  auto rv = emscripten::val::take_ownership(binMsgToHex_hdl(v.as_handle()));
  return rv.as<std::string>();
}
