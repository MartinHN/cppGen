namespace uapi {
struct MessageProcessorHandler;

struct TransportBase {
    virtual ~TransportBase() = default;
    virtual bool processMsg(char *c, size_t size, std::string &respBuf) = 0;
    uapi::MessageProcessorHandler *transportMsgHdlr = nullptr;
};

};  // namespace uapi
