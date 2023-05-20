#include "../../../gen/gen.h"

#include <fstream>
#include <iostream>
#include <sstream>
RootAPI cliApi;

#include <emscripten/bind.h>

namespace e = emscripten;

void loadBin(uint32_t input, uint32_t length) {
  char *str = reinterpret_cast<char *>(input);
  reflect::serialize::from_bin_str(cliApi, str, length);
}

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

EncodedPtr getBin() {
  static std::string outBuf;
  reflect::serialize::to_bin_str_RootAPI(cliApi, outBuf);
  return generateCodedPtrFromBuf(outBuf);
}

template <typename T>
OptEncodedPtr buildModMessage(const std::string &memberName, T i) {
  static std::string modBuf;
  bool isValid = reflect::buildModMessage(cliApi, memberName, i, modBuf);
  if (isValid)
    return generateCodedPtrFromBuf(modBuf);
  return {};
}

// bool processMessage(std::string s) {
//   std::string respBuf;
//   return reflect::processMessage(cliApi, s.data(), s.size(), respBuf);
// }

void dump() { reflect::debug::dump(cliApi); }

EMSCRIPTEN_BINDINGS(test) {
  // e::class_<RootAPI>("RootAPI").constructor()
  //   .function("simpleAction", &RootAPI::simpleAction)
  ;

  e::class_<OptEncodedPtr>("OptEncodedPtr")
      .constructor()
      .property("valid", &OptEncodedPtr::valid)
      .property("ptr", &OptEncodedPtr::ptr);
  e::function("loadBin", &loadBin);
  e::function("getBin", &getBin);
  e::function("dump", &dump);
  e::function("buildIntModMessage", &buildModMessage<int>);
  e::function("buildFloatModMessage", &buildModMessage<float>);
  // e::function("processMessage", &processMessage);
}

int main() {
  // cliApi.name = "cli";
  std::cout << "wasm init state is" << std::endl;
  dump();
  //     auto tmpFile = "/tmp/lala.bin";
  //     api.intVector.push_back(1);
  //     api.intVector.push_back(3);
  //     api.classVector.push_back({1});
  //     api.strVector.push_back("lllll");
  //     api.strVector.push_back("aaaa");

  //     #if UAPI_HAS_BINSERIALIZE
  //     {
  //         std::cout <<"writing now " <<std::endl;
  //         auto outF = std::ofstream(tmpFile);
  //         reflect::serialize::to_bin(api,outF);
  //     }
  //     #endif
  // #if UAPI_HAS_DUMP
  //     reflect::debug::dump(api);
  // #endif

  // #if UAPI_HAS_IDS
  //     std::cout <<reflect::IDs::tryBuild("strVector")->shortId << std::endl;
  // #endif
  //     api.classVector.clear();
  //     api.strVector.clear();

  //     // api.intVector.push_back(111);
  //     // api.intVector.push_back(333);
  //     #if UAPI_HAS_BINSERIALIZE
  //     {
  //     std::cout <<"reading now " <<std::endl;
  //         auto inF = std::ifstream(tmpFile,std::ios_base::in |
  //         std::ios_base::binary); reflect::serialize::from_bin(api,inF);
  //     }
  // #endif
  // #if UAPI_HAS_DUMP
  //     reflect::debug::dump(api);
  // #endif

  // std::cout << "done , see " << tmpFile << std::endl;

  // return 0;
}
