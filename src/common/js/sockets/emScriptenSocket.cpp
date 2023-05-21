#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
#include <stdio.h>

Dbg dbgSock("[ws]");
#define dbg dbgSock

EMSCRIPTEN_WEBSOCKET_T last_valid_socket = {};
bool isSocketOpened = false;

EM_JS(char *, windowHostname, (), {
  var loc = new URL(window.location.href);
  var jsString = loc.hostname;
  var lengthBytes = lengthBytesUTF8(jsString) + 1;
  var stringOnWasmHeap = _malloc(lengthBytes);
  stringToUTF8(jsString, stringOnWasmHeap, lengthBytes);
  return stringOnWasmHeap;
});

struct JsMessageHandlerWrapper : public JsObjWrapper,
                                 public uapi::MessageProcessorHandler {

  JsMessageHandlerWrapper(const emscripten::val &di) : JsObjWrapper(di) {

    // dbg.print("type of di : ", di.typeOf().as<std::string>());
    // logObj(dispatcher);
    call("onInit");
  }
  // should be given as an instance of JSEmptyHandler

  void onMemberSet(const std::string &addr,
                   uapi::variants::AnyMemberRefVar &v) override {
    call("onSet", addr);
  };
  void onMemberGet(const std::string &addr,
                   uapi::variants::AnyMemberRefVar &v) override {
    call("onGet", addr);
  };
  void onRootStateSet(const std::string &addr) override {
    call("onRootStateSet", addr);
  }
  void onRootStateGet(const std::string &addr) override {}
  void onFunctionCall(const std::string &addr,
                      uapi::variants::AnyMethodArgsTuple &args,
                      uapi::variants::AnyMethodReturnValue &res) override {
    // the only unimplemented function, it's app reponsability to know what to
    call("onCall", addr);
  };

  void onFunctionResp(const std::string &addr,
                      uapi::variants::AnyMethodReturnValue &res) override {
    std::visit(
        [this, &addr](auto &&r) {
          using T = decltype(r);
          if constexpr (uapi::traits::printable<T>)
            dbg.print(r, cppTypeOf<T>());
          emscripten::val respV(r);
          call("onCallResp", addr, respV);
        },
        res);
  };
};

struct JSEmptyMsgHandler {
  void onInit(){};
  void onSet(const uapi::variants::MemberAddressStr &s,
             const uapi::variants::MemberAddressStr &name){};
  void onGet(const uapi::variants::MemberAddressStr &s,
             const uapi::variants::MemberAddressStr &name){};
  void onCall(const std::string &addr, const std::string &name){};
  void onCallResp(const std::string &addr, const emscripten::val &resp){};
  void onRootStateSet(const std::string &addr){};
};

EMSCRIPTEN_BINDINGS(JsMessageHandler) {
  emscripten::class_<JSEmptyMsgHandler>("JSHandler")
      .constructor()
      .function("onInit", &JSEmptyMsgHandler::onInit)
      .function("onSet", &JSEmptyMsgHandler::onSet)
      .function("onGet", &JSEmptyMsgHandler::onGet)
      .function("onCall", &JSEmptyMsgHandler::onCall)
      .function("onCallResp", &JSEmptyMsgHandler::onCallResp)
      .function("onRootStateSet", &JSEmptyMsgHandler::onRootStateSet);
}

uapi::MessageProcessorHandler *cliHandler = nullptr;

struct JsConnHandlerWrapper : public JsObjWrapper, public ConnHandler {
  JsConnHandlerWrapper(const emscripten::val &o) : JsObjWrapper(o) {}
  void onConnInit() override { call("onConnInit"); };
  void onConnOpen() override { call("onConnOpen"); };
  void onConnError() override { call("onConnError"); };
  void onConnClose() override { call("onConnClose"); };
};

EMSCRIPTEN_BINDINGS(SocketHandler) {
  emscripten::class_<ConnHandler>("ConnHandler")
      .constructor()
      .function("onConnInit", &ConnHandler::onConnInit)
      .function("onConnOpen", &ConnHandler::onConnOpen)
      .function("onConnError", &ConnHandler::onConnError)
      .function("onConnClose", &ConnHandler::onConnClose);
}

ConnHandler *connHandler = nullptr;

EM_BOOL
onopen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent,
       void *userData) {
  last_valid_socket = websocketEvent->socket;
  isSocketOpened = true;
  dbg.print("[ws] on open");

  if (connHandler)
    connHandler->onConnOpen();
  // EMSCRIPTEN_RESULT result;
  // std::string askMsg;
  // uapi::buildGetRootStateMessage(askMsg);
  // result = emscripten_websocket_send_binary(websocketEvent->socket,
  //                                           askMsg.data(), askMsg.size());

  // if (result) {
  //   printf("Failed to emscripten_websocket_send_utf8_text(): %d\n", result);
  // }
  return EM_TRUE;
}
EM_BOOL onerror(int eventType,
                const EmscriptenWebSocketErrorEvent *websocketEvent,
                void *userData) {
  isSocketOpened = false;
  if (connHandler)
    connHandler->onConnError();
  dbg.print("[ws] on error");
  return EM_TRUE;
}
EM_BOOL onclose(int eventType,
                const EmscriptenWebSocketCloseEvent *websocketEvent,
                void *userData) {
  isSocketOpened = false;
  if (connHandler)
    connHandler->onConnClose();
  dbg.print("[ws] on close");
  return EM_TRUE;
}

std::function<bool(char *data, size_t s, std::string &respBuf)>
    msgProcessingFunction = nullptr;

EM_BOOL onmessage(int eventType,
                  const EmscriptenWebSocketMessageEvent *websocketEvent,
                  void *userData) {
  // puts("onmessage");
  // if (websocketEvent->isText) {
  //   // For only ascii chars.
  //   printf("Text message: %s\n", websocketEvent->data);
  // } else {
  std::string respBuf;
  bool needResp = msgProcessingFunction((char *)websocketEvent->data,
                                        websocketEvent->numBytes, respBuf);

  if (needResp) {
    return emscripten_websocket_send_binary(websocketEvent->socket,
                                            respBuf.data(), respBuf.size());
  }
  // }

  return EM_TRUE;
}

template <typename API>
void init_websocket(API &api, int port, ConnHandler &conHdl,
                    uapi::MessageProcessorHandler &msgHdlr) {
  if (!emscripten_websocket_is_supported()) {
    dbg.print("no web socket !!!!");
  }
  if (cliHandler || connHandler) {
    dbg.print("should init only once");
    return;
  }
  cliHandler = &msgHdlr;
  connHandler = &conHdl;

  msgProcessingFunction = [&api](char *data, size_t numBytes,
                                 std::string &respBuf) -> bool {
    bool needResp =
        uapi::processMessage<API>(api, data, numBytes, respBuf, cliHandler);
    return needResp;
  };

  std::string hostNameStr = "localhost";
  char *hostName = windowHostname();
  hostNameStr = hostName;
  dbg.print(hostName);
  free(hostName);

  hostNameStr = "ws://" + hostNameStr + ":" + std::to_string(port);
  EmscriptenWebSocketCreateAttributes ws_attrs = {hostNameStr.data(), NULL,
                                                  EM_TRUE};
  EMSCRIPTEN_WEBSOCKET_T ws = emscripten_websocket_new(&ws_attrs);
  emscripten_websocket_set_onopen_callback(ws, NULL, onopen);
  emscripten_websocket_set_onerror_callback(ws, NULL, onerror);
  emscripten_websocket_set_onclose_callback(ws, NULL, onclose);
  emscripten_websocket_set_onmessage_callback(ws, NULL, onmessage);
  dbg.print("web socket inited");
}

void send_msg(char *data, size_t len) {
  if (isSocketOpened) {
    // dbg.print( "start message send" );
    auto result =
        emscripten_websocket_send_binary(last_valid_socket, data, len);
    if (result) {
      printf("Failed to emscripten_websocket_send_binary(): %d\n", result);
    }
    // dbg.print( "end message send" );
  } else {
    dbg.err("socket not opened");
  }
}



void send_msg_str(std::string s) { send_msg(s.data(), s.size()); }
EMSCRIPTEN_BINDINGS(GlobalWs) {
  emscripten::function("sendToServer", &send_msg_str);
  emscripten::function("sendToServer", &send_msg_str);
}

// API

// void sendGetState() {
//   std::string askMsg;
//   uapi::buildGetRootStateMessage(askMsg);
//   send_msg(askMsg);
// }

// void sendSetState() {
//   std::string askMsg;
//   uapi::buildSetRootStateMessage(api, askMsg);
//   send_msg(askMsg);
// }

#undef dbg
