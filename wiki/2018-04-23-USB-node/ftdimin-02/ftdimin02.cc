//-- ftdimin-02

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

  //struct ftdi_context _ftdic;

  //ftdi_init(&_ftdic);

  //-- Crear el objeto con el contexto dentro
  FtdiContextWrapper::NewInstance(args);
}

//-- Funcion de inicializacion
void Initialize(Local<Object> exports) {
  NODE_SET_METHOD(exports, "hello", Method);
  NODE_SET_METHOD(exports, "create_context", create_context);
}

//-- NODE_GYP_MODULE_NAME es el nombre del modulo
//-- Es el nombre del fichero .node
//-- node-gyp de encarga de ello
NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

}  // namespace demo
