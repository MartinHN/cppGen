#include <bit>
#include <iostream>
#include <set>

// The ASIO_STANDALONE define is necessary to use the standalone version of
// Asio. Remove if you are using Boost Asio.
#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
using websocketpp::connection_hdl;
typedef std::shared_ptr<websocketpp::config::core::message_type> WebsockMsgPtr;
typedef websocketpp::server<websocketpp::config::asio> server;

Dbg dbgServ("[wsS]");
#define dbg dbgServ

void broadcastMsg(char *data, size_t len, connection_hdl *butNotThis = nullptr);

struct ServerMessageHandler : public uapi::MessageProcessorHandler {

  void onMemberSet(const std::string &name,
                   uapi::variants::AnyMemberRefVar &v) override {
    broadcastMsg(msgPtr->get_raw_payload().data(), msgPtr->get_payload().size(),
                 &processed_con);
    if (externalHandler)
      externalHandler->onMemberSet(name, v);
  };
  void onMemberGet(const std::string &name,
                   uapi::variants::AnyMemberRefVar &v) override {
    if (externalHandler)
      externalHandler->onMemberGet(name, v);
  };
  void onRootStateSet() override {
    broadcastMsg(msgPtr->get_raw_payload().data(), msgPtr->get_payload().size(),
                 &processed_con);
    if (externalHandler)
      externalHandler->onRootStateSet();
  }
  void onRootStateGet() override {
    // TODO remove this
    // broadcastMsg(msgPtr->get_raw_payload().data(),
    // msgPtr->get_payload().size(),
    //              &processed_con);
    if (externalHandler)
      externalHandler->onRootStateGet();
  }
  void onFunctionCall(const std::string &name,
                      uapi::variants::AnyMethodArgsTuple &args,
                      uapi::variants::AnyMethodReturnValue &res) override {
    if (externalHandler)
      externalHandler->onFunctionCall(name, args, res);
  };
  // the only unimplemented functino, it's app reponsability to know what to do
  void onFunctionResp(const std::string &name,
                      uapi::variants::AnyMethodReturnValue &res) override {
    if (externalHandler)
      externalHandler->onFunctionResp(name, res);
  };

  WebsockMsgPtr msgPtr;
  connection_hdl processed_con;
  uapi::MessageProcessorHandler *externalHandler = nullptr;
};
ServerMessageHandler serverHandler;

class MainWsServer {
public:
  MainWsServer() {
    // Set logging settings
    m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
    m_endpoint.set_error_channels(websocketpp::log::elevel::rerror);
    m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
    m_endpoint.set_access_channels(websocketpp::log::alevel::fail);

    // Initialize Asio
    m_endpoint.init_asio();
    m_endpoint.set_reuse_addr(true);
  }

  bool connections_equal(connection_hdl t, connection_hdl u) {
    return !t.owner_before(u) && !u.owner_before(t);
  }

  void broadcastMsg(char *data, size_t len, connection_hdl *butNotThis) {
    // dbg.print( "broadcast start : ";
    for (const auto &it : m_connections) {
      if (butNotThis && connections_equal(it, *butNotThis)) {
        // dbg.print( "skip ,  ";
        continue;
      }

      // dbg.print( "send ,  ";
      m_endpoint.send(it, data, len, websocketpp::frame::opcode::binary);
    }
    // dbg.print( std::endl;
  }

  template <typename APIT>
  void init(APIT &api, int port, ConnHandler &conHdl,
            uapi::MessageProcessorHandler &msgHdlr) {
    // Listen on port
    m_endpoint.listen(port);
    dbg.print("start listening on ", port);

    m_endpoint.set_open_handler([this, &conHdl](auto hdl) {
      m_connections.insert(hdl);
      dbg.print("new Connection");
      conHdl.onConnOpen();
    });

    m_endpoint.set_close_handler([this, &conHdl](auto hdl) {
      m_connections.erase(hdl);
      dbg.print("Connection Closed");
      conHdl.onConnClose();
    });
    m_endpoint.set_fail_handler([this, &conHdl](auto hdl) {
      m_connections.erase(hdl);
      dbg.print("Connection Failed");
      conHdl.onConnError();
    });

    serverHandler.externalHandler = &msgHdlr;
    m_endpoint.set_message_handler([this, &api](auto hdl, WebsockMsgPtr msg) {
      // auto lock = hdl.lock().get();
      dbg.begin("new Msg : ");
      serverHandler.processed_con = hdl;
      serverHandler.msgPtr = msg;
      if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        dbg.err("Text not supported");

      } else {
        dbg.add("Binary ");
        std::string respBuf;

        bool needResp = uapi::processMessage<APIT>(
            api, msg->get_raw_payload().data(), msg->get_payload().size(),
            respBuf, &serverHandler);
        dbg.end("handled ");
        if (needResp) {
          dbg.print("sending resp ");
          m_endpoint.send(hdl, respBuf.c_str(), respBuf.size(),
                          websocketpp::frame::opcode::binary);
          dbg.print("resp ended ");
        }
      }

      // dbg.print( msg->get_payload() );
      // m_endpoint.send(hdl, msg->get_payload(), msg->get_opcode());
    });
  }
  void run() {
    // Queues a connection accept operation
    m_endpoint.start_accept();

    // Start the Asio io_service run loop
    m_endpoint.run();
  }

private:
  server m_endpoint;
  server::connection_ptr con;

  typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;
  con_list m_connections;
};

MainWsServer mainWsServer;

// public function
void broadcastMsg(char *data, size_t len, connection_hdl *butNotThis) {
  mainWsServer.broadcastMsg(data, len, butNotThis);
}

void runServer() { mainWsServer.run(); }

template <typename API>
void init_websocket(API &api, int port, ConnHandler &conHdl,
                    uapi::MessageProcessorHandler &msgHdlr) {

  mainWsServer.init(api, port, conHdl, msgHdlr);
}

#undef dbg

/*
#include "../../../gen/gen.h"

// #include <fstream>
// #include <iostream>
// #include <sstream>
RootAPI api;

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <functional>



typedef websocketpp::server<websocketpp::config::asio> server;
typedef std::shared_ptr<websocketpp::config::core::message_type>
WebsockMsgPtr; using websocketpp::connection_hdl;

void broadcastMsg(char *data, size_t len, connection_hdl *butNotThis =
nullptr);


int main() {
  api.name = "from server";
  api.counter = 88;
  // api.intVector.push_back(555);
  // api.universe.fill(100);
  s.run();
  return 0;
}
*/
