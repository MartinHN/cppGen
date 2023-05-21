#pragma once

struct JsBindMessageProcessorHandler : public uapi::MessageProcessorHandler {
  JsBindMessageProcessorHandler(e::val g) : jsO(g), locker(g) {}

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
          [this, &j, &mName](auto &&ma) {
            using ArgT = std::decay_t<decltype(ma)>;
            using TRUT = typename uapi::traits::unwrap_ref<ArgT>::type;
            if constexpr (uapi::variants::isUserDefined<TRUT>::value ||
                          Vec<TRUT>) {
              dbg.print("entering from rootstate : ", mName);
              applyOnJs<TRUT>(j[mName], ma);
            } else {
              dbg.print("setting from rootstate : ", mName, TRUT(ma));
              jsO.set(mName, TRUT(ma));
            }
          },
          *m);
    }
  }
  void onRootStateSet(const std::string &addr) override {
    NestedCounter c(locker);
    applyOnJs(jsO, *jsBindCppObj);
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
  // the only unimplemented functino, it's app reponsability to know what to do
  void onFunctionResp(const std::string &name,
                      AnyMethodReturnValue &res) override {
    // TODO
    if (nextH)
      nextH->onFunctionResp(name, res);
  }
  e::val jsO;
  NestedLocker locker;
  MessageProcessorHandler *nextH = nullptr;
};
