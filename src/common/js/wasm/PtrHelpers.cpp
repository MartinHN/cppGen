
#include <emscripten/bind.h>

namespace e = emscripten;

typedef uint64_t EncodedPtr;

struct OptEncodedPtr {
  OptEncodedPtr() : valid(false), ptr(0) {}
  OptEncodedPtr(EncodedPtr p) : valid(true), ptr(p) {}
  bool valid;
  EncodedPtr ptr;
};

EncodedPtr generateCodedPtrFromBuf(std::string &buf) {
  uint32_t len = buf.size();
  uint32_t ptr = reinterpret_cast<uint32_t>(buf.c_str());
  return ((uint64_t)len << 32) + ptr;
}

template <typename T>
OptEncodedPtr buildModMessage(const std::string &memberName, T i) {
  static std::string modBuf;
  bool isValid = uapi::buildModMessage(rootApi, memberName, i, modBuf);
  if (isValid)
    return generateCodedPtrFromBuf(modBuf);
  return {};
}

void dump() { uapi::debug::dump(rootApi); }

EMSCRIPTEN_BINDINGS(WasmHelpers) {

  e::class_<OptEncodedPtr>("OptEncodedPtr")
      .constructor()
      .property("valid", &OptEncodedPtr::valid)
      .property("ptr", &OptEncodedPtr::ptr);
  e::function("dump", &dump);
  e::function("buildIntModMessage", &buildModMessage<int>);
  e::function("buildFloatModMessage", &buildModMessage<float>);
  // e::function("processMessage", &processMessage);
}
