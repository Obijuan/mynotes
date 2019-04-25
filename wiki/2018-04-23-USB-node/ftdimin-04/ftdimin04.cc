// addon.cc
#include <node.h>
#include "ftdi_context_wrapper.h"

namespace demo {

using v8::FunctionCallbackInfo;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::NewStringType;
using v8::Object;
using v8::String;
using v8::Value;

void Method(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  //-- Establecer el valor de retorno: Cadena "Hola!"
  args.GetReturnValue().Set(String::NewFromUtf8(
      isolate, "Hola!", NewStringType::kNormal).ToLocalChecked());
}

void create_context(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  FtdiContextWrapper::NewInstance(args);
}


void InitAll(Local<Object> exports) {
  FtdiContextWrapper::Init(exports);
  NODE_SET_METHOD(exports, "hello", Method);
  NODE_SET_METHOD(exports, "create_context", create_context);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, InitAll)

}  // namespace demo
