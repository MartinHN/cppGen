namespace reflect {

typedef unsigned char opCode_t;
enum class MessageOpcode : opCode_t { set = 1, get = 2 };

typedef unsigned char opCode_t;
enum class GetSetType : opCode_t { member = 1, method = 2, rootState = 3 };

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

template <typename T, typename NodeT>
bool buildModMessage(NodeT &parentNode, const std::string &memberName, T i,
                     std::string &modBuf) {
  auto member =
      reflect::variants::getMemberWithName(parentNode, memberName.c_str());
  if (!member) {
    std::cout << "no member found for " << memberName << std::endl;
    return {};
  }
  auto optMemberIdx =
      reflect::variants::getIdxForMemberName(parentNode, memberName.c_str());
  if (!optMemberIdx) {
    std::cout << "no member Id found for " << memberName << std::endl;
    return {};
  }

  bool isValid = false;
  std::visit(
      [memberIdx = *optMemberIdx, &i, &isValid, &modBuf](auto &&arg) {
        using TRUT = std::decay_t<std::reference_wrapper<std::decay_t<T>>>;
        using ArgT = std::decay_t<decltype(arg)>;
        if (std::is_same<TRUT, ArgT>::value) {
          std::ostringstream oss;
          reflect::serialize::write_value<pack_ops_t>(
              oss, packOpCode(MessageOpcode::set, GetSetType::member));

          reflect::serialize::write_value<variants::MemberIdx>(oss, memberIdx);
          reflect::serialize::write_value<T>(oss, i);
          modBuf = oss.str();
          // std::cout << "good member type found for " << memberName << ","
          //           << modBuf.size() << "::" << std::endl;
          // for (const auto &m : modBuf)
          //   std::cout << (int)(char)m << ",";
          // std::cout << std::endl;ain
          isValid = true;
        }
      },
      *member);

  if (!isValid) {
    std::cout << "wrong member type found for " << memberName << std::endl;
  }
  return isValid;
}

template <typename NodeT>
bool buildCallMessage(NodeT &parentNode, reflect::proxy::MethodCallInfo *method,
                      std::string &modBuf) {

  if (!method) {
    std::cout << "no method provided ";
    return false;
  }
  std::ostringstream oss;
  reflect::serialize::write_value<pack_ops_t>(
      oss, packOpCode(MessageOpcode::set, GetSetType::method));

  method->to_bin(oss);
  modBuf = oss.str();
  return true;
}

template <typename NodeT>
bool buildCallResp(NodeT & /* parentNode */, const std::string &name,
                   const variants::AnyMethodReturnValue &resp,
                   std::string &modBuf) {

  std::ostringstream ossResp;
  std::visit(
      [&ossResp](auto &&arg) { reflect::serialize::write_value(ossResp, arg); },
      resp);
  auto respBuf = ossResp.str();
  if (respBuf.size() == 0)
    return false;

  std::ostringstream oss;
  reflect::serialize::write_value<pack_ops_t>(
      oss, packOpCode(MessageOpcode::get, GetSetType::method));

  reflect::serialize::write_value(oss, name);

  reflect::serialize::write_value<std::string>(oss, respBuf);

  modBuf = oss.str();
  return true;
}

void buildGetRootStateMessage(std::string &modBuf) {
  std::ostringstream oss;
  reflect::serialize::write_value<pack_ops_t>(
      oss, packOpCode(MessageOpcode::get, GetSetType::rootState));
  modBuf = oss.str();
}

template <typename NodeT>
void buildSetRootStateMessage(NodeT &parentNode, std::string &modBuf) {
  std::ostringstream oss;
  reflect::serialize::write_value<pack_ops_t>(
      oss, packOpCode(MessageOpcode::set, GetSetType::rootState));
  reflect::serialize::write_value<NodeT>(oss, parentNode);

  modBuf = oss.str();
}

struct MessageProcessorHandler {

  virtual void memberSet(const std::string &name,
                         reflect::variants::AnyMemberRefVar &v){};
  virtual void memberGet(const std::string &name,
                         reflect::variants::AnyMemberRefVar &v){};
  virtual void rootStateSet() {}
  virtual void rootStateGet() {}
  virtual void functionCall(const std::string &name,
                            reflect::variants::AnyMethodArgsValue &args,
                            reflect::variants::AnyMethodReturnValue &res){};
  // the only unimplemented functino, it's app reponsability to know what to do
  virtual void functionResp(const std::string &name,
                            reflect::variants::AnyMethodReturnValue &res){};
};

template <typename T>
bool processMessage(T &parentNode, char *beg, size_t length,
                    std::string &respBuf, MessageProcessorHandler *handler) {
  std::istringstream iss(std::string(beg, length));
  pack_ops_t op_pack = 0;
  reflect::serialize::parse_value<pack_ops_t>(op_pack, iss);
  auto [getSetType, op] = unPackOpCode(op_pack);
  if (MessageOpcode(op) == MessageOpcode::set ||
      (MessageOpcode(op) == MessageOpcode::get)) {
    bool isSet = MessageOpcode(op) == MessageOpcode::set;

    //////////
    // members
    ////////////
    if (GetSetType(getSetType) == GetSetType::member) {
      variants::MemberIdx memberIdx;
      reflect::serialize::parse_value<variants::MemberIdx>(memberIdx, iss);

      auto member = reflect::variants::getMemberWithIdx(parentNode, memberIdx);
      if (!member) {
        std::cout << "no member found for  idx" << memberIdx << std::endl;
        return {};
      }
      auto memberName =
          reflect::variants::getMemberNameForIdx(parentNode, memberIdx);
      if (!memberName) {
        std::cout << "no member name found for  idx" << memberIdx << std::endl;
        return {};
      }
      {
        if (isSet) {
          std::visit(
              [&iss](auto &&arg) {
                reflect::serialize::parse_value(arg.get(), iss);
              },
              *member);
          if (handler)
            handler->memberSet(memberName, *member);
          return false;

        } else {
          std::cout << "get member not tested" << memberName << std::endl;

          std::visit(
              [&respBuf, &parentNode, &memberName](auto &&arg) {
                buildModMessage(parentNode, memberName, arg.get(), respBuf);
              },
              *member);
          if (handler)
            handler->memberGet(memberName, *member);

          return true;
        }
      }
      std::cout << "!!!!!!! unreachable" << memberName << std::endl;
      return false;
    } else if (GetSetType(getSetType) == GetSetType::rootState) {
      if (isSet) {
        std::cout << "setting rootState " << std::endl;
        // std::cout << "!! rootState was :" << std::endl;
        // reflect::debug::dump(parentNode);
        // TODO full node address
        reflect::serialize::from_bin<T>(parentNode, iss);
        // std::cout << "!! rootState is now :" << std::endl;
        // reflect::debug::dump(parentNode);
        if (handler)
          handler->rootStateSet();
        return false;

      } else {
        std::cout << "get rootState " << std::endl;
        std::ostringstream oss;
        reflect::serialize::write_value<pack_ops_t>(
            oss, packOpCode(MessageOpcode::set, GetSetType::rootState));
        reflect::serialize::write_value<T>(oss, parentNode);
        respBuf = oss.str();
        if (handler)
          handler->rootStateGet();
        return true;
      }
    }
    ///////////
    /// methods
    ///////////
    else if (GetSetType(getSetType) == GetSetType::method) {
      std::string methodName;
      reflect::serialize::parse_value(methodName, iss);

      auto method =
          reflect::variants::getMethodWithName(parentNode, methodName.c_str());
      if (!method) {
        std::cout << "no method found for " << methodName << std::endl;
        if (isSet)
          std::cout << "set " << std::endl;
        else
          std::cout << "get " << std::endl;
        return {};
      }
      if (isSet) {
        std::cout << "function call " << methodName << std::endl;
        auto args = method->parse_args(iss);
        auto res = method->call(args);
        buildCallResp(parentNode, methodName, res, respBuf);
        if (handler)
          handler->functionCall(methodName, args, res);
        return respBuf.size() > 0;

      } else {
        std::cout << "function resp " << methodName << std::endl;
        auto resp = method->parse_resp(iss);
        if (handler)
          handler->functionResp(methodName, resp);
        else // TODO full node address
          std::cout << "resp function resp not implemented (no handler) "
                    << std::endl;

        return false;
      }
    }
  }

  std::cout << "op code not supported : " << op << length << std::endl;
  return false;
}

} // namespace reflect
