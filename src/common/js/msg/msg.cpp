
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

static Dbg dbgJs("[jsb]");
#define dbg dbgJs

#include "logHelpers.cpp"
//
#include "valHelpers.cpp"
//
#include "builders.cpp"
//
#include "handlers.cpp"

#ifndef ROOT_JS_CLASS
#error ROOT_JS_CLASS should be defind
#endif

static void initWebSocket(ROOT_JS_CLASS *rootApiObj, int port,
                          const emscripten::val &connHdlr,
                          const emscripten::val &msgHdlr,
                          const emscripten::val &store) {
  if (!rootApiObj) {
    std::cerr << "root class ptr should be passed in initWebSocket"
              << std::endl;
    return;
  }

  // TODO remove following memory leaks
  auto *conHdl = new JsConnHandlerWrapper(connHdlr);
  auto *wasmToJsHdlr =
      new WasmToJsObjHandler<ROOT_JS_CLASS>(*rootApiObj, store);
  wasmToJsHdlr->nextH = new JsMessageHandlerWrapper(msgHdlr);

  init_websocket(*rootApiObj, port, *conHdl, *wasmToJsHdlr);
}

struct JsWasmTransport : public uapi::TransportImpl<ROOT_JS_CLASS> {
  WasmToJsObjHandler<ROOT_JS_CLASS> transpHdlr;
  JsMessageHandlerWrapper jsMessageCallback;
  JsWasmTransport(ROOT_JS_CLASS *rootApiObj, const emscripten::val &jsMsgHdlr,
                  const emscripten::val &store)
      : uapi::TransportImpl<ROOT_JS_CLASS>(rootApiObj),
        transpHdlr(*rootApiObj, store), jsMessageCallback(jsMsgHdlr) {
    if (!rootApiObj) {
      std::cerr << "root class ptr should be passed in initWebSocket"
                << std::endl;
      return;
    }
    // IMPL_JS_BUILDER(MainBuilder, *rootApiObj);

    transpHdlr.nextH = &jsMessageCallback;
    transportMsgHdlr = &transpHdlr;
  }

  virtual ~JsWasmTransport() override = default;

  emscripten::val processMsgStr(std::string binMsg) {
    std::string resp;
    bool needResp = processMsg(binMsg.data(), binMsg.size(), resp);
    if (needResp)
      return strToU8ArrJs(resp);
    return {};
  }
};

DECLARE_JS_BUILDER(MainBuilder, ROOT_JS_CLASS)
EMSCRIPTEN_BINDINGS(JsBindScope) {
    // e::function("initForJsObj", &initForJsObj);
  emscripten::function("initWebSocket", &initWebSocket,
                       emscripten::allow_raw_pointers());
  emscripten::function("binMsgToHex", binMsgToHex);
  // EXPORT_JS_BUILDER(MainBuilder);
  emscripten::class_<JsWasmTransport>("JsWasmTransport")
      .constructor<ROOT_JS_CLASS *, emscripten::val, emscripten::val>()
      .function("processMsg", &JsWasmTransport::processMsgStr);
  // e::function("processMsgForJs", &processMsgForJs);
};

#undef dbg
