#include "UAPI/gen/gen.h"

RootAPI api;

int main() {
  std::string binStr;
  reflect::serialize::to_bin_str(api, binStr);
  reflect::dump(api)
}
