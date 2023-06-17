
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

ROOT_JS_CLASS *jsBindCppObj = nullptr;
JsBindMessageProcessorHandler<ROOT_JS_CLASS> *cliHdlr;
// JsMessageHandlerWrapper *toJsHdlr;
JsConnHandlerWrapper *conHdl = nullptr;
void initJsBindRootApi(ROOT_JS_CLASS *api) {
    jsBindCppObj = api;
}

DECLARE_JS_BUILDER(MainBuilder, ROOT_JS_CLASS)
static void initWebSocket(int port, const emscripten::val &connHdlr,
                          const emscripten::val &msgHdlr,
                          const emscripten::val &store) {
    if (!jsBindCppObj) {
        std::cerr << "root apin not inited with initJsBindRootApi()"
                  << std::endl;
        return;
    }
    if (conHdl != nullptr || cliHdlr != nullptr) {
        std::cerr << "initWebSocket() has already been called" << std::endl;
        return;
    }
    IMPL_JS_BUILDER(MainBuilder, *jsBindCppObj);
    conHdl = new JsConnHandlerWrapper(connHdlr);
    cliHdlr =
        new JsBindMessageProcessorHandler<ROOT_JS_CLASS>(*jsBindCppObj, store);
    cliHdlr->nextH = new JsMessageHandlerWrapper(msgHdlr);

    init_websocket(*jsBindCppObj, port, *conHdl, *cliHdlr);
}

struct BinTransport : public uapi::TransportImpl<ROOT_JS_CLASS> {
    std::unique_ptr<JsBindMessageProcessorHandler<ROOT_JS_CLASS>> transpHdlr;
    BinTransport(const emscripten::val &jsMsgHdlr, const emscripten::val &store)
        : uapi::TransportImpl<ROOT_JS_CLASS>(jsBindCppObj) {
        if (!jsBindCppObj) {
          std::cerr << "root apin not inited with initJsBindRootApi()"
                    << std::endl;
          return;
        }
        // IMPL_JS_BUILDER(MainBuilder, *jsBindCppObj);

        transpHdlr.reset(new JsBindMessageProcessorHandler<ROOT_JS_CLASS>(
            *jsBindCppObj, store));
        transpHdlr->nextH = new JsMessageHandlerWrapper(jsMsgHdlr);
        transportMsgHdlr = transpHdlr.get();
    }

    virtual ~BinTransport() override = default;

    emscripten::val processMsgStr(std::string binMsg) {
        std::string resp;
        bool needResp = processMsg(binMsg.data(), binMsg.size(), resp);
        if (needResp)
          return strToJs(resp);
        return {};
    }
};

EMSCRIPTEN_BINDINGS(JsBindScope) {
    // e::function("initForJsObj", &initForJsObj);
    emscripten::function("initWebSocket", &initWebSocket);
    emscripten::function("binMsgToHex", binMsgToHex);
    EXPORT_JS_BUILDER(MainBuilder);
    emscripten::class_<BinTransport>("BinTransport")
        .constructor<emscripten::val, emscripten::val>()
        .function("processMsg", &BinTransport::processMsgStr);
    // e::function("processMsgForJs", &processMsgForJs);
};

#undef dbg
