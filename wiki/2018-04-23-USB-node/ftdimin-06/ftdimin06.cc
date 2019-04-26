// addon.cc
#include <node.h>
#include "ftdi_context_wrapper.h"
#include <ftdi.h>

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

void ftdi_usb_open(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  FtdiContextWrapper* ctx = node::ObjectWrap::Unwrap<FtdiContextWrapper>(
    args[0]->ToObject());
  int vendor = args[1]->NumberValue();
  int product = args[2]->NumberValue();

  int ret = ftdi_usb_open(&ctx->_ftdic, vendor, product);

  args.GetReturnValue().Set(ret);
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

void ftdi_set_interface(const v8::FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  FtdiContextWrapper* ctx = node::ObjectWrap::Unwrap<FtdiContextWrapper>(
    args[0]->ToObject());

  //-- Obtener el numero de interfaz
  int ftdi_ifnum = args[1]->NumberValue();

  //-- Convertir al tipo correcto
  ftdi_interface ifnum = static_cast<ftdi_interface>(ftdi_ifnum);

  int ret = ftdi_set_interface(&ctx->_ftdic, static_cast<ftdi_interface>(ifnum));

  args.GetReturnValue().Set(ret);
}

void InitAll(Local<Object> exports) {
  FtdiContextWrapper::Init(exports);
  NODE_SET_METHOD(exports, "hello", Method);
  NODE_SET_METHOD(exports, "create_context", create_context);
  NODE_SET_METHOD(exports, "ftdi_usb_open_desc", ftdi_usb_open_desc);
  NODE_SET_METHOD(exports, "ftdi_usb_open", ftdi_usb_open);
  NODE_SET_METHOD(exports, "ftdi_get_error_string", ftdi_get_error_string);
  NODE_SET_METHOD(exports, "ftdi_set_interface", ftdi_set_interface);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, InitAll)

}  // namespace demo


//-- TODO

//ftdi_usb_open(&mpsse_ftdic, 0x0403, 0x6010)
//ftdi_usb_reset(&mpsse_ftdic)
//ftdi_usb_purge_buffers(&mpsse_ftdic)
//ftdi_get_latency_timer(&mpsse_ftdic, &mpsse_ftdi_latency)
//ftdi_set_latency_timer(&mpsse_ftdic, 1)
//ftdi_set_bitmode(&mpsse_ftdic, 0xff, BITMODE_MPSSE)
//int rc = ftdi_write_data(&mpsse_ftdic, &data, 1);
//int rc = ftdi_read_data(&mpsse_ftdic, &data, 1);
