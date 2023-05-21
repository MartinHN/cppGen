#pragma once
#include "traits.h"
#include <iostream>

using uapi::traits::InheritFromVec;
using uapi::traits::Vec;

template <typename T>
concept PrintableVec = Vec<T> || InheritFromVec<T>;

struct Dbg {
  Dbg(const char *_prefix) : prefix(_prefix) {}
  template <typename... Args> void err(Args... a) {
    beginOs(std::cerr, a...);
    endOs(std::cerr);
  }
  template <typename... Args> void print(Args... a) {
    beginOs(std::cout, a...);
    endOs(std::cout);
  }

  template <typename... Args> void begin(Args... a) {
    beginOs(std::cout, a...);
  }

  template <typename... Args> void add(Args... a) {
    (printWithSpace(std::cout, a), ...);
  }

  template <typename... Args> void end(Args... a) {
    add(a...);
    endOs(std::cout);
  }

  template <typename I>
  static std::string int_to_hex(I w, size_t hex_len = sizeof(I) << 1) {
    static const char *digits = "0123456789ABCDEF";
    std::string rc(hex_len, '0');
    for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
      rc[i] = digits[(w >> j) & 0x0f];
    return rc;
  }

private:
  template <typename... Args> void beginOs(std::ostream &os, Args... a) {
    if (getRunningLineOs() != nullptr) {
      endOs(*getRunningLineOs());
    }
    getRunningLineOs() = &os;
    printOs(os, a...);
  }
  template <typename... Args> void endOs(std::ostream &os) {
    printOne(os, "\n");
    getRunningLineOs() = nullptr;
  }

  template <typename... Args> void printOs(std::ostream &os, Args... a) {
    printOne(os, prefix);
    printOne(os, " : ");
    (printWithSpace(os, a), ...);
  }
  template <typename Arg> void printWithSpace(std::ostream &os, Arg a) {
    printOne(os, a);
    printOne(os, " ");
  }

  template <PrintableVec Arg> void printWithSpace(std::ostream &os, Arg v) {
    printOne(os, "[");
    for (const auto &e : v) {
      printWithSpace(os, e);
      printOne(os, ",");
    }
    printOne(os, "]");
  }

  template <typename Arg> inline void printOne(std::ostream &os, Arg a) {
    if (getRunningLineOs() == nullptr) {
      beginOs(os);
    }
    os << a;
  }

  const char *prefix;

  static std::ostream *&getRunningLineOs() {
    static std::ostream *r = nullptr;
    return r;
  }
};

#define CONST_ASSERT(x)                                                        \
  []<bool flag = false>() { static_assert(flag, x); }                          \
  ()
/* simple debug tool

use it like
[myfile.cpp]
static Dbg dbgPhy("[phy]");
#define dbg dbgPhy

....
dbg.print("any",stuff,printable); // will print [phy] any stuff printable
dbg.err("any",...)
....
#undef dbg
endof file
*/
