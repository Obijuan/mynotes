#ifndef FTDI_CONTEXT_WRAPPER_H
#define FTDI_CONTEXT_WRAPPER_H

#include <ftdi.h>
#include <node.h>
#include <node_object_wrap.h>

namespace demo {

class FtdiContextWrapper : public node::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);

 private:
  explicit FtdiContextWrapper(double value = 0);
  ~FtdiContextWrapper();

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void PlusOne(const v8::FunctionCallbackInfo<v8::Value>& args);
  static v8::Persistent<v8::Function> constructor;
  double value_;
  struct ftdi_context _ftdic;
};

}  // namespace demo

#endif
