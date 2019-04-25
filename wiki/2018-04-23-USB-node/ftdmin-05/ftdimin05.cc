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


  FtdiContextWrapper::NewInstance(args);
}


void ftdi_usb_open_desc(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  FtdiContextWrapper* ctx = node::ObjectWrap::Unwrap<FtdiContextWrapper>(
    args[0]->ToObject());
  int vendor = args[1]->NumberValue();
  int product = args[2]->NumberValue();
  char *description = NULL;
  char *serial = NULL;

  String::Utf8Value d(args[3]->ToString());
  if (args[3]->IsString()) {
    description = *d;
  }

  String::Utf8Value s(args[4]->ToString());
  if (args[4]->IsString()) {
    serial = *s;
  }

  int ret = ftdi_usb_open_desc(&ctx->_ftdic, vendor, product, description,
    serial);

  args.GetReturnValue().Set(ret);
}

void ftdi_get_error_string(const v8::FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();

  FtdiContextWrapper* ctx = node::ObjectWrap::Unwrap<FtdiContextWrapper>(
    args[0]->ToObject());
  const char* ret = ftdi_get_error_string(&ctx->_ftdic);

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, ret));
}


void InitAll(Local<Object> exports) {
  FtdiContextWrapper::Init(exports);
  NODE_SET_METHOD(exports, "hello", Method);
  NODE_SET_METHOD(exports, "create_context", create_context);
  NODE_SET_METHOD(exports, "ftdi_usb_open_desc", ftdi_usb_open_desc);
  NODE_SET_METHOD(exports, "ftdi_get_error_string", ftdi_get_error_string);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, InitAll)

}  // namespace demo
