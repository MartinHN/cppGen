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