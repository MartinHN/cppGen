#include "gen.h"
#include <fstream>
#include <iostream>
#include <sstream>
#ifndef ROOT_JS_CLASS
#error " ROOT_JS_CLASS should be defined"
#endif
// #define ROOT_JS_CLASS RootAPI
ROOT_JS_CLASS rootApi;
#include "common/js/js.cpp"

static Dbg dbgApp("[app]");
#define dbg dbgApp

// MessagesRawType buildMemberGetMessageMain(std::string n) {
//     return buildMemberGetMessage(rootApi, n);
// }
// MessagesRawType buildMemberModMessageMain(std::string n) {
//     return buildMemberModMessage(rootApi, n);
// }

void dumpAll() {
    uapi::debug::dump(rootApi);
}
// void abortMe() { abort(); }
void dumpPtr(ROOT_JS_CLASS &a) {
    uapi::debug::dump(a);
}

ROOT_JS_CLASS *getRootApi() {
    return &rootApi;
}
EMSCRIPTEN_BINDINGS(mainApp) {
    emscripten::function("getRootApi", &getRootApi,
                         emscripten::allow_raw_pointers());
    // emscripten::function("initWebSocket", &initWebSocket);
    // emscripten::function("buildMemberGetMessage",
    // &buildMemberGetMessageMain);
    // emscripten::function("buildMemberModMessage",
    // &buildMemberModMessageMain);
    emscripten::function("dump", &dumpAll);
    emscripten::function("dumpPtr", &dumpPtr);
    // emscripten::function("abort", &abortMe);
    // emscripten::function("test", &test, emscripten::allow_raw_pointers());
}

int main() {
    initJsBindRootApi(&rootApi);
#if TYPESCRIPT_DEF
    return 0;
#endif

    //
    // cliApi.name = "cli";
    // std::cout << "wasm init state is" << std::endl;
    // dump();
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
    //         uapi::serialize::to_bin(api,outF);
    //     }
    //     #endif
    // #if UAPI_HAS_DUMP
    //     uapi::debug::dump(api);
    // #endif

    // #if UAPI_HAS_IDS
    //     std::cout <<uapi::IDs::tryBuild("strVector")->shortId << std::endl;
    // #endif
    //     api.classVector.clear();
    //     api.strVector.clear();

    //     // api.intVector.push_back(111);
    //     // api.intVector.push_back(333);
    //     #if UAPI_HAS_BINSERIALIZE
    //     {
    //     std::cout <<"reading now " <<std::endl;
    //         auto inF = std::ifstream(tmpFile,std::ios_base::in |
    //         std::ios_base::binary); uapi::serialize::from_bin(api,inF);
    //     }
    // #endif
    // #if UAPI_HAS_DUMP
    //     uapi::debug::dump(api);
    // #endif

    // std::cout << "done , see " << tmpFile << std::endl;

    // return 0;
}

#undef dbg
