#include "core/Dbg.h"
static Dbg dbgMsg("[msg]");
#define dbg dbgMsg

#include <iostream>

namespace uapi {

typedef unsigned char opCode_t;
enum class MessageOpcode : opCode_t { set = 1, get = 2 };

typedef unsigned char opCode_t;
enum class GetSetType : opCode_t { member = 1, method = 2, rootState = 3 };

const char *getSet_str(const opCode_t &o) {
  auto l = MessageOpcode(o);
  if (l == MessageOpcode::get)
    return "get";
  if (l == MessageOpcode::set)
    return "set";

  return "unknown";
}

const char *getSetType_str(const opCode_t &o) {
  auto l = GetSetType(o);
  if (l == GetSetType::member)
    return "member";
  if (l == GetSetType::method)
    return "method";
  if (l == GetSetType::rootState)
    return "rootState";

  return "unknown";
}

typedef unsigned char pack_ops_t;
constexpr pack_ops_t packOpCode(MessageOpcode m, GetSetType gs) {
  return (opCode_t(m) << 4) | (opCode_t(gs) & 0x0F);
}

constexpr std::tuple<opCode_t, opCode_t> unPackOpCode(pack_ops_t p) {
  return std::tuple(opCode_t(p) >> 4, opCode_t(p & 0x0F));
}

constexpr bool tstPack(MessageOpcode m, GetSetType gs) {
  return unPackOpCode(packOpCode(m, gs)) ==
         std::tuple(opCode_t(m), opCode_t(gs));
}

static_assert(tstPack(MessageOpcode::set, GetSetType::member));
static_assert(tstPack(MessageOpcode::get, GetSetType::member));

template <typename T, typename NodeT>
bool buildModMessage(NodeT &parentNode, const std::string &memberAddr, T i,
                     std::string &modBuf) {

  auto strAddr = uapi::variants::strAddrFromStr(memberAddr);
  auto ointAddr = uapi::variants::addressStrToInt(parentNode, strAddr);
  if (!ointAddr) {
    dbg.print("no member found for ", memberAddr);
    return {};
  }
  auto iAddr = *ointAddr;
  auto member = uapi::variants::getMemberWithAddressInt(parentNode, iAddr);
  if (!member) {
    dbg.print("no member found for ", memberAddr);
    return {};
  }

  bool isValid = false;
  std::visit(
      [&iAddr, &i, &isValid, &modBuf](auto &&arg) {
        using TRUT = std::decay_t<std::reference_wrapper<std::decay_t<T>>>;
        using ArgT = std::decay_t<decltype(arg)>;
        if (std::is_same<TRUT, ArgT>::value) {
          std::ostringstream oss;
          uapi::serialize::write_value<pack_ops_t>(
              oss, packOpCode(MessageOpcode::set, GetSetType::member));

          // uapi::serialize::write_value<variants::MemberIdx>(oss,
          // memberIdx);
          uapi::serialize::write_value<uapi::variants::MemberAddressInt>(oss,
                                                                         iAddr);
          uapi::serialize::write_value<T>(oss, i);
          modBuf = oss.str();

          // dbg.begin("set msg", modBuf.size(), "::");
          // for (const auto &m : modBuf)
          //   dbg.add(dbg.int_to_hex((char)m));
          // dbg.end();

          isValid = true;
        }
      },
      *member);

  if (!isValid) {
    dbg.print("wrong member type found for ", memberAddr);
  }
  return isValid;
}

template <typename NodeT>
bool buildGetMessage(NodeT &parentNode, const std::string &memberAddr,
                     std::string &modBuf) {
  auto strAddr = uapi::variants::strAddrFromStr(memberAddr);
  auto ointAddr = uapi::variants::addressStrToInt(parentNode, strAddr);
  if (!ointAddr) {
    dbg.print("no member found for ", memberAddr);
    return {};
  }
  auto iAddr = *ointAddr;
  auto member = uapi::variants::getMemberWithAddressInt(parentNode, iAddr);
  if (!member) {
    dbg.print("no member found for ", memberAddr);
    return {};
  }

  std::ostringstream oss;
  uapi::serialize::write_value<pack_ops_t>(
      oss, packOpCode(MessageOpcode::get, GetSetType::member));
  uapi::serialize::write_value<variants::MemberAddressInt>(oss, iAddr);
  modBuf = oss.str();
  // dbg.begin("get msg", modBuf.size(), "::");
  // for (const auto &m : modBuf)
  //   dbg.add(dbg.int_to_hex((char)m));
  // dbg.end();
  return true;
}

template <typename NodeT>
bool buildCallMessage(NodeT &parentNode, const std::string &childAddr,
                      const std::string &methodName,
                      const variants::AnyMethodArgsTuple &args,
                      std::string &modBuf) {
  auto strAddr = uapi::variants::strAddrFromStr(childAddr);
  auto ointAddr = uapi::variants::addressStrToInt(parentNode, strAddr);
  if (!ointAddr) {
    dbg.print("[call] no member found for ", childAddr);
    return {};
  }
  auto iAddr = *ointAddr;
  auto member = uapi::variants::getMemberWithAddressInt(parentNode, iAddr);
  if (!member) {
    dbg.print("no member found for ", childAddr);
    return {};
  }
  std::optional<variants::MemberIdx> methodIdx = {};
  std::visit(
      [&methodName, &methodIdx](auto &&arg) {
        using ArgT = std::decay_t<decltype(arg)>;
        using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
        if constexpr (uapi::variants::isUserDefined<TRUT>::value)
          methodIdx =
              uapi::variants::getIdxOfMethodName<TRUT>(arg, methodName.c_str());
      },
      *member);

  if (!methodIdx) {
    dbg.print("method idx not found");
    return false;
  }
  dbg.print("build call for", childAddr, methodName);
  std::ostringstream oss;
  uapi::serialize::write_value<pack_ops_t>(
      oss, packOpCode(MessageOpcode::get, GetSetType::method));
  uapi::serialize::write_value<variants::MemberAddressInt>(oss, iAddr);
  uapi::serialize::write_value<variants::MemberIdx>(oss, *methodIdx);
  uapi::serialize::write_value<variants::AnyMethodArgsTuple>(oss, args);

  modBuf = oss.str();
  return true;
}

bool buildCallResp(const variants::MemberAddressInt &memberAddr,
                   const variants::MemberIdx &funIdx,
                   const variants::AnyMethodReturnValue &resp,
                   std::string &modBuf) {
  bool needResp = true;
  std::visit(
      [&needResp](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        needResp = !std::is_same_v<uapi::variants::VoidReturn, T>;
      },
      resp);

  if (!needResp)
    return false;
  std::ostringstream oss;
  uapi::serialize::write_value<pack_ops_t>(
      oss, packOpCode(MessageOpcode::set, GetSetType::method));

  uapi::serialize::write_value(oss, memberAddr);
  uapi::serialize::write_value(oss, funIdx);
  std::visit([&oss](auto &&arg) { uapi::serialize::write_value(oss, arg); },
             resp);

  modBuf = oss.str();
  return true;
}

template <typename NodeT>
bool buildGetRootStateMessage(NodeT &parentNode, const std::string &childAddr,
                              std::string &modBuf) {
  std::ostringstream oss;
  uapi::serialize::write_value<pack_ops_t>(
      oss, packOpCode(MessageOpcode::get, GetSetType::rootState));
  auto strAddr = uapi::variants::strAddrFromStr(childAddr);
  auto ointAddr = uapi::variants::addressStrToInt(parentNode, strAddr);
  if (!ointAddr) {
    dbg.print("[call] no member found for ", childAddr);
    return false;
  }
  uapi::serialize::write_value(oss, *ointAddr);
  modBuf = oss.str();
  return true;
}

template <typename NodeT>
bool buildSetRootStateMessage(NodeT &parentNode, std::string &childAddr,
                              std::string &modBuf) {
  std::ostringstream oss;
  uapi::serialize::write_value<pack_ops_t>(
      oss, packOpCode(MessageOpcode::set, GetSetType::rootState));
  auto strAddr = uapi::variants::strAddrFromStr(childAddr);
  auto ointAddr = uapi::variants::addressStrToInt(parentNode, strAddr);
  if (!ointAddr) {
    dbg.print("[call] no member found for ", childAddr);
    return false;
  }
  uapi::serialize::write_value(oss, *ointAddr);
  uapi::serialize::write_value<NodeT>(oss, parentNode);

  modBuf = oss.str();
  return true;
}

struct MessageProcessorHandler {
  virtual void onMemberSet(const std::string &name,
                           uapi::variants::AnyMemberRefVar &v){};
  virtual void onMemberGet(const std::string &name,
                           uapi::variants::AnyMemberRefVar &v){};
  virtual void onRootStateSet(const std::string &addr) {}
  virtual void onRootStateGet(const std::string &addr) {}
  virtual void onFunctionCall(const std::string &name,
                              uapi::variants::AnyMethodArgsTuple &args,
                              uapi::variants::AnyMethodReturnValue &res){};
  // the only unimplemented functino, it's app reponsability to know what to
  // do
  virtual void onFunctionResp(const std::string &name,
                              uapi::variants::AnyMethodReturnValue &res){};
};

template <typename T>
bool processMessage(T &parentNode, char *beg, size_t length,
                    std::string &respBuf, MessageProcessorHandler *handler) {
  std::istringstream iss(std::string(beg, length));
  pack_ops_t op_pack = 0;
  uapi::serialize::parse_value<pack_ops_t>(op_pack, iss);
  auto [op, getSetType] = unPackOpCode(op_pack);

  dbg.print("msgType : ", getSetType_str(getSetType), getSet_str(op),
            "l:", length);
  if (MessageOpcode(op) == MessageOpcode::set ||
      (MessageOpcode(op) == MessageOpcode::get)) {
    bool isSet = MessageOpcode(op) == MessageOpcode::set;

    //////////
    // members
    ////////////
    if (GetSetType(getSetType) == GetSetType::member) {
      variants::MemberAddressInt memberAddrInt = {};
      uapi::serialize::parse_value<variants::MemberAddressInt>(memberAddrInt,
                                                               iss);

      auto member =
          uapi::variants::getMemberWithAddressInt(parentNode, memberAddrInt);
      if (!member) {
        dbg.print("no member found for addr ", memberAddrInt);
        return {};
      }
      auto memberAddr =
          uapi::variants::intAddressToStr(parentNode, memberAddrInt);
      if (memberAddr.size() == 0) {
        dbg.err("no member name found for  idx", memberAddrInt);
        return {};
      }

      if (isSet) {

        dbg.print(dbg.int_to_hex((int64_t)(void *)&parentNode));
        std::visit(
            [&memberAddr](auto &&a) {
              using ArgT = std::decay_t<decltype(a)>;
              using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
              if constexpr (std::is_arithmetic_v<TRUT>)
                dbg.print(memberAddr, ">>>>was ", a);
            },
            *member);
        // if (auto *ia =
        // std::get_if<std::reference_wrapper<uint32_t>>(&*member))
        //   dbg.print(">>>>was ", *ia);
        std::visit(
            [&iss](auto &&arg) {
              using ArgT = std::decay_t<decltype(arg)>;
              using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
              uapi::serialize::parse_value<TRUT>(arg.get(), iss);
            },
            *member);
        std::visit(
            [&memberAddr](auto &&a) {
              using ArgT = std::decay_t<decltype(a)>;
              using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
              if constexpr (std::is_arithmetic_v<TRUT>)
                dbg.print(memberAddr, ">>>>is ", a);
            },
            *member);
        if (handler)
          handler->onMemberSet(memberAddr, *member);
        return false;

      } else {
        std::visit(
            [&respBuf, &parentNode, &memberAddr](auto &&arg) {
              buildModMessage(parentNode, memberAddr, arg.get(), respBuf);
            },
            *member);
        if (handler)
          handler->onMemberGet(memberAddr, *member);

        return true;
      }

      dbg.err("!!!!!!! unreachable", memberAddr);
      return false;
    } else if (GetSetType(getSetType) == GetSetType::rootState) {

      variants::MemberAddressInt memberAddrInt = {};
      uapi::serialize::parse_value<variants::MemberAddressInt>(memberAddrInt,
                                                               iss);

      auto member =
          uapi::variants::getMemberWithAddressInt(parentNode, memberAddrInt);
      if (!member) {
        dbg.print("no member found for addr ", memberAddrInt);
        return {};
      }

      auto memberAddr =
          uapi::variants::intAddressToStr(parentNode, memberAddrInt);

      if (isSet) {
        dbg.print("setting rootState ");
        dbg.print("!! rootState was :");
        std::visit(
            [&memberAddr, &handler, &iss](auto &&m) {
              using ArgT = std::decay_t<decltype(m)>;
              using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
              if constexpr (uapi::variants::isUserDefined<TRUT>::value) {
                uapi::debug::dump<TRUT>(m);
                // TODO full node address
                uapi::serialize::from_bin<TRUT>(m, iss);
                dbg.print("!! rootState is now :");
                uapi::debug::dump<TRUT>(m);
                if (handler)
                  handler->onRootStateSet(memberAddr);
              } else {
                dbg.print("no set rooot state for ", memberAddr);
              }
            },
            *member);
        return false;

      } else {
        dbg.print("get rootState ");
        std::ostringstream oss;
        uapi::serialize::write_value<pack_ops_t>(
            oss, packOpCode(MessageOpcode::set, GetSetType::rootState));
        uapi::serialize::write_value(oss, memberAddrInt);
        bool isValid = false;
        std::visit(
            [&isValid, &oss](auto &&m) {
              using ArgT = std::decay_t<decltype(m)>;
              using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
              if constexpr (uapi::variants::isUserDefined<TRUT>::value) {
                uapi::serialize::write_value<TRUT>(oss, m);
                isValid = true;
              }
            },
            *member);
        if (isValid) {
          respBuf = oss.str();
          if (handler)
            handler->onRootStateGet(memberAddr);
        }

        else {
          dbg.print("no get rooot state for ", memberAddr);
        }

        return isValid;
      }
    }
    ///////////
    /// methods
    ///////////
    else if (GetSetType(getSetType) == GetSetType::method) {
      variants::MemberAddressInt memberAddrInt = {};
      uapi::serialize::parse_value<variants::MemberAddressInt>(memberAddrInt,
                                                               iss);
      auto member =
          uapi::variants::getMemberWithAddressInt(parentNode, memberAddrInt);
      if (!member) {
        dbg.print("no member found for addr ", memberAddrInt);
        return {};
      }
      // auto memberAddr =
      //     uapi::variants::intAddressToStr(parentNode, memberAddrInt);

      variants::MemberIdx methodIdx;
      uapi::serialize::parse_value(methodIdx, iss);
      std::string methodName;
      std::visit(
          [&methodIdx, &methodName](auto &&arg) {
            using ArgT = std::decay_t<decltype(arg)>;
            using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
            if constexpr (uapi::variants::isUserDefined<TRUT>::value) {
              auto methods = uapi::variants::getMethodNames<TRUT>(arg.get());
              if (methodIdx < methods.size())
                methodName = methods[methodIdx];
            }
          },
          *member);

      if (methodName.size() == 0) {
        dbg.err("no method found for ", methodIdx, isSet ? "set" : "get");
        return {};
      }
      uapi::variants::Method *method = {};
      std::visit(
          [&method, &methodName](auto &&arg) {
            using ArgT = std::decay_t<decltype(arg)>;
            using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
            if constexpr (uapi::variants::isUserDefined<TRUT>::value)
              method = uapi::variants::getMethodWithName<TRUT>(
                  arg, methodName.c_str());
          },
          *member);
      if (!method) {
        dbg.err("no method found for ", methodName, isSet ? "set" : "get");
        return {};
      }
      if (!isSet) {
        dbg.print("function call ", methodName);
        auto args = method->parse_args(iss);
        auto res = method->call(args);
        buildCallResp(memberAddrInt, methodIdx, res, respBuf);
        if (handler)
          handler->onFunctionCall(methodName, args, res);
        return respBuf.size() > 0;

      } else {
        dbg.print("lllll", iss.tellg());
        dbg.print("function resp ", methodName);
        auto resp = method->parse_resp(iss);
        if (handler)
          handler->onFunctionResp(methodName, resp);
        else // TODO full node address
          dbg.print("resp function resp not implemented (no handler) ");

        return false;
      }
    }
  }

  dbg.print("op code not supported : ", op, length);
  return false;
}

} // namespace uapi

#undef dbg
