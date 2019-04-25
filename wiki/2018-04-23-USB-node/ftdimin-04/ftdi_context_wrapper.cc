// ftdi_context_wrappercc
#include "ftdi_context_wrapper.h"

namespace demo {

using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::NewStringType;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;

Persistent<Function> FtdiContextWrapper::constructor;

FtdiContextWrapper::FtdiContextWrapper(double value) : value_(value) {
}

FtdiContextWrapper::~FtdiContextWrapper() {
}

void FtdiContextWrapper::Init(Local<Object> exports) {
  Isolate* isolate = exports->GetIsolate();

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(
      isolate, "FtdiContextWrapper", NewStringType::kNormal).ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "plusOne", PlusOne);

  Local<Context> context = isolate->GetCurrentContext();
  constructor.Reset(isolate, tpl->GetFunction(context).ToLocalChecked());
  exports->Set(context, String::NewFromUtf8(
      isolate, "FtdiContextWrapper", NewStringType::kNormal).ToLocalChecked(),
               tpl->GetFunction(context).ToLocalChecked()).FromJust();
}

void FtdiContextWrapper::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  if (args.IsConstructCall()) {
    // Invoked as constructor: `new FtdiContextWrapper(...)`
    double value = args[0]->IsUndefined() ?
        0 : args[0]->NumberValue(context).FromMaybe(0);
    FtdiContextWrapper* obj = new FtdiContextWrapper(value);
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    // Invoked as plain function `FtdiContextWrapper(...)`, turn into construct call.
    const int argc = 1;
    Local<Value> argv[argc] = { args[0] };
    Local<Function> cons = Local<Function>::New(isolate, constructor);
    Local<Object> result =
        cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(result);
  }
}

void FtdiContextWrapper::PlusOne(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  FtdiContextWrapper* obj = ObjectWrap::Unwrap<FtdiContextWrapper>(args.Holder());
  obj->value_ += 1;

  args.GetReturnValue().Set(Number::New(isolate, obj->value_));
}

}  // namespace demo
