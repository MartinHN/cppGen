
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

static Dbg dbgJs("[jsb]");
#define dbg dbgJs

#ifndef ROOT_JS_CLASS
#error ROOT_JS_CLASS should be defind
#endif
ROOT_JS_CLASS *jsBindCppObj = nullptr;
#include "logHelpers.cpp"
//
#include "valHelpers.cpp"
//
#include "builders.cpp"
//
#include "handlers.cpp"

JsBindMessageProcessorHandler *cliHdlr;
// JsMessageHandlerWrapper *toJsHdlr;
JsConnHandlerWrapper *conHdl;
void initJsBindRootApi(ROOT_JS_CLASS *api) { jsBindCppObj = api; }

static void initWebSocket(int port, const emscripten::val &connHdlr,
                          const emscripten::val &msgHdlr,
                          const emscripten::val &store) {
  if (!jsBindCppObj) {
    std::cerr << "root apin not inited with initJsBindRootApi()" << std::endl;
    return;
  }
  initJsBindRootApi(jsBindCppObj);
  conHdl = new JsConnHandlerWrapper(connHdlr);
  cliHdlr = new JsBindMessageProcessorHandler(store);
  cliHdlr->nextH = new JsMessageHandlerWrapper(msgHdlr);

  init_websocket(*jsBindCppObj, port, *conHdl, *cliHdlr);
}

EMSCRIPTEN_BINDINGS(JsBindScope) {
  // e::function("initForJsObj", &initForJsObj);
  emscripten::function("initWebSocket", &initWebSocket);
  e::function("buildJsBindModMessage", &buildJsBindModMessage);
  e::function("buildJsBindCallMessage", &buildJsBindCallMessage);
  e::function("buildGetRootStateMessage", &buildGetRootStateMessageJs);
  e::function("callMsgNeedsResp", &callMsgNeedsResp);
  emscripten::function("binMsgToHex", binMsgToHex);
  // e::function("processMsgForJs", &processMsgForJs);
};

#undef dbg
