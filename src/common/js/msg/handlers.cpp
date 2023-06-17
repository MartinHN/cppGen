#pragma once

template <typename T> void applyOnJs(e::val j, T &c) {
  if constexpr (!(uapi::variants::isUserDefined<T>::value || Vec<T>)) {
    dbg.print("!!! applyOnJs , unsupported type", cppTypeOf<T>);
    return;
  }
  for (int i = 0; i < uapi::variants::getNumMembers<T>(c); i++) {
    auto m = uapi::variants::getMemberWithIdx(c, i);
    if (!m) {
      dbg.print("!!! err , no member found");
      continue;
    }
    auto mName = uapi::variants::getMemberNameForIdx<T>(i);
    std::visit(
        [&j, &mName](auto &&ma) {
          using ArgT = std::decay_t<decltype(ma)>;
          using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
          if constexpr (uapi::variants::isUserDefined<TRUT>::value ||
                        Vec<TRUT>) {
            dbg.print("entering from rootstate : ", mName);
            applyOnJs<TRUT>(j[mName], ma);
          } else {
            dbg.print("setting from rootstate : ", mName, TRUT(ma));
            j.set(mName, TRUT(ma));
          }
        },
        *m);
  }
}

template <typename T>
struct JsBindMessageProcessorHandler : public uapi::MessageProcessorHandler {
  JsBindMessageProcessorHandler(T &_api, e::val g)
      : jsO(g), rootApiObj(_api), locker(g) {}

  void onMemberSet(const std::string &addr, AnyMemberRefVar &v) override {
    dbg.print("setting member::", addr);
    NestedCounter c(locker);
    logObj(jsO);
    auto addrVec = uapi::variants::strAddrFromStr(addr);
    auto lastM = addrVec.back();
    addrVec.pop_back();
    auto targetO = jsO;
    for (auto &a : addrVec) {
      targetO = targetO[a];
    }
    targetO.set(lastM, anyMemberToVal(v));
    logObj(jsO);
    if (nextH)
      nextH->onMemberSet(addr, v);
  }
  void onMemberGet(const std::string &addr, AnyMemberRefVar &v) override {
    // TODO
    if (nextH)
      nextH->onMemberGet(addr, v);
  }

  void onRootStateSet(const std::string &addr) override {
    NestedCounter c(locker);
    applyOnJs(jsO, rootApiObj);
    // TODO
    if (nextH)
      nextH->onRootStateSet(addr);
  }
  void onRootStateGet(const std::string &addr) override {
    // TODO
    if (nextH)
      nextH->onRootStateGet(addr);
  }
  void onFunctionCall(const std::string &name, AnyMethodArgsTuple &args,
                      AnyMethodReturnValue &res) override {
    // TODO
    if (nextH)
      nextH->onFunctionCall(name, args, res);
  }
  // the only unimplemented functino, it's app reponsability to know what to
  // do
  void onFunctionResp(const std::string &name,
                      AnyMethodReturnValue &res) override {
    // TODO
    if (nextH)
      nextH->onFunctionResp(name, res);
  }
  e::val jsO;
  T &rootApiObj;
  NestedLocker locker;
  MessageProcessorHandler *nextH = nullptr;
};

struct JsMessageHandlerWrapper : public JsObjWrapper,
                                 public uapi::MessageProcessorHandler {
  JsMessageHandlerWrapper(const emscripten::val &di) : JsObjWrapper(di) {
    // dbg.print("type of di : ", di.typeOf().as<std::string>());
    // logObj(dispatcher);
    call("onInit");
  }
  // should be given as an instance of JSEmptyHandler

  void onMemberSet(const std::string &addr,
                   uapi::variants::AnyMemberRefVar &v) override {
    call("onSet", addr);
  };
  void onMemberGet(const std::string &addr,
                   uapi::variants::AnyMemberRefVar &v) override {
    call("onGet", addr);
  };
  void onRootStateSet(const std::string &addr) override {
    call("onRootStateSet", addr);
  }
  void onRootStateGet(const std::string &addr) override {}
  void onFunctionCall(const std::string &addr,
                      uapi::variants::AnyMethodArgsTuple &args,
                      uapi::variants::AnyMethodReturnValue &res) override {
    // the only unimplemented function, it's app reponsability to know what
    // to
    call("onCall", addr);
  };

  void onFunctionResp(const std::string &addr,
                      uapi::variants::AnyMethodReturnValue &res) override {
    std::visit(
        [this, &addr](auto &&r) {
          using T = decltype(r);
          if constexpr (uapi::traits::printable<T>)
            dbg.print(r, cppTypeOf<T>());
          emscripten::val respV(r);
          call("onCallResp", addr, respV);
        },
        res);
  };
};

struct JSEmptyMsgHandler {
  void onInit(){};
  void onSet(const uapi::variants::MemberAddressStr &s,
             const uapi::variants::MemberAddressStr &name){};
  void onGet(const uapi::variants::MemberAddressStr &s,
             const uapi::variants::MemberAddressStr &name){};
  void onCall(const std::string &addr, const std::string &name){};
  void onCallResp(const std::string &addr, const emscripten::val &resp){};
  void onRootStateSet(const std::string &addr){};
};

EMSCRIPTEN_BINDINGS(JsMessageHandler) {
  emscripten::class_<JSEmptyMsgHandler>("JSHandler")
      .constructor()
      .function("onInit", &JSEmptyMsgHandler::onInit)
      .function("onSet", &JSEmptyMsgHandler::onSet)
      .function("onGet", &JSEmptyMsgHandler::onGet)
      .function("onCall", &JSEmptyMsgHandler::onCall)
      .function("onCallResp", &JSEmptyMsgHandler::onCallResp)
      .function("onRootStateSet", &JSEmptyMsgHandler::onRootStateSet);
}
