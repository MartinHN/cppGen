emscripten::val strToU8ArrJs(const std::string &str) {
  // returns array copy
  auto uArr = emscripten::val::global("Uint8Array");
  return uArr.new_(
      emscripten::typed_memory_view(str.size(), (uint8_t *)str.data()));
}

template <typename T> struct JSMsgBuilder {
  T *parentNode;
  JSMsgBuilder(T *p) : parentNode(p) {
    dbg.print("creating builder for api", p);
  }
  /// setters
  e::val buildJsBindModMessage(const std::string &jsAddr, const e::val &val) {
    std::string modBuf;
    auto aStr = uapi::variants::strAddrFromStr(jsAddr);
    dbg.print("build modmsg for member ", aStr);
    auto intAddr = uapi::variants::addressStrToInt(*parentNode, aStr);
    if (!intAddr) {
      dbg.print("no member found for ", jsAddr);
      return {};
    }
    auto member =
        uapi::variants::getMemberWithAddressInt(*parentNode, *intAddr);
    if (!member) {
      dbg.print("no member found for ", jsAddr);
      return {};
    }

    bool isValid = false;
    std::visit(
        [this, &isValid, &modBuf, &jsAddr, &val](auto &&m) {
          using MT = std::decay_t<decltype(m)>;
          using TT = uapi::traits::unwrap_ref<MT>::type;
          auto vv = getValAs<TT>(val);
          isValid = uapi::buildModMessage(*parentNode, jsAddr, vv, modBuf);
        },
        *member);
    if (!isValid) {
      dbg.print("not valid");
      return strToU8ArrJs("");
    }

    // // apply to internal value
    // std::string fkBuf;
    // uapi::processMessage(jsBindCppObj, modBuf.data(), modBuf.size(),fkBuf,nullptr);
    return strToU8ArrJs(modBuf);
  }

  bool callMsgNeedsResp(const std::string &jsAddr,
                        const std::string &methodName) {
    auto aStr = uapi::variants::strAddrFromStr(jsAddr);
    auto intAddr = uapi::variants::addressStrToInt(*parentNode, aStr);
    if (!intAddr) {
      dbg.print("no member found for ", jsAddr);
      return {};
    }
    auto member =
        uapi::variants::getMemberWithAddressInt(*parentNode, *intAddr);
    if (!member) {
      dbg.print("no member found for ", jsAddr);
      return {};
    }

    uapi::variants::Method *method = nullptr;

    std::visit(
        [&method, &methodName](auto &&m) {
          using MT = std::decay_t<decltype(m)>;
          using TT = uapi::traits::unwrap_ref<MT>::type;
          if constexpr (uapi::variants::isUserDefined<TT>::value)
            method =
                uapi::variants::getMethodWithName<TT>(m, methodName.c_str());
        },
        *member);

    if (!method) {
      dbg.print("not valid");
      return {};
    }

    auto returnT = method->getExpectedReturnType();
    bool needResp = true;
    std::visit(
        [&needResp](auto &&m) {
          using TT = decltype(m);
          needResp = !std::is_same_v<TT, uapi::variants::VoidReturn>;
        },
        returnT);
    return needResp;
  }

  /// getters
  e::val buildJsBindMemberGetMessage(const std::string &memberAddr) {
    std::string modBuf;
    uapi::buildGetMessage(*parentNode, memberAddr, modBuf);
    return strToU8ArrJs(modBuf);
  }

  e::val buildGetRootStateMessageJs(const std::string &jsAddr) {
    std::string modBuf;
    uapi::buildGetRootStateMessage(*parentNode, jsAddr, modBuf);
    return strToU8ArrJs(modBuf);
  }

  /// callers
  e::val buildJsBindCallMessage(const std::string &jsAddr,
                                const std::string &methodName,
                                const e::val &args) {
    std::string modBuf;
    auto aStr = uapi::variants::strAddrFromStr(jsAddr);
    auto intAddr = uapi::variants::addressStrToInt(*parentNode, aStr);
    if (!intAddr) {
      dbg.print("no member found for ", jsAddr);
      return {};
    }
    auto member =
        uapi::variants::getMemberWithAddressInt(*parentNode, *intAddr);
    if (!member) {
      dbg.print("no member found for ", jsAddr);
      return {};
    }

    uapi::variants::Method *method = nullptr;

    std::visit(
        [&method, &methodName](auto &&m) {
          using MT = std::decay_t<decltype(m)>;
          using TT = uapi::traits::unwrap_ref<MT>::type;
          if constexpr (uapi::variants::isUserDefined<TT>::value)
            method =
                uapi::variants::getMethodWithName<TT>(m, methodName.c_str());
        },
        *member);

    if (!method) {
      dbg.print("not valid");
      return strToU8ArrJs("");
    }

    auto argsToFill = method->getExpectedArgsTuple();
    bool isValid = parseJsMethodArgs(args, argsToFill);
    if (!isValid) {
      dbg.print("js args not valid");
      return strToU8ArrJs("");
    }
    isValid = uapi::buildCallMessage(*parentNode, jsAddr, methodName,
                                     argsToFill, modBuf);

    if (!isValid) {
      dbg.print("call message not valid");
      return strToU8ArrJs("");
    }

    dbg.print("valid");
    // // apply to internal value
    // std::string fkBuf;
    // uapi::processMessage(jsBindCppObj, modBuf.data(), modBuf.size(),
    // fkBuf,
    //                         nullptr);
    return strToU8ArrJs(modBuf);
  }
};

#define DECLARE_JS_BUILDER(builderName, RootType)                              \
  EMSCRIPTEN_BINDINGS(builderName) {                                           \
    emscripten::class_<JSMsgBuilder<RootType>>(#builderName)                   \
        .constructor<RootType *>()                                             \
        .function("buildJsBindModMessage",                                     \
                  &JSMsgBuilder<RootType>::buildJsBindModMessage)              \
        .function("buildJsBindMemberGetMessage",                               \
                  &JSMsgBuilder<RootType>::buildJsBindMemberGetMessage)        \
        .function("buildJsBindCallMessage",                                    \
                  &JSMsgBuilder<RootType>::buildJsBindCallMessage)             \
        .function("buildGetRootStateMessage",                                  \
                  &JSMsgBuilder<RootType>::buildGetRootStateMessageJs)         \
        .function("callMsgNeedsResp",                                          \
                  &JSMsgBuilder<RootType>::callMsgNeedsResp);                  \
  };
