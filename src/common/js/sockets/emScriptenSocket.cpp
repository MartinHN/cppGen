#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
#include <stdio.h>

Dbg dbgSock("[ws]");
#define dbg dbgSock

// nasty globals
// uapi::MessageProcessorHandler *wsCliHandler = nullptr;
uapi::TransportBase *wsTransport = nullptr;
ConnHandler *wsConnHandler = nullptr;

EMSCRIPTEN_WEBSOCKET_T last_valid_socket = {};
bool isSocketOpened = false;

EM_JS(emscripten::EM_VAL, windowHostname, (), {
  var loc = new URL(window.location.href);
  // var jsString = loc.hostname;
  // var lengthBytes = lengthBytesUTF8(jsString) + 1;
  // var stringOnWasmHeap = _malloc(lengthBytes);
  // stringToUTF8(jsString, stringOnWasmHeap, lengthBytes);
  return Emval.toHandle(loc.hostname);
  ;
});

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

EM_BOOL
onopen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent,
       void *userData) {
  last_valid_socket = websocketEvent->socket;
  isSocketOpened = true;
  dbg.print("[ws] on open");

  if (wsConnHandler)
    wsConnHandler->onConnOpen();
  // EMSCRIPTEN_RESULT result;
  // std::string askMsg;
  // uapi::buildGetRootStateMessage(askMsg);
  // result = emscripten_websocket_send_binary(websocketEvent->socket,
  //                                           askMsg.data(), askMsg.size());

  // if (result) {
  //   printf("Failed to emscripten_websocket_send_utf8_text(): %d\n",
  //   result);
  // }
  return EM_TRUE;
}
EM_BOOL onerror(int eventType,
                const EmscriptenWebSocketErrorEvent *websocketEvent,
                void *userData) {
  isSocketOpened = false;
  if (wsConnHandler)
    wsConnHandler->onConnError();
  dbg.print("[ws] on error");
  return EM_TRUE;
}
EM_BOOL onclose(int eventType,
                const EmscriptenWebSocketCloseEvent *websocketEvent,
                void *userData) {
  isSocketOpened = false;
  if (wsConnHandler)
    wsConnHandler->onConnClose();
  dbg.print("[ws] on close");
  return EM_TRUE;
}

EM_BOOL onmessage(int eventType,
                  const EmscriptenWebSocketMessageEvent *websocketEvent,
                  void *userData) {
  // puts("onmessage");
  // if (websocketEvent->isText) {
  //   // For only ascii chars.
  //   printf("Text message: %s\n", websocketEvent->data);
  // } else {
  std::string respBuf;
  bool needResp = wsTransport->processMsg((char *)websocketEvent->data,
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
  if (wsTransport) {
    dbg.print("should init only once");
    return;
  }
  wsTransport = new uapi::TransportImpl<API>(&api);
  wsTransport->transportMsgHdlr = &msgHdlr;
  wsConnHandler = &conHdl;

  std::string hostNameStr = "localhost";
  emscripten::val hostName = emscripten::val::take_ownership(windowHostname());
  hostNameStr = hostName.as<std::string>();

  hostNameStr = "ws://" + hostNameStr + ":" + std::to_string(port);
  dbg.print("will open ws :: ", hostNameStr);
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
