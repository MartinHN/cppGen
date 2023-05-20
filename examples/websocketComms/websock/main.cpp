#include "../../../gen/gen.h"

// #include <fstream>
// #include <iostream>
// #include <sstream>
RootAPI api;

// The ASIO_STANDALONE define is necessary to use the standalone version of
// Asio. Remove if you are using Boost Asio.
#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <functional>

#include <set>

typedef websocketpp::server<websocketpp::config::asio> server;
typedef std::shared_ptr<websocketpp::config::core::message_type> WebsockMsgPtr;
using websocketpp::connection_hdl;

void broadcastMsg(char *data, size_t len, connection_hdl *butNotThis = nullptr);
struct ServerMessageHandler : public reflect::MessageProcessorHandler {

  void onMemberSet(const std::string &name,
                   reflect::variants::AnyMemberRefVar &v) override {
    broadcastMsg(msgPtr->get_raw_payload().data(), msgPtr->get_payload().size(),
                 &processed_con);
  };
  void onMemberGet(const std::string &name,
                   reflect::variants::AnyMemberRefVar &v) override{};
  void onRootStateSet() override {
    broadcastMsg(msgPtr->get_raw_payload().data(), msgPtr->get_payload().size(),
                 &processed_con);
  }
  void onRootStateGet() override {
    // TODO remove this
    // broadcastMsg(msgPtr->get_raw_payload().data(),
    // msgPtr->get_payload().size(),
    //              &processed_con);
  }
  void onFunctionCall(const std::string &name,
                      reflect::variants::AnyMethodArgsValue &args,
                      reflect::variants::AnyMethodReturnValue &res) override{};
  // the only unimplemented functino, it's app reponsability to know what to do
  void onFunctionResp(const std::string &name,
                      reflect::variants::AnyMethodReturnValue &res) override{};

  WebsockMsgPtr msgPtr;
  connection_hdl processed_con;
};
ServerMessageHandler serverHandler;

class utility_server {
public:
  utility_server() {
    // Set logging settings
    m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
    m_endpoint.set_error_channels(websocketpp::log::elevel::rerror);
    m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
    m_endpoint.set_access_channels(websocketpp::log::alevel::fail);

    // Initialize Asio
    m_endpoint.init_asio();
    m_endpoint.set_reuse_addr(true);

    m_endpoint.set_open_handler([this](auto hdl) {
      m_connections.insert(hdl);
      std::cout << "new Connection" << std::endl;
    });

    m_endpoint.set_message_handler([this](auto hdl, WebsockMsgPtr msg) {
      // auto lock = hdl.lock().get();
      std::cout << "new Msg : ";
      serverHandler.processed_con = hdl;
      serverHandler.msgPtr = msg;
      if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        std::cout << "Text not supported" << std::endl;

      } else {
        std::cout << "Binary " << std::endl;
        std::string respBuf;
        bool needResp = reflect::processMessage(
            api, msg->get_raw_payload().data(), msg->get_payload().size(),
            respBuf, &serverHandler);
        std::cout << "handled " << std::endl;
        if (needResp) {
          std::cout << "sending resp " << std::endl;
          m_endpoint.send(hdl, respBuf.c_str(), respBuf.size(),
                          websocketpp::frame::opcode::binary);
        }
      }

      // std::cout << msg->get_payload() << std::endl;
      // m_endpoint.send(hdl, msg->get_payload(), msg->get_opcode());
    });

    m_endpoint.set_close_handler([this](auto hdl) {
      m_connections.erase(hdl);
      std::cout << "Connection Closed" << std::endl;
    });
    m_endpoint.set_fail_handler([this](auto hdl) {
      m_connections.erase(hdl);
      std::cout << "Connection Failed" << std::endl;
    });
  }

  bool connections_equal(connection_hdl t, connection_hdl u) {
    return !t.owner_before(u) && !u.owner_before(t);
  }
  void broadcastMsg(char *data, size_t len, connection_hdl *butNotThis) {
    // std::cout << "broadcast start : ";
    for (const auto &it : m_connections) {
      if (butNotThis && connections_equal(it, *butNotThis)) {
        // std::cout << "skip ,  ";
        continue;
      }

      // std::cout << "send ,  ";
      m_endpoint.send(it, data, len, websocketpp::frame::opcode::binary);
    }
    // std::cout << std::endl;
  }

  void run() {
    // Listen on port 9002
    auto port = 9002;
    m_endpoint.listen(port);
    std::cout << "start listening on " << port << std::endl;

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

utility_server s;
void broadcastMsg(char *data, size_t len, connection_hdl *butNotThis) {
  s.broadcastMsg(data, len, butNotThis);
}

int main() {
  api.name = "from server";
  api.counter = 88;
  // api.intVector.push_back(555);
  // api.universe.fill(100);
  s.run();
  return 0;
}
