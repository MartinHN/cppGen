#pragma once
struct ConnHandler {
  virtual ~ConnHandler() = default;
  virtual void onConnInit(){};
  virtual void onConnOpen(){};
  virtual void onConnError(){};
  virtual void onConnClose(){};
};

struct SocketImplBase {
  virtual void connect(int port) = 0;
  virtual void send_msg(const char *s, size_t size) = 0;
};
