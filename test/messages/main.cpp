#include "../lib/simpletest/simpletest.h"
#include "./gen/cpp/gen.h"

#include "../lib/simpletest/simpletest.cpp"

DEFINE_TEST(SimpleModMessage) {
  RootAPI api;
  api.a = 5;
  TEST(!uapi::traits::CanBeIterated<std::string>);
  std::string msg;
  TEST(uapi::buildModMessage(api, "a", 3, msg));
  TEST(msg.length() == 3); // expected 3 bytes : msgType, memberAddr, varInt
  std::string resp = {};
  bool needResp =
      uapi::processMessage(api, msg.data(), msg.size(), resp, nullptr);
  TEST(needResp == false);
  TEST(resp.length() == 0);
  TEST(api.a == 3);
}

DEFINE_TEST(SimpleGetMessage) {
  RootAPI api;
  api.a = 5;
  std::string msg;
  TEST(uapi::buildGetMessage(api, "a", msg));
  std::string resp = {};
  bool needResp =
      uapi::processMessage(api, msg.data(), msg.size(), resp, nullptr);
  TEST(needResp == true);
  TEST(resp.length() >= 0);

  RootAPI api2;
  std::string resp2 = {};
  needResp =
      uapi::processMessage(api2, resp.data(), resp.size(), resp2, nullptr);
  TEST(needResp == false);
  TEST(resp2.length() == 0);
  TEST(api2.a == 5);
}

DEFINE_TEST(SimpleCallMessage) {
  RootAPI api;

  std::string msg;
  TEST(uapi::buildCallMessage(api, "", "func", {22}, msg));
  std::string resp = {};
  bool needResp =
      uapi::processMessage(api, msg.data(), msg.size(), resp, nullptr);
  TEST(needResp == false);
  TEST(resp.length() == 0);
  TEST(api.functionValue == 22);
}

struct TestMessageHandler : public uapi::MessageProcessorHandler {

  // void onMemberSet(const std::string &name,
  //                  uapi::variants::AnyMemberRefVar &v) override {}
  // void onMemberGet(const std::string &name,
  //                  uapi::variants::AnyMemberRefVar &v) override {}
  // void onRootStateSet(const std::string &addr) override {}
  // void onRootStateGet(const std::string &addr) override {}
  void onFunctionCall(const std::string &name,
                      uapi::variants::AnyMethodArgsTuple &args,
                      uapi::variants::AnyMethodReturnValue &res) override {
    lastCallName = name;
    lastCallArgs = args;
    lastCallRes = res;
  }
  std::string lastCallName;
  uapi::variants::AnyMethodArgsTuple lastCallArgs;
  uapi::variants::AnyMethodReturnValue lastCallRes;

  void onFunctionResp(const std::string &name,
                      uapi::variants::AnyMethodReturnValue &res) override {
    lastCallRespName = name;
    lastFunctionResp = res;
  }
  std::string lastCallRespName;
  uapi::variants::AnyMethodReturnValue lastFunctionResp = {};
};

DEFINE_TEST(SimpleCallRespMessage) {
  RootAPI api;
  TestMessageHandler hdl;
  std::string msg;
  TEST(uapi::buildCallMessage(api, "", "funcR", {22}, msg));
  std::string resp = {};
  bool needResp = uapi::processMessage(api, msg.data(), msg.size(), resp, &hdl);
  TEST(needResp == true);
  TEST(resp.length() > 0);
  TEST(hdl.lastCallName == "funcR");
  TEST(std::get<std::tuple<int>>(hdl.lastCallArgs) == std::tuple(22));
  TEST(std::get<int>(hdl.lastCallRes) == 22);

  RootAPI api2;
  std::string resp2 = {};
  TestMessageHandler hdl2;
  needResp = uapi::processMessage(api2, resp.data(), resp.size(), resp2, &hdl2);
  TEST(hdl2.lastCallRespName == "funcR");
  TEST(std::get<int>(hdl2.lastFunctionResp) == 22);
  TEST(needResp == false);
  TEST(resp2.length() == 0);
}

DEFINE_TEST(NestedCallMessage) {
  RootAPI api;
  std::string msg;
  TEST(uapi::buildCallMessage(api, "/member", "funcM", {22}, msg));
  std::string resp = {};
  bool needResp =
      uapi::processMessage(api, msg.data(), msg.size(), resp, nullptr);
  TEST(needResp == false);
  TEST(resp.length() == 0);
  TEST(api.member.i == 22);
}

DEFINE_TEST(NestedVecCallMessage) {
  RootAPI api;
  api.vecM.push_back({.i = 9});
  std::string msg;
  TEST(uapi::buildCallMessage(api, "/vecM/0", "funcM", {22}, msg));
  std::string resp = {};
  bool needResp =
      uapi::processMessage(api, msg.data(), msg.size(), resp, nullptr);
  TEST(needResp == false);
  TEST(resp.length() == 0);
  TEST(api.vecM[0].i == 22);
}

DEFINE_TEST(SimpleRootStateMessage) {
  RootAPI api;
  api.a = 5;
  api.string = "lolo";
  std::string msg;
  TEST(uapi::buildMessageGetState(api, "", msg));
  std::string resp = {};
  bool needResp =
      uapi::processMessage(api, msg.data(), msg.size(), resp, nullptr);
  TEST(needResp == true);
  TEST(resp.length() > 0);

  RootAPI api2;
  std::string resp2 = {};
  needResp =
      uapi::processMessage(api2, resp.data(), resp.size(), resp2, nullptr);
  TEST(needResp == false);
  TEST(resp2.length() == 0);
  TEST(api2.a == 5);
  TEST(api2.string == "lolo");
}

int main() {
  bool pass = true;
  pass &= TestFixture::ExecuteAllTests(TestFixture::Verbose);
  return pass ? 0 : 1;
}
