#include "UAPI/gen/gen.h"

RootAPI api;

int main() {
  std::string binStr;
  uapi::serialize::to_bin_str(api, binStr);
  uapi::dump(api)
}
