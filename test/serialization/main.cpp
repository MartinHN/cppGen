#include "../lib/simpletest/simpletest.h"
#include "./gen/cpp/gen.h"

#include "../lib/simpletest/simpletest.cpp"

DEFINE_TEST(SimpleSerialize) {
  RootAPI api;
  std::string binStr = uapi::serialize::to_bin_str<RootAPI>(api);
  TEST(binStr.size() > 0);
}

DEFINE_TEST(CopyProps) {
  RootAPI api;
  api.lastInt = 8;
  api.string = "lolo";
  api.vec.push_back(1);
  api.vecM.push_back({.i = 1});
  std::string binStr = uapi::serialize::to_bin_str<RootAPI>(api);
  TEST(binStr.size() > 0);
  RootAPI api2;
  uapi::serialize::from_bin_str(api2, binStr.data(), binStr.size());

  TEST(api.lastInt == 8);
  TEST(api2.lastInt == 8);
  TEST(api.string == "lolo");
  TEST(api2.string == "lolo");
  TEST(api2.vec.size() == 1);
  TEST(api.vec.size() == 1);
  TEST(api2.vec[0] == 1);
  TEST(api.vec[0] == 1);
  TEST(api2.vecM.size() == 1);
  TEST(api.vecM.size() == 1);
  TEST(api2.vecM[0].i == 1);
  TEST(api.vecM[0].i == 1);
}

DEFINE_TEST(SimpleDump) {
  RootAPI api;
  uapi::debug::dump(api);
}

DEFINE_TEST(sizeChecks) {
  // std::cout << sizeof(uapi::variants::AnyMemberRefVar) << std::endl;
  // std::cout << sizeof((void *)(nullptr)) << std::endl;
  // should d be sizeof((void *)(nullptr)) but well... cpp is adding some
  // overhed
  TEST(sizeof(uapi::variants::AnyMemberRefVar) <= 16);
  // std::cout << sizeof(uapi::variants::OptMemberRef) << std::endl;
  // std::cout << sizeof(uapi::variants::AnyMemberRefVar) << std::endl;
  // stilll some overhead...
  TEST(sizeof(uapi::variants::OptMemberRef) <= 24);

  TEST(uapi::variants::isUserDefined<IMember>::value);
  TEST(uapi::variants::isUserDefined<RootAPI>::value);
  TEST(!uapi::variants::isUserDefined<int>::value);
  TEST(!uapi::variants::isUserDefined<std::string>::value);
}

int main() {
  bool pass = true;

  pass &= TestFixture::ExecuteAllTests(TestFixture::Verbose);

  return pass ? 0 : 1;
}
